//
// Created by c15jod on 2017-10-02.
//


//#include "readPDU.h"

#ifndef DODOU2_SOCKET_H
#define DODOU2_SOCKET_H

#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <memory.h>
#include <time.h>
#include <inttypes.h> /* strtoimax */
#include <errno.h>
#include <string.h>
#include "PDU_user.h"

typedef struct sock_init_struct {
    bool isUDP;
    char *nexthost;
    char *nextportString;
}sock_init_struct;




int send_pdu(int sock,PDU_struct *pdu);


void timeout_handler (int signum);
int getFQDN(char *fqdn, size_t n);

PDU_struct *receive_pdu(int sock);

int createsocket_client(sock_init_struct *si);
int createsocket_server(sock_init_struct *sis);




#endif //DODOU2_SOCKET_H
