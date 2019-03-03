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
#define KEY_SIZE 64

#define KERNEL_HEADERSIZE 5


//Kernel space struct.
typedef struct PDU_kernel_struct {
    u8 error;
    u32 data_bytes;
    data data;
}PDU_kernel_struct;

//User space structs...
typedef struct INIT_struct {
    u8 OP_code;
}INIT_struct;

typedef struct INSERT_struct {
    u8 OP_code;
    char key[KEY_SIZE];
    u32 data_bytes;
    data data;

}INSERT_struct;

typedef struct GET_struct {
    u8 OP_code;
    char key[KEY_SIZE];
}GET_struct;


typedef struct DELETE_struct {
    u8 OP_code;
    char key[KEY_SIZE];

}DELETE_struct;






#endif //OU2_PDU_H
