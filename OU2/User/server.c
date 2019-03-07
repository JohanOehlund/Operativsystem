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
    if (argc != 3) {
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
    setup_netlink();

    closeAllClients = false;
    exitServer=false;
    if (pthread_mutex_init(&lock, NULL) != 0){
        fprintf(stderr,"\n mutex init failed\n");
        return 1;
    }

    if(pthread_create(&trd[0], NULL, joinThreads, NULL)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&trd[1], NULL, sendToClients, (void*)argv)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&trd[2], NULL, serverWrite, NULL)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&trd[3], NULL, acceptConnections, (void*)argv)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 3; ++i) {
        printf("join trd %d\n", i);
        if(pthread_join(trd[i],NULL) != 0) {
            perror("thread join");
            exit(EXIT_FAILURE);
        }
    }
    if(pthread_cancel(trd[3])<0){
        perror("pthread_cancel");
        exit(EXIT_FAILURE);
    }
    if(pthread_join(trd[3],NULL) != 0) {
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





    //test_rhashtable((void*)0);
    return 0;
}

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
        printf("WAITING FOR MESSAGES TO SEND!\n");
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
        free(pdu->data);
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

        if(temp2 < 0){
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

 /* Listens for PDU:s from client.
 * @param arg - A struct that contains information about client.
 * @return
 * @comment If client leaves then thread terminates and adds its number in the "terminatedthreads" list.
 */
void *clientlistener(void *arg){
    clientThreadInfo *cti = (clientThreadInfo *)arg;
    int temp_socket = cti->client_sock;

    bool exitLoop=false;
    PDU_struct *pdu_JOIN = NULL;
    pdu_JOIN = clientJOIN(temp_socket);
    JOIN_struct *join_struct = (JOIN_struct*) pdu_JOIN->data;
    printf("Received JOIN from user: %s\n", join_struct->client_ID);


    while(1) {

        PDU_struct *PDU_struct_SEND = NULL;
        PDU_struct *PDU_struct_RECEIVE = NULL;
        /*if(llist_isEmpty(clients_sockets)){
            llist_position p3=llist_first(client_ids);
            for (int j = 0; j < list_length(client_ids) ; ++j) {
                llist_remove(client_ids, p3);
                p3 = llist_next(p3);
            }
        }*/
        reset_netlink();
        PDU_struct_SEND = receive_pdu(temp_socket);

        memcpy(NLMSG_DATA(nlh_user), PDU_struct_SEND->data, PDU_struct_SEND->data_bytes);
        if((int *)PDU_struct_SEND==(int*)-100){
            //llist_insertfirst(terminatedThreads, &cti->thread_num);
            closeConnectedClient(temp_socket, 1);
            return NULL;
        }

        printf("Sending message to kernel\n");
        sendmsg(sock_fd,&msg,0);

        printf("Waiting for message from kernel\n");
        recvmsg(sock_fd, &msg, 0);

        PDU_struct_RECEIVE = read_exactly_from_kernel(nlh_user);
        //printf("Received message opcode: %u\n", PDU_struct_RECEIVE->OP_code);
        //printf("Received message data_bytes: %u\n", PDU_struct_RECEIVE->data_bytes);
        send_pdu(temp_socket, PDU_struct_RECEIVE);

        free_struct(KERNEL, PDU_struct_RECEIVE);
        free_struct(KERNEL, PDU_struct_SEND);
        usleep(2000);

    }
    closeConnectedClient(temp_socket, 0);
    //llist_insertfirst(terminatedThreads, &cti->thread_num);
    return NULL;
}

/* Closes a connected client
 * @param client_sock - The socket to close connection with.
 * @param gen_struct - A struct that contains closing sockets name.
 * @param cliC - If cliC=1 then client_sock already closed connection.
 * @return
 */
void closeConnectedClient(int client_sock, int cliC) {

    //JOIN_struct *join_struct=gen_struct->created_struct;
    int socket = 0;
    char *temp_client = NULL;
    /*if(cliC == 0)
        sendQUIT(client_sock);*/

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
        /*if(strncmp(temp_client, join_struct->client_ID, join_struct->ID_len) == 0) {
            llist_remove(client_ids, p2);
            break;
        }*/
        p2 = llist_next(p2);
    }
    //addPLEAVEToList(join_struct);
    if(cliC == 0) {
        shutdown(client_sock, SHUT_RDWR);
        close(client_sock);
    }
    pthread_mutex_unlock(&lock);
    //free_struct(gen_struct);

}


/* Creates a JOIN_struct of the newly connected client.
 * @param client_sock - The connected clients socket.
 * @return gen - The created JOIN_struct.
 */
PDU_struct *clientJOIN(int client_sock){
    PDU_struct *PDU_struct=NULL;
    while(1){
        printf("WAITING for JOINPDU\n");
        usleep(1000);
        PDU_struct = receive_pdu(client_sock);
        if(PDU_struct==NULL)
            continue;
        break;
    }

    return PDU_struct;
}

