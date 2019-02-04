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
    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&cond, NULL);



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
        work_time = 0;
        big_work_time = 0;
        wait_time = 0;

        printf("\n1. Measure throughput\n");
        printf("2. Measure latency\n");
        printf("3. Measure tail latency\n");
        printf("4. Measure waiting time\n");
        printf("5. Change number of threads and/or loops\n");
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
            case 3:
                measure_tail_latency();
                break;
            case 4:
                measure_waiting_time();
                break;
            case 5:
                change_threads_and_loops();
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

void change_threads_and_loops() {

    printf("\nHow many threads do you want?\n");
    scanf("%d", &NRTHR);
    NRTHR -= 1;
    printf("\nHow many loops do you want?\n");
    scanf("%d", &params1->loops);

}

void measure_waiting_time() {

    pid_t pid = getpid();
    pthread_t *trd = calloc((size_t) NRTHR, sizeof(pthread_t));

    for(int i = 0; i < number_schedulers; i++) {
        sched.sched_priority = sched_get_priority_max(schedulers[i]);

        if (sched_setscheduler(pid, schedulers[i], &sched) < 0) {
            fprintf(stderr, "SETSCHEDULER failed - err = %s\n", strerror(errno));
        } else {
            //printf("Priority set to \"%d\"\n", sched.sched_priority);
        }

        for (int l = 0; l < number_test; ++l) {
            params *param1 = malloc(sizeof(params)*(NRTHR+1));
            for (int j = 0; j < NRTHR; ++j) {
                param1[j].loops = params1->loops;
                clock_gettime(CLOCK_REALTIME, &param1[j].start_wait);
                if (pthread_create(&trd[j], NULL, work, (void *) param1)) {
                    perror("ERROR creating thread.");
                    exit(EXIT_FAILURE);
                }
            }
            clock_gettime(CLOCK_REALTIME, &param1[NRTHR].start_wait);
            work((void *) param1);

            for (int k = 0; k < NRTHR; ++k) {
                if (pthread_join(trd[k], NULL)) {
                    perror("pthread_join");
                    exit(EXIT_FAILURE);
                }
            }
            free(param1);
        }
        double mean_wait_time = wait_time / (NRTHR + 1);


        if (sched_getscheduler(pid) == SCHED_OTHER) {
            printf("SCHED_OTHER Waiting time: The average waiting time of a thread %lf seconds\n", mean_wait_time);
        } else if (sched_getscheduler(pid) == SCHED_FIFO) {
            printf("SCHED_FIFO TWaiting time: The average waiting time of a thread %lf seconds\n", mean_wait_time);
        } else if (sched_getscheduler(pid) == SCHED_RR) {
            printf("SCHED_RR Waiting time: The average waiting time of a thread %lf seconds\n", mean_wait_time);
        }

    }

    free(trd);

}

void measure_tail_latency() {

    pid_t pid = getpid();
    pthread_t *trd = calloc((size_t) NRTHR, sizeof(pthread_t));

    for(int i = 0; i < number_schedulers; i++) {
        big_work_time = 0.0;
        work_time = 0.0;
        sched.sched_priority = sched_get_priority_max(schedulers[i]);

        if (sched_setscheduler(pid, schedulers[i], &sched) < 0) {
            fprintf(stderr, "SETSCHEDULER failed - err = %s\n", strerror(errno));
        } else {
            //printf("Priority set to \"%d\"\n", sched.sched_priority);
        }

        for (int l = 0; l < number_test; ++l) {

            for (int j = 0; j < 1; ++j) {
                if (pthread_create(&trd[j], NULL, big_work, (void *) params1)) {
                    perror("ERROR creating thread.");
                    exit(EXIT_FAILURE);
                }
            }
            for (int j = 1; j < NRTHR; ++j) {
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
        }

        double tail_latency_time_work = (work_time / NRTHR) / number_test;
        double tail_latency_time_big_work = (big_work_time / 1) / number_test;

        double percentage = ((tail_latency_time_big_work / tail_latency_time_work) * 100) -100;

        if (sched_getscheduler(pid) == SCHED_OTHER) {
            printf("SCHED_OTHER Tail latency: A Thread with double work took %2.lf%% longer time to finish\n", percentage);
        } else if (sched_getscheduler(pid) == SCHED_FIFO) {
            printf("SCHED_FIFO Tail latency:A Thread with double work took %2.lf%% longer time to finish\n", percentage);
        } else if (sched_getscheduler(pid) == SCHED_RR) {
            printf("SCHED_RR Tail latency: A Thread with double work took %2.lf%% longer time to finish\n", percentage);
        }

    }

    free(trd);
}


void measure_throughput_or_latency(bool latency) {
    double time=0;

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

            clock_gettime(CLOCK_REALTIME, &start_latency);
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
            clock_gettime(CLOCK_REALTIME, &end_latency);

            time += sec_since(&start_latency, &end_latency);
        }
        if (latency) {
            double latency_time = (time / number_test);
            if (sched_getscheduler(pid) == SCHED_OTHER) {
                printf("SCHED_OTHER Latency: %lf seconds\n", latency_time);
            } else if (sched_getscheduler(pid) == SCHED_FIFO) {
                printf("SCHED_FIFO Latency: %lf seconds\n", latency_time);
            } else if (sched_getscheduler(pid) == SCHED_RR) {
                printf("SCHED_RR Latency: %lf seconds\n", latency_time);
            }
        }
        else {
            double throughput = ((NRTHR + 1) / (time / number_test));
            if (sched_getscheduler(pid) == SCHED_OTHER) {
                printf("SCHED_OTHER Throughput: %lf threads/second\n", throughput);
            } else if (sched_getscheduler(pid) == SCHED_FIFO) {
                printf("SCHED_FIFO Throughput: %lf threads/second\n", throughput);
            } else if (sched_getscheduler(pid) == SCHED_RR) {
                printf("SCHED_RR Throughput: %lf threads/second\n", throughput);
            }

        }
    }

    free(trd);

}

void *work(void* param){
    params *params1 = (params*)param;
    struct timespec start, end;

    clock_gettime(CLOCK_REALTIME, &params1->end_wait);

    pthread_mutex_lock(&mtx);
    wait_time += sec_since(&params1->start_wait, &params1->end_wait);
    pthread_mutex_unlock(&mtx);

    int a = 0;
    clock_gettime(CLOCK_REALTIME, &start);

    for (int i = 0; i < params1->loops; ++i) {
        a++;
    }

    clock_gettime(CLOCK_REALTIME, &end);
    pthread_mutex_lock(&mtx);
    work_time += sec_since(&start, &end);
    pthread_mutex_unlock(&mtx);
    return NULL;
}
void *big_work(void* param){
    params *params1 = (params*)param;
    struct timespec start_big, end_big;
    int a = 0;


    clock_gettime(CLOCK_REALTIME, &start_big);

    for (int i = 0; i < params1->loops*2; ++i) {
        a++;
    }
    clock_gettime(CLOCK_REALTIME, &end_big);

    big_work_time += sec_since(&start_big, &end_big);
    return NULL;
}

static double sec_since(struct timespec *start, struct timespec *end) {
    double s, e;

    s = start->tv_sec * 1000000000.0 + start->tv_nsec;
    e =   end->tv_sec * 1000000000.0 +   end->tv_nsec;


    return (e - s)/1000000000.0;
}