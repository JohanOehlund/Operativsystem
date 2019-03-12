//
// Created by hasse on 2/19/19.
//

#ifndef OU2_PDU_H
#define OU2_PDU_H
typedef void* data;

#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include <stdint.h>
#include <ctype.h>
#include <regex.h>
#include "list.h"

#define EXIT "exit"

#define TEST_DATA ("Jag heter HASSE!!!")

#define MAX_PAYLOAD 1024 /* maximum payload size*/

//Defines for client-server
#define INIT 11
#define INSERT 12
#define GET 13
#define DELETE 14
#define KERNEL 15
#define USER 16

#define JOIN 17
#define QUIT 18

#define HEADERSIZE 4
#define KEY_SIZE 64
#define MAXMESSLEN 50

//Kernel space struct.
/*typedef struct PDU_kernel_struct {
    uint8_t error;
    uint16_t data_bytes;
    data data;
}PDU_kernel_struct;*/

//User space structs...
typedef struct INIT_struct {
    uint8_t OP_code;
}INIT_struct;

typedef struct INSERT_struct {
    uint8_t OP_code;
    char key[KEY_SIZE];
    uint16_t data_bytes;
    data data;

}INSERT_struct;

typedef struct GET_struct {
    uint8_t OP_code;
    char key[KEY_SIZE];

}GET_struct;


typedef struct DELETE_struct {
    uint8_t OP_code;
    char key[KEY_SIZE];

}DELETE_struct;

typedef struct JOIN_struct {
    uint8_t OP_code;
    uint16_t ID_len;
    char *client_ID;
}JOIN_struct;

typedef struct QUIT_struct {
    uint8_t OP_code;
    uint16_t sock;
}QUIT_struct;


PDU_struct *read_exactly(int sock, uint8_t OP_code);

PDU_struct *read_exactly_from_kernel(struct nlmsghdr *nlh);

data PDU_to_buffer_user(uint8_t OP_code, data pdu);

PDU_struct *create_QUIT_pdu();

data create_DELETE_buffer(data pdu);

data create_GET_buffer(data pdu);

data create_GET_buffer(data pdu);

data create_INIT_buffer(data pdu);

data create_INSERT_buffer(data pdu);

PDU_struct *read_QUIT(int sock);

PDU_struct *read_JOIN(int sock);

PDU_struct *read_DELETE(int sock);

PDU_struct *read_GET(int sock);

PDU_struct *read_INIT(int sock);

PDU_struct *read_INSERT(int sock);

PDU_struct *read_KERNEL(int sock);

void free_struct(uint8_t OP_code, data free_struct);



#endif //OU2_PDU_H
