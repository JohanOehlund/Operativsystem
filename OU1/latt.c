/*
 * Simple latency tester that combines multiple processes.
 *
 * Compile: gcc -Wall -O2 -D_GNU_SOURCE -lrt -lm -lz -o latt latt.c
 *
 * Run with: latt -c8 'program --args'
 *
 * Options:
 *
 *	-cX	Use X number of clients
 *	-sX	Use X msec as the minimum sleep time for the parent
 *	-SX	Use X msec as the maximum sleep time for the parent
 *	-xX	Use X kb as the work buffer to randomize and compress
 *	-v	Print all delays as they are logged
 *
 * (C) Jens Axboe <jens.axboe@oracle.com> 2009
 * Fixes from Peter Zijlstra <a.p.zijlstra@chello.nl> to actually make it
 * measure what it was intended to measure.
 * Fix from Ingo Molnar for poll() using 0 as timeout (should be negative).
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <time.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <zlib.h>
#include <string.h>

const char version[] = "latt-0.1";

/*
 * In msecs
 */
static unsigned int min_delay = 100;
static unsigned int max_delay = 500;
static unsigned int clients = 1;
static unsigned int compress_size = 64 * 1024;
static unsigned int verbose;

#define print_if_verbose(args...)		\
	do {					\
	if (verbose) {				\
		fprintf(stderr, ##args);	\
		fflush(stderr);			\
	}					\
	} while (0)

#define MAX_CLIENTS		512

static int pipes[MAX_CLIENTS*2][2];
static pid_t app_pid;

#define CLOCKSOURCE		CLOCK_MONOTONIC

struct stats
{
    const char *name;
    double n, mean, M2, max, max_client;
};

static struct stats delay_stats = {
        .name	= "Wakeup averages",
};
static struct stats delay_work_stats = {
        .name	= "Work averages",
};

struct return_stamp {
    struct timespec pre_work;
    struct timespec post_work;
};

static void update_stats(struct stats *stats, unsigned long long val)
{
    double delta, x = val;

    stats->n++;
    delta = x - stats->mean;
    stats->mean += delta / stats->n;
    stats->M2 += delta*(x - stats->mean);

    if (stats->max < x)
        stats->max = x;
}

static unsigned long nr_stats(struct stats *stats)
{
    return stats->n;
}

static double max_stats(struct stats *stats)
{
    return stats->max;
}

static double avg_stats(struct stats *stats)
{
    return stats->mean;
}

/*
 * http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
 *
 *       (\Sum n_i^2) - ((\Sum n_i)^2)/n
 * s^2 = -------------------------------
 *                  n - 1
 *
 * http://en.wikipedia.org/wiki/Stddev
 */
static double stddev_stats(struct stats *stats)
{
    double variance = stats->M2 / (stats->n - 1);

    return sqrt(variance);
}

/*
 * The std dev of the mean is related to the std dev by:
 *
 *             s
 * s_mean = -------
 *          sqrt(n)
 *
 */
static double stddev_mean_stats(struct stats *stats)
{
    double variance = stats->M2 / (stats->n - 1);
    double variance_mean = variance / stats->n;

    return sqrt(variance_mean);
}

struct sem {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int value;
    int waiters;
};

static void init_sem(struct sem *sem)
{
    pthread_mutexattr_t attr;
    pthread_condattr_t cond;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_init(&cond);
    pthread_condattr_setpshared(&cond, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&sem->cond, &cond);
    pthread_mutex_init(&sem->lock, &attr);

    sem->value = 0;
    sem->waiters = 0;
}

static void sem_down(struct sem *sem)
{
    pthread_mutex_lock(&sem->lock);

    while (!sem->value) {
        sem->waiters++;
        pthread_cond_wait(&sem->cond, &sem->lock);
        sem->waiters--;
    }

    sem->value--;
    pthread_mutex_unlock(&sem->lock);
}

static void sem_up(struct sem *sem)
{
    pthread_mutex_lock(&sem->lock);
    if (!sem->value && sem->waiters)
        pthread_cond_signal(&sem->cond);
    sem->value++;
    pthread_mutex_unlock(&sem->lock);
}

static int parse_options(int argc, char *argv[])
{
    struct option l_opts[] = {
            { "min-delay (msec)", 	1, 	NULL,	's' },
            { "max-delay (msec)",	1,	NULL,	'S' },
            { "clients",		1,	NULL,	'c' },
            { "compress-size (kb)",	1,	NULL,	'x' },
            { "verbose",		0,	NULL,	'v' },
            { "help",		0,	NULL,	'h' },
            { "version",		0,	NULL,	'V' },
            { NULL,					    },
    };
    const char optstr[] = "s:S:c:vhx:V";
    int c, res, index = 0, i;

    while ((c = getopt_long(argc, argv, optstr, l_opts, &res)) != -1) {
        index++;
        switch (c) {
            case 's':
                min_delay = atoi(optarg);
                break;
            case 'S':
                max_delay = atoi(optarg);
                break;
            case 'c':
                clients = atoi(optarg);
                if (clients > MAX_CLIENTS) {
                    clients = MAX_CLIENTS;
                    printf("Capped clients to %d\n", clients);
                }
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                for (i = 0; l_opts[i].name; i++)
                    printf("-%c: %s\n", l_opts[i].val, l_opts[i].name);
                break;
            case 'x':
                compress_size = atoi(optarg);
                compress_size *= 1024;
                break;
            case 'V':
                printf("%s\n", version);
                exit(0);
        }
    }

    return index + 1;
}

static pid_t fork_off(const char *app)
{
    pid_t pid;

    pid = fork();
    if (pid)
        return pid;

    exit(system(app));
}

static unsigned long usec_since(struct timespec *start, struct timespec *end)
{
    unsigned long long s, e;

    s = start->tv_sec * 1000000000ULL + start->tv_nsec;
    e =   end->tv_sec * 1000000000ULL +   end->tv_nsec;

    return (e - s) / 1000;
}

static void log_delay(struct stats *s, unsigned long delay)
{
    print_if_verbose("log delay %8lu usec (pid=%d)\n", delay, getpid());

    update_stats(s, delay);
}

static unsigned long pseed = 1;

static int get_rand(void)
{
    pseed = pseed * 1103515245 + 12345;
    return ((unsigned) (pseed / 65536) % 32768);
}

static void fill_random_data(int *buf, unsigned int nr_ints)
{
    int i;

    for (i = 0; i < nr_ints; i++)
        buf[i] = get_rand();
}

/*
 * Generate random data and compress it with zlib
 */
static void do_work(void)
{
    unsigned long work_size = compress_size;
    z_stream stream;
    int ret, bytes;
    void *zbuf;
    int *buf;

    memset(&stream, 0, sizeof(stream));
    deflateInit(&stream, 7);
    bytes = deflateBound(&stream, work_size);

    zbuf = malloc(bytes);
    buf = malloc(work_size);
    fill_random_data(buf, work_size / sizeof(int));

    stream.next_in = (void *) buf;
    stream.avail_in = work_size;
    stream.next_out = zbuf;
    stream.avail_out = bytes;

    do {
        ret = deflate(&stream, Z_FINISH);
    } while (ret == Z_OK);

    deflateEnd(&stream);
    free(buf);
    free(zbuf);
}

/*
 * Reads a timestamp (which is ignored, it's just a wakeup call), and replies
 * with the timestamp of when we saw it
 */
static void run_child(int *in, int *out, struct sem *sem)
{
    struct timespec ts;
    struct return_stamp rs;

    print_if_verbose("present: %d\n", getpid());

    sem_up(sem);

    do {
        int ret;

        ret = read(in[0], &ts, sizeof(ts));
        if (ret <= 0)
            break;
        else if (ret != sizeof(ts))
            break;

        clock_gettime(CLOCKSOURCE, &rs.pre_work);

        if (compress_size) {
            do_work();
            clock_gettime(CLOCKSOURCE, &rs.post_work);
        }

        ret = write(out[1], &rs, sizeof(rs));
        if (ret <= 0)
            break;
        else if (ret != sizeof(rs))
            break;

        print_if_verbose("alive: %d\n", getpid());
    } while (1);
}

/*
 * Do a random sleep between min and max delay
 */
static void do_rand_sleep(void)
{
    unsigned int msecs;

    msecs = min_delay + ((float) max_delay * (rand() / (RAND_MAX + 1.0)));
    print_if_verbose("sleeping for: %u msec\n", msecs);
    usleep(msecs * 1000);
}

static void kill_connection(void)
{
    int i;

    for (i = 0; i < 2*clients; i++) {
        if (pipes[i][0] != -1) {
            close(pipes[i][0]);
            pipes[i][0] = -1;
        }
        if (pipes[i][1] != -1) {
            close(pipes[i][1]);
            pipes[i][1] = -1;
        }
    }
}

static int __write_ts(int i, struct timespec *ts)
{
    int fd = pipes[2*i][1];

    clock_gettime(CLOCKSOURCE, ts);

    return write(fd, ts, sizeof(*ts)) != sizeof(*ts);
}

static long __read_ts(int i, struct timespec *ts, pid_t *cpids)
{
    int fd = pipes[2*i+1][0];
    unsigned long delay, work_delay;
    struct return_stamp rs;

    if (read(fd, &rs, sizeof(rs)) != sizeof(rs))
        return -1;

    delay = usec_since(ts, &rs.pre_work);
    log_delay(&delay_stats, delay);

    if (compress_size) {
        work_delay = usec_since(ts, &rs.post_work);
        log_delay(&delay_work_stats, work_delay - delay);
    }

    print_if_verbose("got delay %ld from child %d [pid %d]\n", delay,
                     i, cpids[i]);
    return 0;
}

static int read_ts(struct pollfd *pfd, unsigned int nr, struct timespec *ts,
                   pid_t *cpids)
{
    unsigned int i;

    for (i = 0; i < clients; i++) {
        if (pfd[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            return -1L;
        if (pfd[i].revents & POLLIN) {
            pfd[i].events = 0;
            if (__read_ts(i, &ts[i], cpids))
                return -1L;
            nr--;
        }
        if (!nr)
            break;
    }

    return 0;
}

static int app_has_exited(void)
{
    int ret, status;

    /*
     * If our app has exited, stop
     */
    ret = waitpid(app_pid, &status, WNOHANG);
    if (ret < 0) {
        perror("waitpid");
        return 1;
    } else if (ret == app_pid &&
               (WIFSIGNALED(status) || WIFEXITED(status))) {
        return 1;
    }

    return 0;
}

/*
 * While our given app is running, send a timestamp to each client and
 * log the maximum latency for each of them to wakeup and reply
 */
static void run_parent(pid_t *cpids)
{
    struct pollfd *ipfd;
    int do_exit = 0, i;
    struct timespec *t1;

    t1 = malloc(sizeof(struct timespec) * clients);
    ipfd = malloc(sizeof(struct pollfd) * clients);

    srand(1234);

    do {
        unsigned pending_events;

        do_rand_sleep();

        if (app_has_exited())
            break;

        for (i = 0; i < clients; i++) {
            ipfd[i].fd = pipes[2*i+1][0];
            ipfd[i].events = POLLIN;
        }

        /*
         * Write wakeup calls
         */
        for (i = 0; i < clients; i++) {
            print_if_verbose("waking: %d\n", cpids[i]);

            if (__write_ts(i, t1+i)) {
                do_exit = 1;
                break;
            }
        }

        if (do_exit)
            break;

        /*
         * Poll and read replies
         */
        pending_events = clients;
        while (pending_events) {
            int evts = poll(ipfd, clients, -1);

            if (evts < 0) {
                do_exit = 1;
                break;
            } else if (!evts)
                continue;

            if (read_ts(ipfd, evts, t1, cpids)) {
                do_exit = 1;
                break;
            }

            pending_events -= evts;
        }
    } while (!do_exit);

    kill_connection();
    free(t1);
    free(ipfd);
}

static void run_test(void)
{
    struct sem *sem;
    pid_t *cpids;
    int i, status;

    sem = mmap(NULL, sizeof(*sem), PROT_READ|PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (sem == MAP_FAILED) {
        perror("mmap");
        return;
    }

    init_sem(sem);

    for (i = 0; i < 2*clients; i++) {
        if (pipe(pipes[i])) {
            perror("pipe");
            return;
        }
    }

    cpids = malloc(sizeof(pid_t) * clients);

    for (i = 0; i < clients; i++) {
        cpids[i] = fork();
        if (cpids[i]) {
            sem_down(sem);
            continue;
        }

        run_child(pipes[2*i], pipes[2*i+1], sem);
        exit(0);
    }

    run_parent(cpids);

    for (i = 0; i < clients; i++)
        kill(cpids[i], SIGQUIT);
    for (i = 0; i < clients; i++)
        waitpid(cpids[i], &status, 0);

    free(cpids);
    munmap(sem, sizeof(*sem));
}

static void show_stats(struct stats *s)
{
    if (!s->n)
        return;

    printf("\n%s\n", s->name);
    printf("-------------------------------------\n");
    printf("\tMax\t\t%8.0f usec\n", max_stats(s));
    printf("\tAvg\t\t%8.0f usec\n", avg_stats(s));
    printf("\tStdev\t\t%8.0f usec\n", stddev_stats(s));
    printf("\tStdev mean\t%8.0f usec\n", stddev_mean_stats(s));
}

static void handle_sigint(int sig)
{
    kill(app_pid, SIGINT);
}

int main(int argc, char *argv[])
{
    int app_offset, off;
    char app[256];

    off = 0;
    app_offset = parse_options(argc, argv);
    if (app_offset >= argc) {
        printf("%s: [options] 'app'\n", version);
        return 1;
    }

    while (app_offset < argc) {
        if (off) {
            app[off] = ' ';
            off++;
        }
        off += sprintf(app + off, "%s", argv[app_offset]);
        app_offset++;
    }

    signal(SIGINT, handle_sigint);

    /*
     * Start app and start logging latencies
     */
    app_pid = fork_off(app);
    run_test();

    printf("\nParameters: min_wait=%ums, max_wait=%ums, clients=%u\n",
           min_delay, max_delay, clients);
    printf("Entries logged: %lu\n", nr_stats(&delay_stats));

    show_stats(&delay_stats);
    show_stats(&delay_work_stats);

    return 0;
}