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
    PDU_struct *PDU_struct;

    do {

        printf("\n1. Init hashtable\n");
        printf("2. Insert to hashtable\n");
        printf("3. Get from hashtable\n");
        printf("4. Delete from hashtable\n");
        printf("5. Exit program\n");

        printf("\nGive your choice (1 - 5): ");
        scanf("%d", &choice);


        switch (choice) {
            case 1:
                PDU_struct = create_INIT_to_server();
                break;
            case 2:
                printf("\nEnter key: ");
                scanf("%s", key);
                printf("\nEnter value: ");
                scanf("%s", data);
                PDU_struct = create_INSERT_to_server(key, data);
                break;
            case 3:
                printf("\nEnter key: ");
                scanf("%s", key);
                PDU_struct = create_GET_to_server(key);
                break;
            case 4:
                printf("\nEnter key: ");
                scanf("%s", key);
                PDU_struct = create_DELETE_to_server(key);
                break;
            case 5:
                printf("Closing program\n");
                break;
            default:
                fprintf(stdout, "BAD INPUT!\n");

        }





        /*if(scanf("%s",message) < 0){
            fprintf(stderr,"Error when reading from stdin\n");
            return -1;
        }
        int i =0;
        while(message[i]) {
            message[i] = tolower(message[i]);
            i++;
        }
        printf("message: %s\n", message);
        if(strncmp(message, "\n", MAXMESSLEN)==0) {
            continue;
        }else if(strncmp(message, EXIT, 4)==0) {

            printf("Disconnecting from server.\n");
            //closeClient(sock);
            return -1;
        }else if(strncmp(message, "init", strlen(message))==0){
            PDU_struct = create_INIT_to_server();

        }else if(strncmp(message, "insert", 6) == 0){
            PDU_struct = create_INSERT_to_server("key","message");
        }else if(strncmp(message, "get", strlen(message))==0){
            PDU_struct = create_GET_to_server("key");
        }else if(strncmp(message, "delete", 6)==0){
            PDU_struct = create_DELETE_to_server("key");
        }*/

        send_pdu(client_server, PDU_struct);

        /*PDU_struct = create_message_to_server(message);
        send_pdu(client_server, PDU_struct);
        free(PDU_struct->data);
        free(PDU_struct);*/
        PDU_struct = receive_pdu(client_server);
        printf("OP i return: %u\n", PDU_struct->OP_code);





    }while (choice != 5);

    return 0;
}



PDU_struct *create_INIT_to_server(){
    PDU_struct *PDU_struct = calloc(1,sizeof(PDU_struct));

    INIT_struct* init_struct = calloc(1, sizeof(INIT_struct));
    init_struct->OP_code = INIT;

    data buffer = PDU_to_buffer_user(INIT, init_struct);

    PDU_struct->OP_code = INIT;
    PDU_struct->data_bytes = HEADERSIZE;
    PDU_struct->data = buffer;
    return PDU_struct;
}

PDU_struct *create_INSERT_to_server(char* key, char* data){

    PDU_struct *PDU_struct = calloc(1,sizeof(PDU_struct));

    INSERT_struct *insert_struct = calloc(1,sizeof(INSERT_struct));
    insert_struct->OP_code=INSERT;
    memcpy(insert_struct->key, key, KEY_SIZE);
    insert_struct->data_bytes=strnlen(data, MAX_PAYLOAD)+1;
    insert_struct->data = calloc(1,(insert_struct->data_bytes));

    memcpy(insert_struct->data, data, (insert_struct->data_bytes));

    void *action = PDU_to_buffer_user(INSERT, insert_struct); //kan inte vara data måste vara void*....

    PDU_struct->OP_code = INSERT;
    PDU_struct->data_bytes = (insert_struct->data_bytes) + HEADERSIZE + KEY_SIZE;
    PDU_struct->data = action;
    return PDU_struct;
}

PDU_struct *create_GET_to_server(char* key) {

    PDU_struct *PDU_struct = calloc(1,sizeof(PDU_struct));

    GET_struct *get_struct = calloc(1,sizeof(GET_struct));

    get_struct->OP_code = GET;
    memcpy(get_struct->key, key, KEY_SIZE);

    data action = PDU_to_buffer_user(GET, get_struct);
    PDU_struct->OP_code = GET;
    PDU_struct->data_bytes = HEADERSIZE + KEY_SIZE;
    PDU_struct->data = action;
    return PDU_struct;


}

PDU_struct *create_DELETE_to_server(char* key) {
    PDU_struct *PDU_struct = calloc(1,sizeof(PDU_struct));

    DELETE_struct *delete_struct = calloc(1,sizeof(DELETE_struct));

    delete_struct->OP_code = DELETE;
    memcpy(delete_struct->key, key, KEY_SIZE);

    data action = PDU_to_buffer_user(DELETE, delete_struct);
    PDU_struct->OP_code = DELETE;
    PDU_struct->data_bytes = HEADERSIZE + KEY_SIZE;
    PDU_struct->data = action;
    return PDU_struct;


}

/* Connects direct to a server without using the nameserver.
* @param   sis -  A struct with information to create a socket.
* @param   clientname -  The clients name (input argument to program).
* @return pdu - The created messagePDU.
*/
int connectCS(sock_init_struct *sis,char *clientname){

    //PDU_struct *pdu=setupJOINPDU(clientname);
    int server_client=createsocket_client(sis);


    /*if((send_pdu(server_client,pdu)==-1)){
    fprintf(stderr,"Could not send to server\n");
    free(pdu->data);
    free(pdu);
    return -1;
}*/
//free(pdu->data);
//free(pdu);
return server_client;
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
