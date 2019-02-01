//
// Created by root on 1/31/19.
//


#ifndef OU1_SCHEDULE_TESTER_H
#define OU1_SCHEDULE_TESTER_H

#include <getopt.h>
#include <stdbool.h>
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

int NRTHR = 0;
int loops = 0;
int schedulers[3] = {SCHED_OTHER, SCHED_RR, SCHED_FIFO};

typedef struct {
    int loops;
}params;

params *params1;
void measure_throughput_or_latency(bool latency);
static unsigned long usec_since(struct timespec *start, struct timespec *end);
void *work(void* param);

#endif //OU1_SCHEDULE_TESTER_H
