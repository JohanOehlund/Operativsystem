#include "server.h"


//
// Created by c15jod on 2017-09-19.
//


#include "server.h"

pthread_mutex_t lock;
llist *clients_sockets;
llist *sender_list;
llist *client_ids;
llist *availableThreads;
llist *terminatedThreads;
bool closeAllClients;
bool exitServer;

pthread_t clientThreads[THREADS];

int main(int argc, char **argv){
    if (argc != 5) {
        printWrongParams(argv[0]);
        return EXIT_FAILURE;
    }

    clients_sockets=llist_empty();
    sender_list=llist_empty();
    client_ids=llist_empty();
    availableThreads = llist_empty();
    terminatedThreads = llist_empty();
    pthread_t trd[5];
    closeAllClients = false;
    exitServer=false;
    if (pthread_mutex_init(&lock, NULL) != 0){
        fprintf(stderr,"\n mutex init failed\n");
        return 1;
    }

    if(pthread_create(&trd[0], NULL, nameserverREG, (void*)argv)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&trd[1], NULL, joinThreads, NULL)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&trd[2], NULL, sendToClients, (void*)argv)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&trd[3], NULL, serverWrite, NULL)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&trd[4], NULL, acceptConnections, (void*)argv)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < 4; ++i) {
        if(pthread_join(trd[i],NULL) != 0) {
            perror("thread join");
            exit(EXIT_FAILURE);
        }
    }
    if(pthread_cancel(trd[4])<0){
        perror("pthread_cancel");
        exit(EXIT_FAILURE);
    }

    if(pthread_join(trd[4],NULL) != 0) {
        perror("thread join");
        exit(EXIT_FAILURE);
    }

    if(pthread_mutex_destroy(&lock)!=0){
        perror("mutex destroy failed");
        return 1;
    }
    llist_free(clients_sockets);
    llist_free(client_ids);
    llist_free(availableThreads);
    llist_free(terminatedThreads);
    llist_free(sender_list);
    return 0;
}

/* Closes server and sends QUIT to for data from server and handles depending of what PDU is received.
 * @param    arg -  The created socket to server.
 * @return NULL
 * @comment Shuts down connection and socket if received QUIT or client leaves server.
 */
void closeServer() {
    closeAllClients = true;
    while(list_length(availableThreads)<THREADS){
        pthread_mutex_lock(&availableThreads->mtx);
        pthread_cond_wait(&availableThreads->cond,&availableThreads->mtx);
        pthread_mutex_unlock(&availableThreads->mtx);
    }
    /*while(list_length(availableThreads)<THREADS){
        usleep(2000);
    }*/
    printf("Joined all client threads.\n");
    exitServer=true;
    pthread_cond_broadcast(&sender_list->cond);
    pthread_cond_broadcast(&terminatedThreads->cond);
    pthread_mutex_unlock(&terminatedThreads->mtx);
}

/* Joins client threads that have left the server(terminated).
 * @param    arg -  The created socket to server.
 * @return NULL
 * @comment Shuts down connection and socket if received QUIT or client leaves server.
 */
void *joinThreads(void *arg) {
    int *temp=NULL;

    while(1) {

        if(llist_isEmpty(terminatedThreads)) {
            pthread_mutex_lock(&terminatedThreads->mtx);
            pthread_cond_wait(&terminatedThreads->cond, &terminatedThreads->mtx);
            pthread_mutex_unlock(&terminatedThreads->mtx);
            if(exitServer)
                break;
        }
        int *thread_number = calloc(1, sizeof(int *));
        temp = llist_removefirst_INT(terminatedThreads);
        memcpy(&thread_number, &temp, sizeof(int *));
        printf("joining thread %d\n", *temp);
        if(pthread_join(clientThreads[*temp],NULL) != 0) {
            perror("thread join");
            exit(EXIT_FAILURE);
        }
        llist_insertlast(availableThreads, thread_number);

    }
    return NULL;
}

/* Create a message from a client by adding client name, clients len and timestamp. The new PDU
 *  gets a new checksum.
 * @param   mess -  The MESS-PDU from client
 * @return  pdu - The created MESS-PDU.
 */
