//
// Created by hasse on 2/19/19.
//

#include "PDU_user.h"

PDU_struct *read_exactly(int sock, uint8_t OP_code){
    PDU_struct *PDU_struct;

    switch (OP_code){
        case INIT:
            PDU_struct = read_INIT(sock);
            break;
        case INSERT:
            PDU_struct = read_INSERT(sock);
            break;
        case GET:
            PDU_struct = read_GET(sock);
            break;
        case DELETE:
            PDU_struct = read_DELETE(sock);
            break;
        case KERNEL:
            PDU_struct = read_KERNEL(sock);
            break;
        default:
            fprintf(stderr,"INVALID OP-code read_exactly: %u\n", OP_code);
            return NULL;
    }
    return PDU_struct;
}

PDU_struct *read_KERNEL(int sock) {
    size_t nread = 0;

    data header=calloc(1,HEADERSIZE);
    while(nread<HEADERSIZE){
        nread=recv(sock,header,HEADERSIZE,0);
        if(nread==-1) {
            continue;
        }
    }
    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct));

    uint16_t msg_size = 0;
    header++;
    memcpy(&msg_size, header, 2);
    printf("msg_size: %u\n", msg_size);
    PDU_struct->data = calloc(1,msg_size);
    nread = 0;
    while(nread<msg_size){
        nread=recv(sock,PDU_struct->data,msg_size,0);
        if(nread==-1) {
            continue;
        }
    }

    PDU_struct->OP_code = KERNEL;
    PDU_struct->data_bytes = msg_size;
    header--;
    free(header);
    return PDU_struct;
}

PDU_struct *read_INIT(int sock) {
    size_t nread = 0;

    data header=calloc(1,HEADERSIZE);
    while(nread<HEADERSIZE){
        nread=recv(sock,header,HEADERSIZE,0);
        if(nread==-1) {
            continue;
        }
    }
    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct));


    PDU_struct->OP_code = INIT;
    PDU_struct->data_bytes = HEADERSIZE;
    PDU_struct->data = calloc(1, HEADERSIZE);
    memcpy(PDU_struct->data, header, HEADERSIZE);

    free(header);
    return PDU_struct;

}
PDU_struct *read_INSERT(int sock) {
    size_t nread = 0;


    data header = calloc(1, HEADERSIZE);
    while(nread<HEADERSIZE){
        nread=recv(sock,header,HEADERSIZE,MSG_PEEK);
        if(nread==-1) {
            continue;
        }
    }

    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct));
    PDU_struct->OP_code = INSERT;
    uint16_t mess_len;
    header++;
    memcpy(&mess_len, header, 2);
    PDU_struct->data_bytes = mess_len + HEADERSIZE + KEY_SIZE;

    data pdu = calloc(1, PDU_struct->data_bytes);

    nread = 0;
    while(nread<PDU_struct->data_bytes){
        nread=recv(sock,pdu,PDU_struct->data_bytes,0);
        if(nread==-1) {
            continue;
        }
    }

    PDU_struct->data = pdu;
    header--;
    free(header);
    return PDU_struct;

}
PDU_struct *read_GET(int sock) {
    size_t nread = 0;

    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct));
    PDU_struct->OP_code = GET;
    PDU_struct->data_bytes = HEADERSIZE + KEY_SIZE;

    data pdu=calloc(1,PDU_struct->data_bytes);
    while(nread<PDU_struct->data_bytes){
        nread=recv(sock,pdu,PDU_struct->data_bytes,0);
        if(nread==-1) {
            continue;
        }
    }
    PDU_struct->data = pdu;

    return PDU_struct;

}
PDU_struct *read_DELETE(int sock) {
    size_t nread = 0;

    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct));
    PDU_struct->OP_code = DELETE;
    PDU_struct->data_bytes = HEADERSIZE + KEY_SIZE;

    data pdu=calloc(1,PDU_struct->data_bytes);
    while(nread<PDU_struct->data_bytes){
        nread=recv(sock,pdu,PDU_struct->data_bytes,0);
        if(nread==-1) {
            continue;
        }
    }
    PDU_struct->data = pdu;

    return PDU_struct;
}

PDU_struct *read_exactly_from_kernel(struct nlmsghdr *nlh){
    PDU_struct *pdu = calloc(1, sizeof(PDU_struct));

    data response_buffer = NLMSG_DATA(nlh);
    data head = response_buffer;
    uint16_t msg_size = 0;
    data message = NULL;

    memcpy(&pdu->OP_code,response_buffer, 1);
    response_buffer+=1;
    memcpy(&msg_size,response_buffer, 2);
    response_buffer+=3;
    pdu->data = calloc(1, msg_size + HEADERSIZE);
    memcpy(pdu->data, head, msg_size + HEADERSIZE);
    pdu->data_bytes = msg_size + HEADERSIZE;
    //printf("Received message opcode: %u\n", pdu->OP_code);
    //printf("Received message data_bytes: %u\n", pdu->data_bytes);
    //printf("Received message data: %s\n", (char*)pdu->data);
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

        case GET:
            response_buffer = create_GET_buffer(pdu);
        break;

        case DELETE:
            response_buffer = create_DELETE_buffer(pdu);
        break;

        default:
            fprintf(stderr, "Error creating buffer from PDU.\n");
            return NULL;
    }
    return response_buffer;
}

data create_DELETE_buffer(data pdu){
    DELETE_struct* delete_struct = (DELETE_struct*) pdu;
    data response_buffer = calloc(1, HEADERSIZE+KEY_SIZE);

    data head = response_buffer;

    memset(response_buffer, DELETE, 1);
    response_buffer+=HEADERSIZE;
    memcpy(response_buffer, delete_struct->key, KEY_SIZE);

    return head;
}

data create_GET_buffer(data pdu){
    GET_struct* get_struct = (GET_struct*) pdu;
    data response_buffer = calloc(1, HEADERSIZE + KEY_SIZE);

    data head = response_buffer;

    memset(response_buffer, GET, 1);
    response_buffer+=HEADERSIZE;
    memcpy(response_buffer, get_struct->key, KEY_SIZE);

    return head;
}

data create_INIT_buffer(data pdu){
    data response_buffer = calloc(1, HEADERSIZE);

    data head = response_buffer;
    memset(response_buffer, INIT, 1);

    return head;
}

data create_INSERT_buffer(data pdu){
    INSERT_struct *insert_struct = (INSERT_struct*) pdu;
    size_t response_buffer_size = (insert_struct->data_bytes)+HEADERSIZE + KEY_SIZE;
    data response_buffer = calloc(1, response_buffer_size);

    data head = response_buffer;
    memcpy(response_buffer, &insert_struct->OP_code, 1);
    response_buffer++;
    memcpy(response_buffer, &insert_struct->data_bytes, 2);
    response_buffer+=3;
    memcpy(response_buffer, &insert_struct->key, KEY_SIZE);
    response_buffer+=KEY_SIZE;
    memcpy(response_buffer, insert_struct->data, (insert_struct->data_bytes));
    //printf("insert_struct->key %s\n", (char*)insert_struct->key);
    return head;
}
