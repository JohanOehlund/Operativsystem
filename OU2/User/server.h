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

void closeConnectedClient(int client_sock, int cliC);


void *sendToClients(void *arg);

PDU_struct *clientJOIN(int client_sock);










#define NETLINK_USER 31

int sock_fd;
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh_user = NULL;
struct iovec iov;
struct msghdr msg;


size_t strnlen(const char *s, size_t maxlen);

void *test_rhashtable(data arg);
void delete_rhashtable(char* key);
void get_rhashtable(char* key);
void init_rhashtable();
void insert_rhashtable(char* key);


int setup_netlink();
void reset_netlink();


#endif //DODOU2_SERVER_H