PDU_struct *createServerMESS(char *mess){
    GEN_struct *gen=calloc(1,sizeof(GEN_struct));
    gen->OPCode=MESS;
    MESS_struct *mess_struct=calloc(1,sizeof(MESS_struct));
    mess_struct->OPCode=MESS;

    mess_struct->timestamp=(uint32_t )time(NULL);
    mess_struct->ID_len=0;
    mess_struct->client_ID=NULL;
    mess_struct->message=calloc(1, padFunction(strnlen(mess,MAXMESSLEN)));
    strncpy(mess_struct->message,mess,padFunction(strnlen(mess,MAXMESSLEN)));
    mess_struct->message_len=(uint16_t)strnlen(mess,MAXMESSLEN);
    gen->created_struct=mess_struct;
    mess_struct->checksum=calculateSTRUCTChecksum(mess_struct);
    PDU_struct *pdu=pduCreater(gen);
    return pdu;

}

/* Takes input from user from stdin.
 *  gets a new checksum.
 * @param   arg -
 * @return
 * @comment Type EXIT to close server.
 */
void *serverWrite(void *arg) {
    printf("To close server type <%s>\n",EXIT);
    while (1){
        char message[MAXMESSLEN+1];
        fgets(message,MAXMESSLEN,stdin);
        if(strncmp(message, "\n", MAXMESSLEN)==0)
            continue;
        if(strncmp(message, EXIT, 4)==0) {
            printf("Closing server.\n");
            closeServer();
            return NULL;
        }
        char *mess=calloc(1,strnlen(message,MAXMESSLEN));
        strncpy(mess,message,strnlen(message,MAXMESSLEN)-1);
        PDU_struct *messPDU=createServerMESS(mess);
        llist_insertfirst(sender_list,messPDU);
        free(mess);
        fflush(stdin);
    }
    return NULL;
}

/* Sends all PDU:s to all connected clients.
 * @param   arg -
 * @return
 * @comment Thread terminates by setting varible "exitServer" to true.
 */
void *sendToClients(void *arg){
    while(!exitServer){
        if(llist_isEmpty(clients_sockets)){
            llist_position p3=llist_first(client_ids);
            for (int j = 0; j < list_length(client_ids) ; ++j) {
                llist_remove(client_ids, p3);
                p3 = llist_next(p3);
            }
        }
        if(llist_isEmpty(sender_list)){
            pthread_mutex_lock(&sender_list->mtx);
            pthread_cond_wait(&sender_list->cond, &sender_list->mtx);
            pthread_mutex_unlock(&sender_list->mtx);
            if(exitServer)
                return NULL;
        }
        pthread_mutex_lock(&lock);
        PDU_struct *pdu=llist_removefirst_PDU(sender_list);
        llist_position p=llist_first(clients_sockets);
        for (int i = 0; i < list_length(clients_sockets) ; ++i) {
            int sock=*(int *)llist_inspect(p,clients_sockets);
            send_pdu(sock,pdu);
            p=llist_next(p);
        }
        pthread_mutex_unlock(&lock);
        free(pdu->pdu);
        free(pdu);
    }
    return NULL;
}

/* Create a struct that contains information to connecting client
 * @param   argv -  input argument to program.
 * @return  sock - The created socket.
 */
int setupListeningSocket(char **argv) {

    sock_init_struct *sis = calloc(1, sizeof(sock_init_struct));
    sis->isUDP=false;
    sis->nexthost=NULL;
    sis->nextportString=argv[1];
    int sock = createsocket_server(sis);
    free(sis);
    return sock;
}

/* Listens for client connections.
 * @param   argv -  input argument to program.
 * @return
 * @comment Closing connection if new client tries to connect when server is full.
 */
