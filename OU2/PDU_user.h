//
// Created by hasse on 2/19/19.
//

#ifndef OU2_PDU_H
#define OU2_PDU_H

#include <stdint.h>
typedef void* data;
//Defines for client-server
#define INIT 11
#define INSERT 12
#define GET 13
#define DELETE 14



typedef struct GEN_struct {
    uint8_t OPCode;
    data created_struct;
}GEN_struct;

//Kernel space struct.
typedef struct PDU_kernel_struct {
    uint8_t error;
    uint16_t numbytes;
    data data;
}PDU_kernel_struct;

//User space structs...
typedef struct INIT_struct {
    uint8_t OPCode;
}INIT_struct;

typedef struct INSERT_struct {
    uint8_t OPCode;
    uint16_t key;
    uint16_t value_bytes;
    data value;

}INSERT_struct;

typedef struct GET_struct {
    uint8_t OPCode;
    uint16_t key;

}GET_struct;


typedef struct DELETE_struct {
    uint8_t OPCode;
    uint16_t key;

}DELETE_struct;

static data create_init_PDU(void* answer);

static data create_insert_PDU(void* answer);

static data create_get_PDU(void* answer);

static data create_delete_PDU(void* answer);

static void free_struct(GEN_struct *p);

static PDU_kernel_struct *pdu_kernel_creater(GEN_struct *p);

static data PDU_to_buffer_kernel(PDU_kernel_struct *pdu);





#endif //OU2_PDU_H
