//
// Created by c15jod on 2017-10-09.
//
#include "client.h"
pthread_mutex_t lock;
char* connectedServer;
int main(int argc, char **argv){

    if (argc != 5) {
        printWrongParams(argv[0]);
        return EXIT_FAILURE;
    }

    if (pthread_mutex_init(&lock, NULL) != 0){
        fprintf(stderr,"\n mutex init failed\n");
        return 1;
    }
    char message[MAXMESSLEN+1]={0};
    int client_server;
    int choice;
    char key[MAXMESSLEN+1];
    char data[MAXMESSLEN+1];
    //pthread_t trd[1];

    if((client_server = setupConnection(argv))==-1){
        printf("Exit program.\n");
        exit(EXIT_FAILURE);
    }
    PDU_struct *PDU_struct_send = NULL;
    PDU_struct *PDU_struct_recieve = NULL;

    do {

        printf("\n1. Init hashtable\n");
        printf("2. Insert to hashtable\n");
        printf("3. Get from hashtable\n");
        printf("4. Delete from hashtable\n");
        printf("5. Exit program\n");
        printf("6. TEST_INT_INSERT()\n");
        printf("\nGive your choice (1 - 5): ");
        scanf("%d", &choice);


        switch (choice) {
            case 1:
                PDU_struct_send = create_INIT_to_server();
                break;
            case 2:
                printf("\nEnter key: ");
                scanf("%s", key);
                printf("\nEnter value: ");
                scanf("%s", data);
                PDU_struct_send = create_INSERT_to_server(key, data, strnlen(data, MAXMESSLEN)+1);
                break;
            case 3:
                printf("\nEnter key: ");
                scanf("%s", key);
                PDU_struct_send = create_GET_to_server(key);
                break;
            case 4:
                printf("\nEnter key: ");
                scanf("%s", key);
                PDU_struct_send = create_DELETE_to_server(key);
                break;
            case 5:
                printf("Closing program\n");
                PDU_struct_send = create_QUIT_to_server();
                send_pdu(client_server, PDU_struct_send);
                free_struct(USER,PDU_struct_send);
                return 0;
                break;
            case 6:
                TEST_INT_INSERT(client_server);
                continue;
            default:
                fprintf(stdout, "BAD INPUT!\n");

        }

        send_pdu(client_server, PDU_struct_send);

        /*PDU_struct = create_message_to_server(message);
        send_pdu(client_server, PDU_struct);
        free(PDU_struct->data);
        free(PDU_struct);*/
        PDU_struct_recieve = receive_pdu(client_server);
        //printf("OP i return: %u\n", PDU_struct_recieve->OP_code);
        //printf("data_bytes i return: %u\n", PDU_struct_recieve->data_bytes);
        printf("\nDATA FROM KERNEL: %s\n", (char*)PDU_struct_recieve->data);


        free_struct(USER,PDU_struct_send);
        free_struct(KERNEL,PDU_struct_recieve);
    }while (choice != 5);

    return 0;
}

void TEST_INT_INSERT(int sock){

    //char buffer[4];

    uint32_t test_data = 1024;
    char* test_key;
    int i = 0;
    for(i = 0; i < 2; i++){
        //sprintf(buffer, "%d", test);
        if(i==0){
            test_key="key1";
        }else{
            test_key="key2";
        }
        PDU_struct *PDU_struct_send = NULL;
        PDU_struct *PDU_struct_recieve = NULL;

        uint16_t size = 4;
        data data = calloc(1,size);

        memcpy(data,&test_data,size);
        PDU_struct_send = create_INSERT_to_server(test_key, data, size);

        send_pdu(sock, PDU_struct_send);

        PDU_struct_recieve = receive_pdu(sock);
        //uint16_t res_data = 0;
        //memcpy(&res_data, PDU_struct_recieve->data, 2);
        printf("\n DATA INSERT INT: %s\n", (char*)PDU_struct_recieve->data);

        free_struct(USER,PDU_struct_send);
        free_struct(KERNEL,PDU_struct_recieve);

        PDU_struct *PDU_struct_send2 = NULL;
        PDU_struct *PDU_struct_recieve2 = NULL;

        PDU_struct_send2 = create_GET_to_server(test_key);

        send_pdu(sock, PDU_struct_send2);

        PDU_struct_recieve2 = receive_pdu(sock);
        uint16_t res_data = 0;
        printf(" PDU_struct_recieve->data_bytes: %u\n",  PDU_struct_recieve2->data_bytes);
        memcpy(&res_data, PDU_struct_recieve2->data, size);
        printf("\n DATA INSERT INT (%u?): %u\n", test_data, res_data);

        free(data);
        free_struct(USER,PDU_struct_send2);
        free_struct(KERNEL,PDU_struct_recieve2);
    }

}

PDU_struct *create_QUIT_to_server() {
    printf("QUIT\n");
    PDU_struct *PDU_struct = calloc(1,sizeof(PDU_struct)+sizeof(data));
    PDU_struct->OP_code = USER;
    PDU_struct->data_bytes = HEADERSIZE;

    data data = calloc(1,HEADERSIZE);
    memset(data , QUIT, 1);
    PDU_struct->data = data;
    return PDU_struct;
}

