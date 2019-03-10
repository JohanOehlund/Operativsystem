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
        print_wrong_params(argv[0]);
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
    setup_netlink_send();
    setup_netlink_receive();


    closeAllClients = false;
    exitServer=false;
    if (pthread_mutex_init(&lock, NULL) != 0){
        fprintf(stderr,"\n mutex init failed\n");
        return 1;
    }

    if(pthread_create(&trd[0], NULL, join_threads, NULL)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&trd[1], NULL, send_to_clients, (void*)argv)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&trd[2], NULL, server_writer, NULL)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&trd[3], NULL, accept_connections, (void*)argv)!=0){
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

void close_server() {
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
void *join_threads(void *arg) {
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
void *server_writer(void *arg) {
    printf("To close server type <%s>\n",EXIT);
    while (1){
        char message[MAXMESSLEN+1];
        fgets(message,MAXMESSLEN,stdin);
        if(strncmp(message, "\n", MAXMESSLEN)==0)
            continue;
        if(strncmp(message, EXIT, 4)==0) {
            printf("Closing server.\n");
            close_server();
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
void *send_to_clients(void *arg){

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
int setup_listening_socket(char **argv) {

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
void *accept_connections(void *arg) {
    char **argv = arg;


    int temp2;
    int *thread_number;
    int sock=setup_listening_socket(argv);
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
        if((thread_number=find_free_thread()) == (int *) -5) {
            shutdown(temp2, SHUT_RDWR); //Server is full
            close(temp2);
            free(client_sock);
            continue;
        }
        if(!closeAllClients) {
            clientThreadInfo *cti = calloc(1, sizeof(clientThreadInfo));
            cti->client_sock = *client_sock;
            cti->thread_num = *thread_number;
            if (pthread_create(&clientThreads[*thread_number], NULL, client_listener, cti) != 0) {
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
int *find_free_thread() {

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
void *client_listener(void *arg){
    clientThreadInfo *cti = (clientThreadInfo *)arg;
    int temp_socket = cti->client_sock;

    bool exitLoop=false;
    PDU_struct *pdu_JOIN = NULL;
    pdu_JOIN = client_JOIN(temp_socket);
    JOIN_struct *join_struct = (JOIN_struct*) pdu_JOIN->data;
    printf("Received JOIN from user: %s\n", join_struct->client_ID);


    while(1) {
        reset_netlink_send();
        reset_netlink_receive();

        PDU_struct *PDU_struct_SEND = NULL;
        PDU_struct *PDU_struct_RECEIVE = NULL;
        /*if(llist_isEmpty(clients_sockets)){
            llist_position p3=llist_first(client_ids);
            for (int j = 0; j < list_length(client_ids) ; ++j) {
                llist_remove(client_ids, p3);
                p3 = llist_next(p3);
            }
        }*/

        PDU_struct_SEND = receive_pdu(temp_socket);
        if((int *)PDU_struct_SEND==(int*)-100){
            //llist_insertfirst(terminatedThreads, &cti->thread_num);
            close_connected_client(temp_socket, 1);
            return NULL;
        }
        memcpy(NLMSG_DATA(nlh_user_send), PDU_struct_SEND->data, PDU_struct_SEND->data_bytes);
        printf("Sending message to kernel\n");
        sendmsg(sock_fd_send,&msg_send,0);

        printf("Waiting for message from kernel\n");
        recvmsg(sock_fd_receive, &msg_receive, 0);

        PDU_struct_RECEIVE = read_exactly_from_kernel(nlh_user_receive);
        //printf("Received message opcode: %u\n", PDU_struct_RECEIVE->OP_code);
        //printf("Received message data_bytes: %u\n", PDU_struct_RECEIVE->data_bytes);
        send_pdu(temp_socket, PDU_struct_RECEIVE);

        free_struct(KERNEL, PDU_struct_RECEIVE);
        free_struct(KERNEL, PDU_struct_SEND);
        reset_netlink_send();
        usleep(2000);

    }
    close_connected_client(temp_socket, 0);
    //llist_insertfirst(terminatedThreads, &cti->thread_num);
    return NULL;
}

/* Closes a connected client
 * @param client_sock - The socket to close connection with.
 * @param gen_struct - A struct that contains closing sockets name.
 * @param cliC - If cliC=1 then client_sock already closed connection.
 * @return
 */
void close_connected_client(int client_sock, int cliC) {

    //JOIN_struct *join_struct=gen_struct->created_struct;
    int socket = 0;
    char *temp_client = NULL;
    /*if(cliC == 0)
        sendQUIT(client_sock);*/

    pthread_mutex_lock(&lock);
    llist_position p = llist_first(clients_sockets);
    /*for (int i = 0; i < list_length(clients_sockets); ++i) {
        socket = *(int*)llist_inspect(p, clients_sockets);
        if(client_sock == socket) {
            llist_remove(clients_sockets, p);
            break;
        }
        p = llist_next(p);
    }*/
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
PDU_struct *client_JOIN(int client_sock){
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

void print_wrong_params(char *progName) {
    fprintf(stderr,
            "%s\n%s %s\n",
            "Invalid parameters",
            progName,
            "<PORT> <SERVERNAME>");
}


int setup_netlink_send(){
    sock_fd_send=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER_SEND);
    printf("sock_fd_send: %d\n", sock_fd_send);
    if(sock_fd_send<0){
        perror("Error creating socket: ");
        return -1;
    }


    memset(&src_addr_send, 0, sizeof(src_addr_send));
    src_addr_send.nl_family = AF_NETLINK;
    src_addr_send.nl_pid = getpid(); /* self pid */

    memset(&dest_addr_send, 0, sizeof(src_addr_send));
    memset(&src_addr_send, 0, sizeof(src_addr_send));
    src_addr_send.nl_family = AF_NETLINK;
    src_addr_send.nl_pid = 0;
    src_addr_send.nl_groups = 0;

    nlh_user_send = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh_user_send, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh_user_send->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh_user_send->nlmsg_pid = getpid();
    nlh_user_send->nlmsg_flags = 0;

    iov_send.iov_base = (void *)nlh_user_send;
    iov_send.iov_len = nlh_user_send->nlmsg_len;
    msg_send.msg_name = (void *)&src_addr_send;
    msg_send.msg_namelen = sizeof(src_addr_send);
    msg_send.msg_iov = &iov_send;
    msg_send.msg_iovlen = 1;

    if(bind(sock_fd_send, (struct sockaddr*)&src_addr_send, sizeof(src_addr_send))< 0){
        perror("Error: ");
        return -2;
    }
    return sock_fd_send;
}

int setup_netlink_receive(){
    sock_fd_receive=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER_RECEIVE);
    printf("sock_fd_receive: %d\n", sock_fd_receive);
    if(sock_fd_receive<0){
        perror("Error creating socket: ");
        return -1;
    }

    memset(&src_addr_receive, 0, sizeof(src_addr_receive));
    src_addr_receive.nl_family = AF_NETLINK;
    src_addr_receive.nl_pid = getpid(); /* self pid */

    memset(&dest_addr_receive, 0, sizeof(dest_addr_receive));
    memset(&dest_addr_receive, 0, sizeof(dest_addr_receive));
    dest_addr_receive.nl_family = AF_NETLINK;
    dest_addr_receive.nl_pid = 0;
    dest_addr_receive.nl_groups = 0;

    nlh_user_receive = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh_user_receive, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh_user_receive->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh_user_receive->nlmsg_pid = getpid();
    nlh_user_receive->nlmsg_flags = 0;

    iov_receive.iov_base = (void *)nlh_user_receive;
    iov_receive.iov_len = nlh_user_receive->nlmsg_len;
    msg_receive.msg_name = (void *)&dest_addr_receive;
    msg_receive.msg_namelen = sizeof(dest_addr_receive);
    msg_receive.msg_iov = &iov_receive;
    msg_receive.msg_iovlen = 1;

    if(bind(sock_fd_receive, (struct sockaddr*)&src_addr_receive, sizeof(src_addr_receive))< 0){
        perror("Error: ");
        return -2;
    }

}


void reset_netlink_send(){

    //nlh_user = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh_user_send, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh_user_send->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh_user_send->nlmsg_pid = getpid();
    nlh_user_send->nlmsg_flags = 0;

    //printf("NLMSG_SPACE(MAX_PAYLOAD): %d\n", NLMSG_SPACE(MAX_PAYLOAD));
}


void reset_netlink_receive(){

    //nlh_user = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh_user_receive, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh_user_receive->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh_user_receive->nlmsg_pid = getpid();
    nlh_user_receive->nlmsg_flags = 0;

    //printf("NLMSG_SPACE(MAX_PAYLOAD): %d\n", NLMSG_SPACE(MAX_PAYLOAD));
}
