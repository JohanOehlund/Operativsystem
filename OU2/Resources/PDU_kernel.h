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
#define KERNEL 15

#define MAX_PAYLOAD 1024 /* maximum payload size*/
#define KEY_SIZE 64


typedef struct PDU_struct {
	u8 OP_code;
	u16 data_bytes;
	data data;
}PDU_struct;

//User space structs...
typedef struct INIT_struct {
    u8 OP_code;
}INIT_struct;

typedef struct INSERT_struct {
    u8 OP_code;
    char key[KEY_SIZE];
    u16 data_bytes;
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
