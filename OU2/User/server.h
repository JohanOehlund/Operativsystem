//
// Created by c15jod on 2017-09-19.
//

#ifndef DODOU2_SERVER_H
#define DODOU2_SERVER_H
#include "../Resources/socket.h"

#define THREADS 10
#define NETLINK_USER_SEND 30
#define NETLINK_USER_RECEIVE 31

int sock_fd_send;
struct nlmsghdr *nlh_user_send = NULL;
struct sockaddr_nl src_addr_send, dest_addr_send;
struct iovec iov_send;
struct msghdr msg_send;

int sock_fd_receive;
struct nlmsghdr *nlh_user_receive = NULL;
struct sockaddr_nl src_addr_receive, dest_addr_receive;
struct iovec iov_receive;
struct msghdr msg_receive;



typedef struct clientThreadInfo {
    int thread_num;
    int client_sock;

}clientThreadInfo;


void *acceptConnections(void *arg);

void *clientlistener(void *arg);

int setupListeningSocket(char **argv);

int *findFreeThread();

void printWrongParams(char *progName);

void closeConnectedClient(int client_sock, int cliC);


void *sendToClients(void *arg);

PDU_struct *clientJOIN(int client_sock);

void closeServer();

void *serverWrite(void *arg);

void *joinThreads(void *arg);



size_t strnlen(const char *s, size_t maxlen);

int setup_netlink_send();
void reset_netlink_send();

int setup_netlink_receive();
void reset_netlink_receive();

#endif //DODOU2_SERVER_H
