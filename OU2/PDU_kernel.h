//
// Created by hasse on 2/19/19.
//

#ifndef OU2_PDU_H
#define OU2_PDU_H

#include <linux/types.h>
//#include <stdint.h>

//Defines for client-server
#define INIT 10
#define INSERT 11
#define GET 12
#define DELETE 13


typedef struct PDU_kernel_struct {
    uint8_t numbytes;
    void *pdu;
}PDU_kernel_struct;


typedef struct GEN_struct {
    uint8_t OPCode;
    char* test;
    void *created_struct;

}GEN_struct;


typedef struct INIT_struct {
    uint8_t OPCode;

}INIT_struct;

typedef struct INSERT_struct {
    uint8_t OPCode;
    uint8_t servername_len;
    void *key;
    void *value;

}INSERT_struct;

typedef struct GET_struct {
    uint8_t OPCode;
    uint8_t servername_len;
    uint16_t tcp_port;
    char *servername;

}GET_struct;


typedef struct DELETE_struct {
    uint8_t OPCode;
    uint8_t servername_len;
    uint16_t tcp_port;
    char *servername;

}DELETE_struct;

static void *create_init_PDU(void* answer);

static void *create_insert_PDU(void* answer);

static void *create_get_PDU(void* answer);

static void *create_delete_PDU(void* answer);

static void free_struct(GEN_struct *p);

static PDU_kernel_struct *pdu_kernel_creater(GEN_struct *p);





#endif //OU2_PDU_H
