//
// Created by c15jod on 2017-10-02.
//


#include "readPDU.h"

#ifndef DODOU2_SOCKET_H
#define DODOU2_SOCKET_H





int send_pdu(int sock,PDU_struct *pdu);


void timeout_handler (int signum);
int getFQDN(char *fqdn, size_t n);

void *receive_pdu(int sock);

int createsocket_client(sock_init_struct *si);
int createsocket_server(sock_init_struct *sis);




#endif //DODOU2_SOCKET_H
