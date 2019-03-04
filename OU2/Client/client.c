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
    char message[255+1]={0};
    int client_server;
    //pthread_t trd[1];

    if((client_server = setupConnection(argv))==-1){
        printf("Exit program.\n");
        exit(EXIT_FAILURE);
    }
    PDU_struct *PDU_struct;
    while(1){
        if(fgets(message,255,stdin)==NULL){
            usleep(1000);
            continue;
        }
        if(strncmp(message, "\n", 255)==0) {
            continue;
        }
        if(strncmp(message, EXIT, 4)==0) {

            printf("Disconnecting from server.\n");
            //closeClient(sock);
            return -1;
        }
        PDU_struct = create_message_to_server(message);
        send_pdu(client_server, PDU_struct);
        free(PDU_struct->pdu);
        free(PDU_struct);


    }

    return 0;
}

PDU_struct *create_message_to_server(char *input) {

    PDU_struct *PDU_struct = calloc(1, sizeof(PDU_struct));
    size_t messlen = strnlen(input, MAXMESSLEN)+1);
    PDU_struct->pdu = calloc(1, messlen);
    PDU_struct->numbytes = messlen;

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
        free(pdu->pdu);
        free(pdu);
        return -1;
    }*/
    //free(pdu->pdu);
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
    printf("sis->nexthost: %s\n", sis->nexthost);
    printf("sis->nextportString: %s\n", sis->nextportString);
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