void printWrongParams(char *progName) {
    fprintf(stderr,
            "%s\n%s %s\n",
            "Invalid parameters",
            progName,
            "<PORT> <SERVERNAME>");
}



























/*

void *test_rhashtable(data arg) {
    setup_netlink();

    init_rhashtable();
    int i;
    char key[KEY_SIZE];
    char key2[KEY_SIZE];
    char key3[KEY_SIZE];
    for (i = 10; i < 15; i++) {
        memcpy(key, "10", KEY_SIZE);
        reset_netlink();
        insert_rhashtable(key);
    //}
    //printf("HEJHEJ2\n");
    //int j;
    //for (j = 10; j < 15; j++) {
        //memcpy(key2, "10", KEY_SIZE);
        reset_netlink();
        get_rhashtable(key);
    //}
    //printf("HEJHEJ3\n");
    //int k;
    //for (k = 10; k < 15; k++) {
        //memcpy(key3, "10", KEY_SIZE);
        reset_netlink();
        delete_rhashtable(key);
    }
}

void delete_rhashtable(char* key){
    DELETE_struct *delete_struct = calloc(1,sizeof(DELETE_struct));

    delete_struct->OP_code = DELETE;
    memcpy(delete_struct->key, key, KEY_SIZE);

    data action = PDU_to_buffer_user(DELETE, delete_struct);

    memcpy(NLMSG_DATA(nlh_user), action, 68);

    //printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);

    //printf("Waiting for message from kernel\n");
    recvmsg(sock_fd, &msg, 0);

    PDU_struct * pdu = read_exactly_from_kernel(nlh_user);

    free(delete_struct);
    free(action);
    free(pdu->data);
    free(pdu);
}

void get_rhashtable(char* key){
    GET_struct *get_struct = calloc(1,sizeof(GET_struct));

    get_struct->OP_code = GET;
    memcpy(get_struct->key, key, KEY_SIZE);

    data action = PDU_to_buffer_user(GET, get_struct);

    memcpy(NLMSG_DATA(nlh_user), action, 68);

    //printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);

    //printf("Waiting for message from kernel\n");
    recvmsg(sock_fd, &msg, 0);

    PDU_struct * pdu = read_exactly_from_kernel(nlh_user);

    free(get_struct);
    free(action);
    free(pdu->data);
    free(pdu);
}

void insert_rhashtable(char* key){
    INSERT_struct *insert_struct = calloc(1,sizeof(INSERT_struct));
    insert_struct->OP_code=INSERT;
    memcpy(insert_struct->key, key, KEY_SIZE);
    insert_struct->data_bytes=strnlen(TEST_DATA, MAX_PAYLOAD)+1;
    insert_struct->data = calloc(1,(insert_struct->data_bytes));

    memcpy(insert_struct->data, TEST_DATA, insert_struct->data_bytes);
    data action = PDU_to_buffer_user(INSERT, insert_struct);

    memcpy(NLMSG_DATA(nlh_user), action, (insert_struct->data_bytes)+HEADERSIZE+KEY_SIZE);

    //printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);

    //printf("Waiting for message from kernel\n");
    recvmsg(sock_fd, &msg, 0);

    PDU_struct * pdu = read_exactly_from_kernel(nlh_user);

    free(insert_struct->data);
    free(insert_struct);
    free(action);
    free(pdu->data);
    free(pdu);
}

void init_rhashtable() {


    INIT_struct* init_struct = calloc(1, sizeof(INIT_struct));
    init_struct->OP_code = INIT;

    data action = PDU_to_buffer_user(INIT, init_struct);

    memcpy(NLMSG_DATA(nlh_user), action, HEADERSIZE);

    //printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);

    //printf("Waiting for message from kernel\n");
    recvmsg(sock_fd, &msg, 0);

    PDU_struct *pdu = read_exactly_from_kernel(nlh_user);

    free(init_struct);
    free(action);
    free(pdu->data);
    free(pdu);

}*/

int setup_netlink(){
    sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if(sock_fd<0)
        return -1;

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */

    memset(&dest_addr, 0, sizeof(dest_addr));
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;
    dest_addr.nl_groups = 0;

    nlh_user = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh_user, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh_user->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh_user->nlmsg_pid = getpid();
    nlh_user->nlmsg_flags = 0;

    iov.iov_base = (void *)nlh_user;
    iov.iov_len = nlh_user->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if(bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr))< 0){
        perror("Error: ");
        return -2;
    }

}


void reset_netlink(){

    //nlh_user = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh_user, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh_user->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh_user->nlmsg_pid = getpid();
    nlh_user->nlmsg_flags = 0;

    //printf("NLMSG_SPACE(MAX_PAYLOAD): %d\n", NLMSG_SPACE(MAX_PAYLOAD));
}
