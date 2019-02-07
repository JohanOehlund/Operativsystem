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
int schedulers[3] = {SCHED_OTHER, SCHED_RR, SCHED_FIFO};
double big_work_time = 0.0;
double work_time = 0.0;
double wait_time = 0.0;
struct sched_param sched;
struct timespec start_latency, end_latency;
int number_schedulers = 3;
int number_test = 5;
pthread_mutex_t mtx;
pthread_cond_t cond;


typedef struct {
    int loops;
    struct timespec start_wait, end_wait;
}params;

params *params1;
void measure_throughput_or_latency(bool latency);
static double sec_since(struct timespec *start, struct timespec *end);
void *work(void* param);
void *big_work(void* param);
void measure_tail_latency();
void measure_waiting_time();
void change_threads_and_loops();


#endif //OU1_SCHEDULE_TESTER_H
