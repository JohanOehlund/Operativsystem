//
// Created by c15jod on 2017-10-09.
//

#ifndef DODOU2_CLIENT_H
#define DODOU2_CLIENT_H

#include "../Resources/socket.h"
#define EXIT "exit"
#define MAXMESSLEN 1024

int setupConnection(char **argv);

int connectCS(sock_init_struct *sis,char *clientname);

PDU_struct *setupJOINPDU(char *clientname);

void printWrongParams(char *progName);


#endif //DODOU2_CLIENT_H
