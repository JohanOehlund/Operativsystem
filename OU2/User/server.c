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

    if(pthread_create(&trd[0], NULL, acceptConnections, (void*)argv)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }


    if(pthread_join(trd[0],NULL) != 0) {
            perror("thread join");
            exit(EXIT_FAILURE);
        }



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

    while(1){
        usleep(1000000);

        PDU_struct *PDU_struct = receive_pdu(temp_socket);
        printf("OP: %u\n", PDU_struct->OP_code);
        printf("Numbytes: %zu\n", PDU_struct->numbytes);
        reset_netlink();
        memcpy(NLMSG_DATA(nlh_user), PDU_struct->pdu, PDU_struct->numbytes);

        sendmsg(sock_fd,&msg,0);

        //printf("Waiting for message from kernel\n");
        recvmsg(sock_fd, &msg, 0);

        PDU_kernel_struct *pdu = read_exactly_from_kernel(nlh_user);


        /*data temp_data = calloc(1,30);
        memcpy(temp_data, pdu, 30);
        printf("temp_data: %s\n", temp_data);*/

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
