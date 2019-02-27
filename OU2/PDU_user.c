//
// Created by hasse on 2/19/19.
//

#include "PDU_user.h"

PDU_kernel_struct *read_exactly_from_kernel(struct nlmsghdr *nlh){
    PDU_kernel_struct *pdu = calloc(1, sizeof(PDU_kernel_struct));

    data response_buffer = NLMSG_DATA(nlh);
    uint8_t error;
    uint16_t numbytes;

    memcpy(&pdu->error,response_buffer, 1);
    response_buffer+=1;
    memcpy(&pdu->data_bytes,response_buffer, 2);
    response_buffer+=2;
    pdu->data = malloc(pdu->data_bytes);
    memcpy(pdu->data, response_buffer, pdu->data_bytes);

    printf("Received message error: %u\n", pdu->error);
    printf("Received message numbytes: %u\n", pdu->data_bytes);
    printf("Received message data: %s\n", (char*)pdu->data);
    return pdu;
}

data PDU_to_buffer_user(uint8_t OP_code, data pdu){
    data response_buffer = NULL;
    switch (OP_code){
        case INIT:
            response_buffer = create_INIT_buffer(pdu);
        break;
        case INSERT:
            response_buffer = create_INSERT_buffer(pdu);
        break;
        default:
        fprintf(stderr, "Error creating buffer from PDU.\n");
        return NULL;
    }
    return response_buffer;
}

static data create_INIT_buffer(data pdu){
    INIT_struct* init_struct = (INIT_struct*) pdu;
    data response_buffer = calloc(1, INIT_HEADERSIZE);

    data head = response_buffer;
    memset(response_buffer, INIT, 1);

    return head;
}

static data create_INSERT_buffer(data pdu){
    INSERT_struct *insert_struct = (INSERT_struct*) pdu;
    data response_buffer = calloc(1, (insert_struct->data_bytes)+INSERT_HEADERSIZE);

    data head = response_buffer;
    memcpy(response_buffer, &insert_struct->OP_code, 1);
    response_buffer++;
    memcpy(response_buffer, &insert_struct->key, 2);
    response_buffer+=2;
    memcpy(response_buffer, &insert_struct->data_bytes, 2);
    response_buffer+=2;
    memcpy(response_buffer, insert_struct->data, (insert_struct->data_bytes));

    return head;
}
