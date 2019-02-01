#include "schedule_tester.h"



//https://stackoverflow.com/questions/33991918/link-to-pthread-library-using-cmake-in-clion


//typedef struct sched_param sched_param;

// gcc -Wall -pthread -o schedule_tester schedule_tester.c
// sudo ./schedule_tester -p 10000 -l 500000
// sudo valgrind --leak-check=full ./schedule_tester -p 7 -l 10000


int main(int argc, char** argv ) {

    char pstring[1024];
    int c = 0;
    params1 = malloc(sizeof(params));
    params1->loops = 0;
    int choice;

    while ((c = getopt(argc, argv, "p:l:")) != -1) {

        switch (c) {
            case 'p':
                strcpy(pstring, *(argv + optind - 1));
                if ((NRTHR = atoi(pstring)) != 0)
                    NRTHR--;
                else {
                    fprintf(stderr, "p-flag needs to be an int > 0\n");
                    exit(EXIT_FAILURE);
                }

                break;
            case 'l':
                strcpy(pstring, *(argv + optind - 1));

                if ((params1->loops = atoi(pstring)) <= 0) {
                    fprintf(stderr, "l-flag must be an integer > 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stdout, "BAD FLAG!\n");
                exit(EXIT_FAILURE);

        }
    }

    if (argc < 0 + optind) {
        fprintf(stderr, "Input syntax is: schedule_tester [-p threads] [-l loops]");
        exit(EXIT_FAILURE);
    }


    printf("Choose testcase\n");

    do {

        printf("\n1. Measure throughput\n");
        printf("2. Measure latency\n");
        printf("6. End\n");


        printf("\nGive your choice (1 - 6): ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                measure_throughput_or_latency(false);
                break;
            case 2:
                measure_throughput_or_latency(true);
                break;
            case 6:
                printf("Closing program");

                break;
            default:
                fprintf(stdout, "BAD INPUT!\n");

        }



    }while (choice != 6);
    free(params1);
}


void measure_throughput_or_latency(bool latency) {
    struct timespec start, end;
    struct sched_param sched;
    int number_schedulers = 3;
    int number_test = 3;
    unsigned long time=0;

    pid_t pid = getpid();
    pthread_t *trd = calloc((size_t) NRTHR, sizeof(pthread_t));

    for(int i = 0; i < number_schedulers; i++) {
        time = 0;
        sched.sched_priority = sched_get_priority_max(schedulers[i]);

        if (sched_setscheduler(pid, schedulers[i], &sched) < 0) {
            fprintf(stderr, "SETSCHEDULER failed - err = %s\n", strerror(errno));
        } else {
            //printf("Priority set to \"%d\"\n", sched.sched_priority);
        }

        for (int l = 0; l < number_test; ++l) {

            clock_gettime(CLOCK_REALTIME, &start);
            for (int j = 0; j < NRTHR; ++j) {
                if (pthread_create(&trd[j], NULL, work, (void *) params1)) {
                    perror("ERROR creating thread.");
                    exit(EXIT_FAILURE);
                }
            }
            work((void *) params1);

            for (int k = 0; k < NRTHR; ++k) {
                if (pthread_join(trd[k], NULL)) {
                    perror("pthread_join");
                    exit(EXIT_FAILURE);
                }
            }
            clock_gettime(CLOCK_REALTIME, &end);

            time += usec_since(&start, &end);
        }
        if (latency) {
            unsigned long latency_time = (time / number_test);
            if (sched_getscheduler(pid) == SCHED_OTHER) {
                printf("SCHED_OTHER Latency: %.8lu seconds\n", latency_time);
            } else if (sched_getscheduler(pid) == SCHED_FIFO) {
                printf("SCHED_FIFO Latency: %li seconds\n", latency_time);
            } else if (sched_getscheduler(pid) == SCHED_RR) {
                printf("SCHED_RR Latency: %li seconds\n", latency_time);
            }
        }
        else {
            unsigned long throughput = ((NRTHR + 1) * 1000000UL) / (time / number_test);
            if (sched_getscheduler(pid) == SCHED_OTHER) {
                printf("SCHED_OTHER Throughput: %li threads/second\n", throughput);
            } else if (sched_getscheduler(pid) == SCHED_FIFO) {
                printf("SCHED_FIFO Throughput: %li threads/second\n", throughput);
            } else if (sched_getscheduler(pid) == SCHED_RR) {
                printf("SCHED_RR Throughput: %li threads/second\n", throughput);
            }

        }
    }

    free(trd);

}

void *work(void* param){
    //pid_t pid = getpid();
    params *params1 = (params*)param;
    int a = 0;

    for (int i = 0; i < params1->loops; ++i) {
        a++;
    }
    //sleep(1);

    return NULL;
}

static unsigned long usec_since(struct timespec *start, struct timespec *end) {
    unsigned long long s, e;

    s = start->tv_sec * 1000000000ULL + start->tv_nsec;
    e =   end->tv_sec * 1000000000ULL +   end->tv_nsec;

    unsigned long time = (e - s)/1000;

    //printf("Time elapsed: %.9lu microcseconds\n", time);
    printf("%d.%.9ld\n", (int) (end->tv_sec - start->tv_sec), end->tv_nsec - start->tv_nsec);

    return time;
}