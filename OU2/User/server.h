//
// Created by c15jod on 2017-09-19.
//

#ifndef DODOU2_SERVER_H
#define DODOU2_SERVER_H

#include "../Resources/socket.h"
#define SLEEP 8
#define THREADS 255

typedef struct clientThreadInfo {
    int thread_num;
    int client_sock;

}clientThreadInfo;

void *joinThreads(void *arg);

int *findFreeThread();

void *serverWrite(void* arg);

void *clientlistener(void *arg);

int setupListeningSocket(char **argv);

void *acceptConnections(void *arg);

void *nameserverREG(void * arg);

int setup_nameserver(char **argv);


void printWrongParams(char *progName);

void send_REG(int server_nameserver,char **argv);


PDU_struct *setupREGPDU(char **argv);

bool keep_alive(int server_nameserver,GEN_struct *gen);

PDU_struct *setupALIVEPDU(GEN_struct *gen);

void *createALIVEBuffer(uint16_t ID_num);


GEN_struct *clientJOIN(int client_sock);

void closeConnectedClient(int client_sock, GEN_struct *gen_struct, int cliC);

void createPARTICIPANTSServer(int client_socket);

int addMESSToList(MESS_struct *stru,JOIN_struct *join_struct);
void addPLEAVEToList(JOIN_struct *join_struct);
void addPJOINToList(JOIN_struct *join_struct);
void *sendToClients(void *arg);

PDU_struct *createServerMESS(char *mess);
void sendQUIT(int sock);
#endif //DODOU2_SERVER_H
