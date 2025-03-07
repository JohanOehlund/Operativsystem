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
        case JOIN:
            PDU_struct = read_JOIN(sock);
            break;
        case QUIT:
            PDU_struct = read_QUIT(sock);
            break;
        default:
            fprintf(stderr,"INVALID OP-code read_exactly: %u\n", OP_code);
            return NULL;
    }
    return PDU_struct;
}

PDU_struct *read_QUIT(int sock){
    size_t nread = 0;


    data header = calloc(1, HEADERSIZE);
    while(nread<HEADERSIZE){
        nread=recv(sock,header,HEADERSIZE,0);
        if(nread==-1) {
            continue;
        }
    }

    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct) + sizeof(data));
    QUIT_struct *quit_struct = calloc(1, sizeof(QUIT_struct));


    quit_struct->OP_code = QUIT;
    quit_struct->sock = sock;

    PDU_struct->OP_code = QUIT;
    PDU_struct->data = quit_struct;
    free(header);
    return PDU_struct;

}

PDU_struct *read_JOIN(int sock){
    size_t nread = 0;


    data header = calloc(1, HEADERSIZE);
    while(nread<HEADERSIZE){
        nread=recv(sock,header,HEADERSIZE,0);
        if(nread==-1) {
            continue;
        }
    }

    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct) + sizeof(data));
    JOIN_struct *join_struct = calloc(1, sizeof(JOIN_struct));


    uint16_t name_len;
    header++;
    memcpy(&name_len, header, 2);
    PDU_struct->data_bytes = name_len;
    join_struct->client_ID = calloc(1, PDU_struct->data_bytes+1);

    nread = 0;
    while(nread < PDU_struct->data_bytes){
        nread=recv(sock,join_struct->client_ID,PDU_struct->data_bytes,0);
        if(nread==-1) {
            continue;
        }
    }

    join_struct->OP_code = JOIN;
    join_struct->ID_len = name_len;

    PDU_struct->OP_code = JOIN;
    PDU_struct->data = join_struct;
    header--;
    free(header);
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
    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct) + sizeof(data));

    uint16_t msg_size = 0;
    header++;
    memcpy(&msg_size, header, 2);
    PDU_struct->data = calloc(1,msg_size + sizeof(data));
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
    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct) + sizeof(data));


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

    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct) + sizeof(data));
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

    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct) + sizeof(data));
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

    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct) + sizeof(data));
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
    PDU_struct *pdu = calloc(1, sizeof(PDU_struct) + sizeof(data));

    data response_buffer = NLMSG_DATA(nlh);
    data head = response_buffer;
    uint16_t msg_size = 0;
    data message = NULL;

    //memcpy(&pdu->OP_code,response_buffer, 1);
    response_buffer+=1;
    memcpy(&msg_size,response_buffer, 2);
    response_buffer+=3;
    pdu->data = calloc(1, msg_size + HEADERSIZE);
    memcpy(pdu->data, head, msg_size + HEADERSIZE);
    pdu->data_bytes = msg_size + HEADERSIZE;
    pdu->OP_code=KERNEL;
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
    memcpy(response_buffer, delete_struct->key, strlen(delete_struct->key));

    return head;
}

data create_GET_buffer(data pdu){
    GET_struct* get_struct = (GET_struct*) pdu;
    data response_buffer = calloc(1, HEADERSIZE + KEY_SIZE);

    data head = response_buffer;

    memset(response_buffer, GET, 1);
    response_buffer+=HEADERSIZE;
    memcpy(response_buffer, get_struct->key, strlen(get_struct->key));

    return head;
}

data create_INIT_buffer(data pdu){
    data response_buffer = calloc(1, HEADERSIZE);

    memset(response_buffer, INIT, 1);
    return response_buffer;
}

data create_INSERT_buffer(data pdu){
    INSERT_struct *insert_struct = (INSERT_struct*) pdu;
    size_t response_buffer_size = (insert_struct->data_bytes)+HEADERSIZE + KEY_SIZE;

    data response_buffer = calloc(1, response_buffer_size);
    //memset(response_buffer,'\0',response_buffer_size);
    data head = response_buffer;
    memcpy(response_buffer, &insert_struct->OP_code, 1);
    response_buffer++;
    memcpy(response_buffer, &insert_struct->data_bytes, 2);
    response_buffer+=3;
    memcpy(response_buffer, &insert_struct->key, strlen(insert_struct->key));
    response_buffer+=KEY_SIZE;
    memcpy(response_buffer, insert_struct->data, (insert_struct->data_bytes));
    //printf("insert_struct->key %s\n", (char*)insert_struct->key);

    return head;
}

PDU_struct *create_QUIT_pdu() {
    printf("QUIT\n");
    PDU_struct *PDU_struct = calloc(1,sizeof(PDU_struct)+sizeof(data));
    PDU_struct->OP_code = USER;
    PDU_struct->data_bytes = HEADERSIZE;

    data data = calloc(1,HEADERSIZE);
    memset(data , QUIT, 1);
    PDU_struct->data = data;
    return PDU_struct;
}


/* Free a struct.
 * @param   p -  The generic struct that will be free'd.
 * @return
 */
void free_struct(uint8_t OP_code, data free_struct){

    if(OP_code == INIT){
        INIT_struct *temp_struct = (INIT_struct*) free_struct;
        free(temp_struct);
    }else if(OP_code == INSERT){
        INSERT_struct *temp_struct = (INSERT_struct*) free_struct;
        free(temp_struct->data);
        free(temp_struct);
    }else if(OP_code == GET){
        GET_struct *temp_struct = (GET_struct*) free_struct;
        free(temp_struct);
    }else if(OP_code == DELETE){
        DELETE_struct *temp_struct = (DELETE_struct*) free_struct;
        free(temp_struct);
    }else if(OP_code == KERNEL || OP_code == USER || OP_code == QUIT){
        PDU_struct *temp_struct = (PDU_struct*) free_struct;
        free(temp_struct->data);
        free(temp_struct);
    }else if(OP_code == JOIN){
        PDU_struct *temp_struct = (PDU_struct*) free_struct;
        JOIN_struct *temp_join = temp_struct->data;
        free(temp_join->client_ID);
        free(temp_join);
        free(temp_struct);

    }else {
        fprintf(stderr, "Invalid OP_code in free_struct.\n");

    }

}

void print_test_time(){
    test_time = sec_since(&time_start, &time_end);
    printf("\n-------------------------\n");
    printf("Test took %lf s", test_time);
    printf("\n-------------------------\n");
}


double sec_since(struct timespec *start, struct timespec *end) {
    double s, e;

    s = start->tv_sec * 1000000000.0 + start->tv_nsec;
    e =   end->tv_sec * 1000000000.0 +   end->tv_nsec;


    return (e - s)/1000000000.0;
}
