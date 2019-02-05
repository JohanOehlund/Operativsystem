//
// Created by root on 2/5/19.
//

#ifndef OU1_IOTESTER_H
#define OU1_IOTESTER_H

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
#include <zconf.h>
#include <bits/time.h>

static double sec_since(struct timespec *start, struct timespec *end);

#endif //OU1_IOTESTER_H
