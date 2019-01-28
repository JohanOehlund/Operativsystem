#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include <syscall.h>

#define BILLION  1000000000.0;

//typedef struct sched_param sched_param;

void *work(void* param){
    pid_t pid = getpid();

    int a = 0;
    for (int i = 0; i < 1000000000; ++i) {
        a++;
    }
    return NULL;
}

int main() {
    int nrthr=20;

    struct timespec start, end;

    clock_gettime(CLOCK_REALTIME, &start);



    /*
    int policy = sched_getscheduler(getpid());
    printf("policy before: %d\n",policy);

    sched_param *param = malloc(sizeof(sched_param));

    param->sched_priority = sched_get_priority_max(SCHED_FIFO);

    int test = sched_setscheduler(pid, SCHED_FIFO, param);
    int policy2 = sched_getscheduler(pid);
    printf("policy after: %d\n",test);*/

    /*********************\
     * set up scheduling *
    \*********************/

    struct sched_param sched;
    pid_t pid = getpid();
    sched.sched_priority = sched_get_priority_min(SCHED_RR);

    if (sched_setscheduler(pid, SCHED_RR, &sched) < 0 )
        fprintf(stderr, "SETSCHEDULER failed - err = %s\n", strerror(errno));
    else
        printf("Priority set to \"%d\"\n", sched.sched_priority);

    printf("test: %d\n",sched_getscheduler(pid));
    pthread_t *trd=calloc((size_t)nrthr,sizeof(pthread_t));

    for (int i = 0; i < nrthr; ++i) {
        if(pthread_create(&trd[i], NULL, work, (void*)0)){
            perror("ERROR creating thread.");
            exit(EXIT_FAILURE);
        }
    }
    
    for (int j = 0; j < nrthr; ++j) {
        if(pthread_join(trd[j], NULL)){
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    clock_gettime(CLOCK_REALTIME, &end);

    // time_spent = end - start
    double time_spent = (end.tv_sec - start.tv_sec) +
                        (end.tv_nsec - start.tv_nsec) / BILLION;

    printf("Time elpased is %f seconds", time_spent);


    //exit(0);

    return 0;
}