PDU_struct *create_INIT_to_server(){
    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct)+sizeof(data));

    INIT_struct* init_struct = calloc(1, sizeof(INIT_struct));
    init_struct->OP_code = USER;

    data buffer = PDU_to_buffer_user(INIT, init_struct);

    PDU_struct->OP_code = USER;
    PDU_struct->data_bytes = HEADERSIZE;
    PDU_struct->data = buffer;

    free_struct(INIT, init_struct);
    return PDU_struct;
}

PDU_struct *create_INSERT_to_server(char* key, data data, uint16_t data_bytes){

    PDU_struct *PDU_struct = calloc(1,sizeof(PDU_struct)+sizeof(data));

    INSERT_struct *insert_struct = calloc(1,sizeof(INSERT_struct));
    insert_struct->OP_code=INSERT;
    memcpy(insert_struct->key, key, KEY_SIZE);
    insert_struct->data_bytes = data_bytes;
    insert_struct->data = calloc(1,data_bytes);

    memcpy(insert_struct->data, data, (insert_struct->data_bytes));

    void *action = PDU_to_buffer_user(INSERT, insert_struct); // 'action' kan inte vara data måste vara void*....

    PDU_struct->OP_code = USER;
    PDU_struct->data_bytes = (insert_struct->data_bytes) + HEADERSIZE + KEY_SIZE;
    PDU_struct->data = action;

    free_struct(INSERT, insert_struct);
    return PDU_struct;
}

PDU_struct *create_GET_to_server(char* key) {

    PDU_struct *PDU_struct = calloc(1,sizeof(PDU_struct)+sizeof(data));

    GET_struct *get_struct = calloc(1,sizeof(GET_struct));

    get_struct->OP_code = GET;
    memcpy(get_struct->key, key, KEY_SIZE);

    data action = PDU_to_buffer_user(GET, get_struct);
    PDU_struct->OP_code = USER;
    PDU_struct->data_bytes = HEADERSIZE + KEY_SIZE;
    PDU_struct->data = action;

    free_struct(GET, get_struct);
    return PDU_struct;
}

PDU_struct *create_DELETE_to_server(char* key) {
    PDU_struct *PDU_struct = calloc(1,sizeof(PDU_struct)+sizeof(data));

    DELETE_struct *delete_struct = calloc(1,sizeof(DELETE_struct));

    delete_struct->OP_code = DELETE;
    memcpy(delete_struct->key, key, KEY_SIZE);

    data action = PDU_to_buffer_user(DELETE, delete_struct);
    PDU_struct->OP_code = USER;
    PDU_struct->data_bytes = HEADERSIZE + KEY_SIZE;
    PDU_struct->data = action;

    free_struct(DELETE, delete_struct);
    return PDU_struct;
}

/* Connects direct to a server without using the nameserver.
* @param   sis -  A struct with information to create a socket.
* @param   clientname -  The clients name (input argument to program).
* @return pdu - The created messagePDU.
*/
int connectCS(sock_init_struct *sis, char *clientname){

    PDU_struct *pdu=setupJOINPDU(clientname);
    int server_client=createsocket_client(sis);


    if((send_pdu(server_client,pdu)==-1)){
        fprintf(stderr,"Could not send to server\n");
        free(pdu->data);
        free(pdu);
        return -1;
    }
    free(pdu->data);
    free(pdu);
    return server_client;
}




/* Creates a JOIN-PDU to server.
 * @param    clientname -  The clients name (input argument to program).
 * @return pdu - The created JOIN-PDU.
 */
PDU_struct *setupJOINPDU(char *clientname){

    PDU_struct *PDU_struct = calloc(1,sizeof(PDU_struct)+sizeof(data));
    PDU_struct->OP_code = USER;
    PDU_struct->data_bytes = HEADERSIZE + strnlen(clientname,MAXMESSLEN)+1;

    uint16_t client_len = (uint16_t)strnlen(clientname,MAXMESSLEN)+1;
    printf("client_len %u\n", client_len);
    printf("client name: %s\n", clientname);
    size_t data_size = HEADERSIZE + strnlen(clientname,MAXMESSLEN)+1;

    data data = calloc(1,data_size);
    memset(data , JOIN, 1);
    data++;
    memcpy(data , &client_len, 2);
    data+=3;
    memcpy(data , clientname, client_len);
    data-=4;
    PDU_struct->data = data;
    return PDU_struct;
}



/* Setup a connection based on input argument (cs/ns).
* @param   argv -  Input argument to program.
* @return  client_server - On success, a socket to server i received
* else -1.
*/
int setupConnection(char **argv) {

    int client_server=0;
    sock_init_struct *sis=calloc(1,sizeof(sock_init_struct));
    sis->isUDP=false;
    sis->nexthost=argv[3];
    sis->nextportString=argv[4];
    if(strncmp("cs",argv[2],2)==0){
        if((client_server=connectCS(sis,argv[1]))==-1){
            free(sis);
            return -1;
        }
        connectedServer="SERVER-MESSAGE";
    }else{
        fprintf(stderr,"Invalid argument (setupConnection)\n");
        free(sis);
        return -1;
    }
    free(sis);
    return client_server;
}



/* Print if input argument was wrong.
* @param    progName -  Program name.
* @return
*/
void printWrongParams(char *progName) {
    fprintf(stderr, "%s\n%s %s\n", "Invalid parameters",
    progName, "<NAME> <ns/cs> <HOST> <PORT>");
}
