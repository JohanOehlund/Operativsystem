//
// Created by c15aen on 2019-02-07.
//

#include <bits/time.h>
#include <time.h>
#include <stdio.h>

static double sec_since(struct timespec *start, struct timespec *end) {
    double s, e;

    s = start->tv_sec * 1000000000.0 + start->tv_nsec;
    e =   end->tv_sec * 1000000000.0 +   end->tv_nsec;


    return (e - s)/1000000000.0;
}

int main(int argc, char** argv ) {

    struct timespec start, end;

    unsigned int a = 0;
    clock_gettime(CLOCK_REALTIME, &start);
    for (unsigned int i = 0; i < 4000000000; ++i) {
        a++;
    }
    clock_gettime(CLOCK_REALTIME, &end);

    double j = sec_since(&start, &end);

    printf("Time: %lf", j);
}

