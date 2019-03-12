//
// Created by c15jod on 2017-10-09.
//
char *gets(char *str);
#include "client.h"
pthread_mutex_t lock;
char* connectedServer;
bool close_client = false;

int main(int argc, char **argv){

    if (argc != 5) {
        printWrongParams(argv[0]);
        return EXIT_FAILURE;
    }

    /*if (pthread_mutex_init(&lock, NULL) != 0){
        fprintf(stderr,"\n mutex init failed\n");
        return 1;
    }*/
    pthread_t trd[1];
    int client_server;

    if((client_server = setupConnection(argv))==-1){
        printf("Exit program.\n");
        exit(EXIT_FAILURE);
    }

    if(pthread_create(&trd[0], NULL, client_listen, &client_server) != 0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    client_send(client_server);
    if(pthread_cancel(trd[0])<0){
        perror("pthread_cancel");
        exit(EXIT_FAILURE);
    }

    if(pthread_join(trd[0],NULL) != 0) {
        perror("thread join");
        exit(EXIT_FAILURE);
    }

    return 0;
}

data client_send(int sock){
    int choice;
    int temp_char;
    char newline;
    int index;
    char key[MAXMESSLEN+1];
    char data[MAXMESSLEN+1];
    PDU_struct *PDU_struct_send = NULL;
    do {
        if(close_client){
            break;
        }
        memset(data,'\0',MAXMESSLEN+1);
        memset(key,'\0',MAXMESSLEN+1);
        index = 0;

        print_options();
        char message[MAXMESSLEN+1];
        fgets(message,MAXMESSLEN,stdin);
        if(strncmp(message, "\n", MAXMESSLEN)==0)
            continue;
        //if()
        fflush(stdout);
        switch (message[0]) {
            case '1':
                PDU_struct_send = create_INIT_to_server();
                break;
            case '2':
                printf("\nEnter key: ");
                temp_char = getchar();
                key[0] = temp_char;
                index = 0;
                while(temp_char != '\n'){
                     index++;
                     temp_char = getchar();
                     if(temp_char != '\n')
                        key[index] = temp_char;
                }
                printf("\nEnter value: ");
                temp_char = getchar();
                data[0] = temp_char;
                index = 0;
                while(temp_char != '\n'){
                     index++;
                     temp_char = getchar();
                     if(temp_char != '\n')
                        data[index] = temp_char;
                }
                PDU_struct_send = create_INSERT_to_server(key, data, strlen(data)+1);
                break;
            case '3':
                printf("\nEnter key: ");
                temp_char = getchar();
                key[0] = temp_char;
                index = 0;
                while(temp_char != '\n'){
                     index++;
                     temp_char = getchar();
                     if(temp_char != '\n')
                        key[index] = temp_char;
                }
                PDU_struct_send = create_GET_to_server(key);
                break;
            case '4':
                printf("\nEnter key: ");
                temp_char = getchar();
                key[0] = temp_char;
                index = 0;
                while(temp_char != '\n'){
                     index++;
                     temp_char = getchar();
                     if(temp_char != '\n')
                        key[index] = temp_char;
                }
                PDU_struct_send = create_DELETE_to_server(key);
                break;
            case '5':
                printf("Closing program\n");
                PDU_struct_send = create_QUIT_pdu();
                send_pdu(sock, PDU_struct_send);
                free_struct(USER,PDU_struct_send);
                return 0;
                break;
            case '6':
                TEST_INT_INSERT(sock);
                continue;
            default:
                fprintf(stdout, "BAD INPUT!\n");
                continue;

        }



        send_pdu(sock, PDU_struct_send);
        free_struct(USER,PDU_struct_send);
        //free_struct(KERNEL,PDU_struct_recieve);
    }while (choice != 5);
    shutdown(sock,SHUT_RDWR);
    close(sock);
    close_client = true;
}

void print_options(){
    printf("\n1. Init hashtable\n");
    printf("2. Insert to hashtable\n");
    printf("3. Get from hashtable\n");
    printf("4. Delete from hashtable\n");
    printf("5. Exit program\n");
    printf("6. TEST_INT_INSERT()\n");
    printf("\nGive your choice (1 - 6): ");
    fflush(stdout);
}

data client_listen(data arg) {
    int client_server = *(int*)arg;
    uint8_t OP_code;
    while(!close_client){
        PDU_struct *PDU_struct_recieve = NULL;
        PDU_struct_recieve = receive_pdu(client_server);
        memcpy(&OP_code, PDU_struct_recieve->data, 1);
        if(OP_code == QUIT) {
            close_client = true;
            printf("Server has kicked you, press enter to quit.\n");
            free_struct(USER,PDU_struct_recieve);
            close_client = true;
            break;
        }
        //printf("OP i return: %u\n", PDU_struct_recieve->OP_code);
        //printf("data_bytes i return: %u\n", PDU_struct_recieve->data_bytes);
        printf("\n\nDATA FROM KERNEL: %s\n", (char*)PDU_struct_recieve->data);
        free_struct(USER,PDU_struct_recieve);
        print_options();
    }
    shutdown(client_server,SHUT_RDWR);
    close(client_server);
    return NULL;
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
    memset(insert_struct->key,'\0', KEY_SIZE);
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
    memset(get_struct->key,'\0', KEY_SIZE);
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

    if(server_client == -1) {
        free_struct(USER, pdu);
        return -1;
    }


    if((send_pdu(server_client,pdu)==-1)){
        fprintf(stderr,"Could not send to server\n");
        free_struct(USER, pdu);
        return -1;
    }
    free_struct(USER, pdu);
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
