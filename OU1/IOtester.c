//
// Created by root on 2/5/19.
//
#include "IOtester.h"

// if=/dev/zero of=/home/c15aen/Operativsystem/Operativsystem/OU1/test bs=1G count=1
// https://www.thomas-krenn.com/en/wiki/Linux_I/O_Performance_Tests_using_dd
// https://www.techrepublic.com/article/how-to-change-the-linux-io-scheduler-to-fit-your-needs/

int main(int argc, char** argv ) {

    FILE *fp;
    struct timespec time_start, time_end;
    fp = fopen("/home/c15aen/Operativsystem/Operativsystem/OU1/IOtester/IO.txt", "w+");
    /*if (fsync(fp->_fileno) < 0) {
        fprintf(stderr, "Fsync filaed - err = %s\n", strerror(errno));
        exit(EXIT_FAILURE);

    }*/

    clock_gettime(CLOCK_REALTIME, &time_start);
    for (int i = 0; i < 1000000000; ++i) {

        fprintf(fp, "This is testing for fprintf...\n");
    }
    clock_gettime(CLOCK_REALTIME, &time_end);

    double time = sec_since(&time_start, &time_end);



    printf("Time: %lf", time);

    fclose(fp);

}
static double sec_since(struct timespec *start, struct timespec *end) {
    double s, e;

    s = start->tv_sec * 1000000000.0 + start->tv_nsec;
    e =   end->tv_sec * 1000000000.0 +   end->tv_nsec;


    return (e - s)/1000000000.0;
}
