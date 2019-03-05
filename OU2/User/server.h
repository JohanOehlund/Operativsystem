//
// Created by c15jod on 2017-09-19.
//

#ifndef DODOU2_SERVER_H
#define DODOU2_SERVER_H

#define THREADS 10
#define NETLINK_USER 31

#include "../Resources/socket.h"


int sock_fd;
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh_user = NULL;
struct iovec iov;
struct msghdr msg;


typedef struct clientThreadInfo {
    int thread_num;
    int client_sock;

}clientThreadInfo;


void *acceptConnections(void *arg);

void *clientlistener(void *arg);

int setupListeningSocket(char **argv);

int *findFreeThread();

void printWrongParams(char *progName);

int setup_netlink();
void reset_netlink();



#endif //DODOU2_SERVER_H
