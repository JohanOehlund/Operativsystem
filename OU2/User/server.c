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

    setup_netlink();
    /*if(pthread_create(&trd[0], NULL, acceptConnections, (void*)argv)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }


    if(pthread_join(trd[0],NULL) != 0) {
        perror("thread join");
        exit(EXIT_FAILURE);
    }*/

    acceptConnections((void*)argv);
    //test_rhashtable((void*)0);
    return 0;
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
    /*for (int j = 0; j < THREADS ; ++j) {
        int *temp=calloc(1,sizeof(int *));
        memcpy(temp,&j,sizeof(int *));
        llist_insertlast(availableThreads, temp);
    }*/
    while(1){
        temp2 = accept(sock,0, 0);
        //int *client_sock = calloc(1, sizeof(int *));
        //memcpy(client_sock, &temp2, sizeof(int));

        if(temp2 < 0){
            perror("Receiver - accept failed");
            exit(EXIT_FAILURE);
        }
        /*if((thread_number=findFreeThread()) == (int *) -5) {
            shutdown(temp2, SHUT_RDWR); //Server is full
            close(temp2);
            //free(client_sock);
            continue;
        }*/
        if(!closeAllClients) {
            clientThreadInfo *cti = calloc(1, sizeof(clientThreadInfo));
            cti->client_sock = temp2;
            cti->thread_num = 0;
            if (pthread_create(&clientThreads[0], NULL, clientlistener, cti) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }else {
            printf("Server is closing refuse new connections\n"); //Closing server
            //shutdown(*client_sock, SHUT_RDWR);
            //close(*client_sock);

        }

        //free(thread_number);
        //free(client_sock);
    }

    return NULL;
}

/* Listens for PDU:s from client.
 * @param arg - A struct that contains information about client.
 * @return
 * @comment If client leaves then thread terminates and adds its number in the "terminatedthreads" list.
 */
void *clientlistener(void *arg){

    clientThreadInfo *cti = (clientThreadInfo *)arg;
    int temp_socket = cti->client_sock;
    PDU_struct *pdu = NULL;
    PDU_struct *PDU_struct = NULL;


    while(1){

        usleep(1000000);
        reset_netlink();
        PDU_struct = receive_pdu(temp_socket);

        /*data t1 = calloc(1,HEADERSIZE);
        memset(t1, INIT, 1);

        printf("OP in clientlistener: %u\n", PDU_struct->OP_code);
        printf("Numbytes: %zu\n", PDU_struct->numbytes);*/
        memcpy(NLMSG_DATA(nlh_user), PDU_struct->data, PDU_struct->data_bytes);

        printf("Sending message to kernel\n");
        sendmsg(sock_fd,&msg,0);

        printf("Waiting for message from kernel\n");
        recvmsg(sock_fd, &msg, 0);

        pdu = read_exactly_from_kernel(nlh_user);


        /*data temp_data = calloc(1,30);
        memcpy(temp_data, pdu, 30);
        printf("temp_data: %s\n", temp_data);*/
        send_pdu(temp_socket, pdu);
    }

    llist_insertfirst(terminatedThreads, &cti->thread_num);
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
