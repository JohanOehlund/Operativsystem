#include "schedule_tester.h"



//https://stackoverflow.com/questions/33991918/link-to-pthread-library-using-cmake-in-clion


//typedef struct sched_param sched_param;



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
        printf("6. End\n");


        printf("\nGive your choice (1 - 6): ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                measure_throughput();

                break;
            case 6:
                printf("Closing program");

                break;
            default:
                fprintf(stdout, "BAD INPUT!\n");

        }



    }while (choice != 6);

}

void measure_throughput() {
    struct timespec start, end;
    struct sched_param sched;

    pid_t pid = getpid();
    sched.sched_priority = sched_get_priority_max(SCHED_OTHER);

    if (sched_setscheduler(pid, SCHED_OTHER, &sched) < 0)
        fprintf(stderr, "SETSCHEDULER failed - err = %s\n", strerror(errno));
    else
        printf("Priority set to \"%d\"\n", sched.sched_priority);

    printf("test: %d\n", sched_getscheduler(pid));
    pthread_t *trd = calloc((size_t) NRTHR, sizeof(pthread_t));

    clock_gettime(CLOCK_REALTIME, &start);

    for (int i = 0; i < NRTHR; ++i) {
        if (pthread_create(&trd[i], NULL, work, (void *) params1)) {
            perror("ERROR creating thread.");
            exit(EXIT_FAILURE);
        }
    }
    work((void *) params1);

    for (int j = 0; j < NRTHR; ++j) {
        if (pthread_join(trd[j], NULL)) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    clock_gettime(CLOCK_REALTIME, &end);

    usec_since(&start, &end);


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

    printf("Time elapsed: %lu microcseconds\n", time);

    return time;
}