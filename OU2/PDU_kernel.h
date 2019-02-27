//
// Created by hasse on 2/19/19.
//

#ifndef OU2_PDU_H
#define OU2_PDU_H

#include <linux/types.h>

typedef void* data;
//Defines for client-server
#define INIT 11
#define INSERT 12
#define GET 13
#define DELETE 14

#define MAX_PAYLOAD 1024 /* maximum payload size*/


#define KERNEL_HEADERSIZE 3
#define INIT_HEADERSIZE 1
#define INSERT_HEADERSIZE 5
#define GET_HEADERSIZE 3
#define DELETE_HEADERSIZE 3


typedef struct GEN_struct {
    uint8_t OP_code;
    data created_struct;
}GEN_struct;

//Kernel space struct.
typedef struct PDU_kernel_struct {
    uint8_t error;
    uint16_t data_bytes;
    data data;
}PDU_kernel_struct;

//User space structs...
typedef struct INIT_struct {
    uint8_t OP_code;
}INIT_struct;

typedef struct INSERT_struct {
    uint8_t OP_code;
    uint16_t key;
    uint16_t data_bytes;
    data data;

}INSERT_struct;

typedef struct GET_struct {
    uint8_t OP_code;
    uint16_t key;

}GET_struct;


typedef struct DELETE_struct {
    uint8_t OP_code;
    uint16_t key;

}DELETE_struct;






#endif //OU2_PDU_H