void *acceptConnections(void *arg) {
    char **argv = arg;

    int temp2;
    int *thread_number;
    int sock=setupListeningSocket(argv);
    listen(sock, 128);
    for (int j = 0; j < THREADS ; ++j) {
        int *temp=calloc(1,sizeof(int *));
        memcpy(temp,&j,sizeof(int *));
        llist_insertlast(availableThreads, temp);
    }

    while(1){
        temp2 = accept(sock,0, 0);
        int *client_sock = calloc(1, sizeof(int *));
        memcpy(client_sock, &temp2, sizeof(int));

        if(client_sock < 0){
            perror("Receiver - accept failed");
            exit(EXIT_FAILURE);
        }
        if((thread_number=findFreeThread()) == (int *) -5) {
            shutdown(temp2, SHUT_RDWR); //Server is full
            close(temp2);
            free(client_sock);
            continue;
        }
        if(!closeAllClients) {
            clientThreadInfo *cti = calloc(1, sizeof(clientThreadInfo));
            cti->client_sock = *client_sock;
            cti->thread_num = *thread_number;
            if (pthread_create(&clientThreads[*thread_number], NULL, clientlistener, cti) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }else {
            printf("Server is closing refuse new connections\n"); //Closing server
            shutdown(*client_sock, SHUT_RDWR);
            close(*client_sock);

        }

        free(thread_number);
        free(client_sock);
    }

    return NULL;
}

/* Look through list if there are any free threads available.
 * @param
 * @return int - The first free thread.
 * @comment The list "availableThreads" do not contains threads, it contains the associated thread number.
 */
int *findFreeThread() {

    if(llist_isEmpty(availableThreads)){
        usleep(1000);
        printf("Server is full, closing connection\n");
        return (int *)-5;
    }

    return llist_removefirst_INT(availableThreads);
}

/* Listens for PDU:s from client.
 * @param arg - A struct that contains information about client.
 * @return
 * @comment If client leaves then thread terminates and adds its number in the "terminatedthreads" list.
 */
void *clientlistener(void *arg){
    clientThreadInfo *cti = (clientThreadInfo *)arg;
    int temp_socket = cti->client_sock;
    bool exitLoop=false;
    GEN_struct *gen_struct=NULL;


    gen_struct=clientJOIN(temp_socket);

    if(gen_struct==NULL){
        fprintf(stderr,"JOIN_struct was NULL.(client_listener)\n");
        return NULL;
    }
    JOIN_struct *join_struct=gen_struct->created_struct;
    printf("§%s§ connected.\n",join_struct->client_ID);
    while(!closeAllClients) {
        if(llist_isEmpty(clients_sockets)){
            llist_position p3=llist_first(client_ids);
            for (int j = 0; j < list_length(client_ids) ; ++j) {
                llist_remove(client_ids, p3);
                p3 = llist_next(p3);
            }
        }
        void *pdu = receive_pdu(temp_socket);
        if((int *)pdu==(int*)-100){
            llist_insertfirst(terminatedThreads, &cti->thread_num);
            closeConnectedClient(temp_socket, gen_struct, 1);
            return NULL;
        }
        usleep(2000);
        if(pdu==NULL)
            continue;

        GEN_struct *gen=structcreater(pdu);
        if(gen==NULL){
            closeConnectedClient(temp_socket, gen_struct, 0);
            llist_insertfirst(terminatedThreads, &cti->thread_num);
            return NULL;
        }
        switch(gen->OPCode){
            case MESS:
                if(addMESSToList(gen->created_struct,join_struct)!=0){
                    closeConnectedClient(temp_socket, gen_struct, 0);
                    llist_insertfirst(terminatedThreads, &cti->thread_num);
                    free_struct(gen);
                    return NULL;
                }
                break;
            case QUIT:
                llist_insertfirst(terminatedThreads, &cti->thread_num);
                closeConnectedClient(temp_socket, gen_struct, 0);
                free_struct(gen);
                return NULL;
            default:
                fprintf(stderr,"Server received invalid pdu from: %s, opcode "
                                "%u\n", join_struct->client_ID, gen->OPCode);
                closeConnectedClient(temp_socket, gen_struct, 0);
                llist_insertfirst(terminatedThreads, &cti->thread_num);
                free_struct(gen);
                return NULL;

        }
        free_struct(gen);
    }
    closeConnectedClient(temp_socket, gen_struct, 0);
    llist_insertfirst(terminatedThreads, &cti->thread_num);
    return NULL;
}

/* Create and send a QUIT-PDU
 * @param sock - The socket to send a QUIT-PDU.
 * @return
 * @comment
 */
void sendQUIT(int sock) {
    GEN_struct *gen = calloc(1, sizeof(GEN_struct));
    QUIT_struct *quit = calloc(1, sizeof(QUIT_struct));
    gen->OPCode = QUIT;
    quit->OPCode = QUIT;
    gen->created_struct = quit;
    PDU_struct *pdu = pduCreater(gen);
    send_pdu(sock, pdu);
    free(pdu->pdu);
    free(pdu);
}

/* Closes a connected client
 * @param client_sock - The socket to close connection with.
 * @param gen_struct - A struct that contains closing sockets name.
 * @param cliC - If cliC=1 then client_sock already closed connection.
 * @return
 */
void closeConnectedClient(int client_sock, GEN_struct *gen_struct, int cliC) {

    JOIN_struct *join_struct=gen_struct->created_struct;
    int socket = 0;
    char *temp_client = NULL;
    if(cliC == 0)
        sendQUIT(client_sock);

    pthread_mutex_lock(&lock);
    llist_position p = llist_first(clients_sockets);
    for (int i = 0; i < list_length(clients_sockets); ++i) {
        socket = *(int*)llist_inspect(p, clients_sockets);
        if(client_sock == socket) {
            llist_remove(clients_sockets, p);
            break;
        }
        p = llist_next(p);
    }
    llist_position p2 = llist_first(client_ids);
    for (int j = 0; j < list_length(client_ids) ; ++j) {
        temp_client = llist_inspect(p2, client_ids);
        if(strncmp(temp_client, join_struct->client_ID, join_struct->ID_len) == 0) {
            llist_remove(client_ids, p2);
            break;
        }
        p2 = llist_next(p2);
    }
    addPLEAVEToList(join_struct);
    if(cliC == 0) {
        shutdown(client_sock, SHUT_RDWR);
        close(client_sock);
    }
    pthread_mutex_unlock(&lock);
    free_struct(gen_struct);

}


/* Adds a MESS-PDU to sender_list
 * @param stru - The created MESS-Struct of client.
 * @param join_struct - A struct that contains closing sockets name.
 * @return On success then 0 else 1
 */
int addMESSToList(MESS_struct *stru,JOIN_struct *join_struct){
    MESS_struct *server_mess=calloc(1,sizeof(MESS_struct));
    GEN_struct *gen=calloc(1,sizeof(GEN_struct));
    server_mess->OPCode=stru->OPCode;
    server_mess->client_ID = calloc(1,join_struct->ID_len);
    memcpy(server_mess->client_ID,join_struct->client_ID, join_struct->ID_len);
    server_mess->ID_len=join_struct->ID_len;
    server_mess->message=calloc(1,stru->message_len);
    memcpy(server_mess->message,stru->message, stru->message_len);
    server_mess->message_len=stru->message_len;
    server_mess->timestamp=(uint32_t )time(NULL);
    //server_mess->timestamp=5555;
    server_mess->checksum = calculateSTRUCTChecksum(server_mess);
    gen->OPCode=MESS;
    gen->created_struct=server_mess;
    PDU_struct *pdu=pduCreater(gen);
    if(pdu==NULL)
        return 1;
    llist_insertfirst(sender_list,pdu);
    return 0;

}

/* Adds a PLEAVE-PDU to sender_list
 * @param join_struct - A struct that contains closing sockets name.
 * @return
 */
void addPLEAVEToList(JOIN_struct *join_struct){

    PLEAVE_struct *pleave = calloc(1, sizeof(PLEAVE_struct));
    GEN_struct *gen=calloc(1,sizeof(GEN_struct));
    pleave->OPCode=PLEAVE;
    pleave->client_ID = calloc(1,join_struct->ID_len);
    memcpy(pleave->client_ID,join_struct->client_ID,join_struct->ID_len);
    pleave->ID_len =join_struct->ID_len;
    pleave->timestamp = (uint32_t)time(NULL);
    gen->OPCode = PLEAVE;
    gen->created_struct = pleave;
    PDU_struct *pdu=pduCreater(gen);
    llist_insertfirst(sender_list, pdu);

}

/* Adds a PJOIN-PDU to sender_list
 * @param join_struct - A struct that contains closing sockets name.
 * @return
 */
void addPJOINToList(JOIN_struct *join_struct) {

    PJOIN_struct *pjoin = calloc(1, sizeof(PJOIN_struct));
    GEN_struct *gen=calloc(1,sizeof(GEN_struct));
    pjoin->OPCode=PJOIN;
    pjoin->client_ID = calloc(1,join_struct->ID_len);
    memcpy(pjoin->client_ID,join_struct->client_ID, join_struct->ID_len);
    pjoin->ID_len = join_struct->ID_len;
    pjoin->timestamp = (uint32_t)time(NULL);
    gen->OPCode = PJOIN;
    gen->created_struct = pjoin;
    PDU_struct *pdu=pduCreater(gen);
    llist_insertfirst(sender_list, pdu);

}

/* Creates a JOIN_struct of the newly connected client.
 * @param client_sock - The connected clients socket.
 * @return gen - The created JOIN_struct.
 */
GEN_struct *clientJOIN(int client_sock){

    int *clientsock=calloc(1,sizeof(int*));
    memcpy(clientsock,&client_sock,sizeof(int*));

    void *pdu=NULL;
    while(1){
        pdu= receive_pdu(client_sock);
        if(pdu==NULL)
            continue;
        break;
    }
    GEN_struct *gen=structcreater(pdu);
    JOIN_struct *join_struct=gen->created_struct;

    switch(gen->OPCode){
        case JOIN:
            addPJOINToList(join_struct);
            while(!llist_isEmpty(sender_list)){
                usleep(1000);
            }
            pthread_mutex_lock(&lock);
            llist_insertfirst(client_ids,join_struct->client_ID);
            createPARTICIPANTSServer(client_sock);
            llist_insertfirst(clients_sockets, clientsock);
            pthread_mutex_unlock(&lock);
            break;
        default:
            fprintf(stderr,"Client sent incorrect pdu.(clientJOIN)\n");
            free(clientsock);
            return NULL;
    }
    return gen;
}


/* Creates a PARTICIPANTS to send to client.
 * @param client_sock - The connected clients socket.
 * @return
 */
void createPARTICIPANTSServer(int client_socket){
    GEN_struct *gen=calloc(1,sizeof(GEN_struct));
    PARTICIPANTS_struct *part=calloc(1,sizeof(PARTICIPANTS_struct));
    gen->OPCode=PARTICIPANTS;
    llist_position p=llist_first(client_ids);
    uint8_t list_len=(uint8_t )list_length(client_ids);
    uint32_t client_len=0;
    for (int i = 0; i < list_len; ++i) {
        char *temp_client=llist_inspect(p,client_ids);
        client_len+=strnlen(temp_client,MAXNAMELEN);
        p=llist_next(p);
    }
    part->OPCode=PARTICIPANTS;
    part->num_clients=(uint8_t )list_len;
    part->ID_len=(uint16_t )client_len+list_len;
    char *clients=calloc(1,padFunction(part->ID_len));
    char *clients_start=clients;
    llist_position p2=llist_first(client_ids);
    for (int j = 0; j < list_len; ++j) {
        char *temp_client=llist_inspect(p2,client_ids);
        strncpy(clients,temp_client,strnlen(temp_client,MAXNAMELEN));
        clients+=strnlen(temp_client,MAXNAMELEN)+1;
        p2=llist_next(p2);
    }

    part->clients=calloc(1,padFunction(part->ID_len));
    memcpy(part->clients,clients_start,part->ID_len);

    gen->created_struct=part;
    PDU_struct *pdu=pduCreater(gen);
    send_pdu(client_socket, pdu);
    free(clients_start);
    free(pdu->pdu);
    free(pdu);
    return;
}

/* Register the server to a nameserver.
 * @param arg - input argument to the program.
 * @return
 */
void *nameserverREG(void * arg){
    int server_nameserver=0;
    char ** argv=arg;
    void *ans_ack_pdu;
    int i=0;
    server_nameserver=setup_nameserver(argv);
    while(!closeAllClients){
        i=0;
        send_REG(server_nameserver,argv);
        while(1){
            ans_ack_pdu=receive_pdu(server_nameserver);
            if(ans_ack_pdu==NULL){
                if(++i==5)
                    break;
                sleep(1);
                continue;
            }

            break;
        }
        if(ans_ack_pdu!=NULL){
            GEN_struct *gen=structcreater(ans_ack_pdu);
            keep_alive(server_nameserver,gen);
            free_struct(gen);
        }
    }
    return NULL;
}

/* Creates a REG-PDU to nameserver.
 * @param arg - input argument to the program.
 * @return pdu -The created REG-PDU.
 */
PDU_struct *setupREGPDU(char **argv){
    char *end;
    GEN_struct *gen=calloc(1, sizeof(GEN_struct));
    gen->OPCode=REG;
    REG_struct *reg_struct=calloc(1,sizeof(REG_struct));
    reg_struct->OPCode=REG;
    reg_struct->tcp_port=(uint16_t)strtol(argv[1],&end,10);
    reg_struct->servername_len=(uint8_t)strnlen(argv[2],MAXNAMELEN);
    reg_struct->servername=calloc(1,strnlen(argv[2],MAXNAMELEN));
    strncpy(reg_struct->servername,argv[2],strnlen(argv[2],MAXNAMELEN));
    gen->created_struct=reg_struct;
    PDU_struct *pdu=pduCreater(gen);
    return pdu;
}

/* Creates a ALIVE-PDU to nameserver.
 * @param gen - ACK-struct that nameserver send.
 * @return pdu -The created ALIVE-PDU.
 */
PDU_struct *setupALIVEPDU(GEN_struct *gen){
    ACK_struct *ack_struct=gen->created_struct;
    GEN_struct *alive_gen=calloc(1, sizeof(GEN_struct));
    alive_gen->OPCode=ALIVE;
    void *alive_buff=createALIVEBuffer(ack_struct->ID_number);
    GEN_struct *alive_struct=structcreater(alive_buff);
    PDU_struct *pdu=pduCreater(alive_struct);
    free_struct(alive_gen);
    return pdu;
}

/* Send the created REG-PDU to nameserver.
 * @param arg - input argument to the program.
 * @param server_nameserver - Nameserver socket.
 * @return
 */
void send_REG(int server_nameserver,char **argv){
    PDU_struct *pdu=setupREGPDU(argv);

    send_pdu(server_nameserver,pdu);
    free(pdu->pdu);
    free(pdu);
    return;

}

/* Keeps the server registered om the nameserver
 * @param gen - ACK-struct that nameserver send.
 * @param server_nameserver - Nameserver socket.
 * @return
 */
bool keep_alive(int server_nameserver,GEN_struct *gen){
    sleep(SLEEP);
    while(!closeAllClients){
        PDU_struct *alive_pdu=setupALIVEPDU(gen);
        send_pdu(server_nameserver,alive_pdu);
        PDU_struct *answer=NULL;
        while(1){
            answer =receive_pdu(server_nameserver);
            if(answer==NULL) {
                sleep(3);
            }else{
                break;
            }
            answer =receive_pdu(server_nameserver);
            if(answer==NULL){
                free(alive_pdu->pdu);
                free(alive_pdu);
                return true;
            }
            break;
        }

        GEN_struct *gen2=structcreater(answer);

        switch(gen2->OPCode) {
            case ACK:
                sleep(SLEEP);
                break;
            case NOTREG:
                return false;
            default:
                fprintf(stderr,"Nameserver sent invalid pdu (keep_alive).\n");
                return true;
        }
        free_struct(gen2);
        free(alive_pdu->pdu);
        free(alive_pdu);
    }
    return false;
}

int setup_nameserver(char **argv){
    sock_init_struct *sis=calloc(1,sizeof(sock_init_struct));
    int sock;
    sis->isUDP=true;
    sis->nexthost=argv[3];
    sis->nextportString=argv[4];

    sock=createsocket_client(sis);
    free(sis);
    return sock;

}



void *createALIVEBuffer(uint16_t ID_num){
    uint16_t id_num=ntohs(ID_num);
    uint8_t num_clients=(uint8_t)list_length(client_ids);
    void *buffer=calloc(1,HEADERSIZE);
    void *start=buffer;
    memset(buffer,ALIVE,sizeof(uint8_t));
    ++buffer;
    memcpy(buffer,&num_clients,sizeof(uint8_t));
    ++buffer;
    memcpy(buffer,&id_num,sizeof(uint16_t));
    return start;
}

void printWrongParams(char *progName) {
    fprintf(stderr,
            "%s\n%s %s\n",
            "Invalid parameters",
            progName,
            "<PORT> <SERVERNAME> <NAMESERVER-HOST> <NAMESERVER-PORT>");
}
