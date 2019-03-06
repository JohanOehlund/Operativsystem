//
// Created by c15jod on 2017-10-09.
//

#ifndef DODOU2_CLIENT_H
#define DODOU2_CLIENT_H

#include "../Resources/socket.h"

int setupConnection(char **argv);

int connectCS(sock_init_struct *sis,char *clientname);

PDU_struct *create_GET_to_server(char* key);

PDU_struct *create_message_to_server(char *input);

PDU_struct *create_INSERT_to_server(char* key, char* data);

PDU_struct *create_DELETE_to_server(char* key);

PDU_struct *create_INIT_to_server();

void printWrongParams(char *progName);


#endif //DODOU2_CLIENT_H
