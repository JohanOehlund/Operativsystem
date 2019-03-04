//
// Created by c15jod on 2017-09-19.
//

#ifndef DODOU2_SERVER_H
#define DODOU2_SERVER_H

#define THREADS 10


#include "../Resources/socket.h"

typedef struct clientThreadInfo {
    int thread_num;
    int client_sock;

}clientThreadInfo;


void *acceptConnections(void *arg);

void *clientlistener(void *arg);

int setupListeningSocket(char **argv);

int *findFreeThread();

void printWrongParams(char *progName);




#endif //DODOU2_SERVER_H
