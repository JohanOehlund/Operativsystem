#include "server.h"

pthread_mutex_t lock;
llist *clients_sockets;
llist *sender_list;
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
    availableThreads = llist_empty();
    terminatedThreads = llist_empty();
    pthread_t trd[5];
    closeAllClients = false;
    exitServer=false;
    setup_netlink_send();
    setup_netlink_receive();
    init_hashtable();

    if (pthread_mutex_init(&lock, NULL) != 0){
        fprintf(stderr,"\n mutex init failed\n");
        return 1;
    }

    if(pthread_create(&trd[0], NULL, join_threads, NULL)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    /*if(pthread_create(&trd[1], NULL, send_to_clients, (void*)argv)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }*/
    if(pthread_create(&trd[1], NULL, server_writer, NULL)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    if(pthread_create(&trd[2], NULL, kernel_communication, NULL)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&trd[3], NULL, accept_connections, (void*)argv)!=0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }



    for (int i = 0; i < 2; ++i) {
        printf("join trd[%d]\n", i);
        if(pthread_join(trd[i],NULL) != 0) {
            perror("thread join");
            exit(EXIT_FAILURE);
        }
    }
    if(pthread_cancel(trd[2])<0){
        perror("pthread_cancel");
        exit(EXIT_FAILURE);
    }
    if(pthread_cancel(trd[3])<0){
        perror("pthread_cancel");
        exit(EXIT_FAILURE);
    }
    if(pthread_join(trd[2],NULL) != 0) {
        perror("thread join");
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
    llist_free(availableThreads);
    llist_free(terminatedThreads);
    llist_free(sender_list);





    //test_rhashtable((void*)0);
    return 0;
}

data kernel_communication(data arg){
    while(!exitServer){
        reset_netlink_send();
        reset_netlink_receive();
        if(llist_isEmpty(sender_list)){
           pthread_mutex_lock(&sender_list->mtx);
           pthread_cond_wait(&sender_list->cond, &sender_list->mtx);
           pthread_mutex_unlock(&sender_list->mtx);
           if(exitServer)
               return NULL;
       }
       PDU_struct *pdu_send=llist_removefirst_PDU(sender_list);

       memcpy(NLMSG_DATA(nlh_user_send), pdu_send->data, pdu_send->data_bytes);
       sendmsg(sock_fd_send,&msg_send,0);
       recvmsg(sock_fd_receive, &msg_receive, 0);

       PDU_struct *pdu_recieve = read_exactly_from_kernel(nlh_user_receive);
       store_data(pdu_send, pdu_recieve);

       llist_position p=llist_first(clients_sockets);
       for (int i = 0; i < list_length(clients_sockets) ; ++i) {
           int temp_socket=*(int *)llist_inspect(p,clients_sockets);
           send_pdu(temp_socket, pdu_recieve);
           p=llist_next(p);
       }

       free_struct(USER, pdu_send);
       free_struct(KERNEL, pdu_recieve);
    }

}


void init_hashtable() {
    PDU_struct *PDU_struct_INIT = calloc(1, sizeof(PDU_struct));
    PDU_struct_INIT->OP_code = KERNEL;
    PDU_struct_INIT->data_bytes = HEADERSIZE;
    PDU_struct_INIT->data = calloc(1, HEADERSIZE);
    memset(PDU_struct_INIT->data, INIT, 1);
    memcpy(NLMSG_DATA(nlh_user_send), PDU_struct_INIT->data, PDU_struct_INIT->data_bytes);
    printf("Sending init to kernel\n");
    sendmsg(sock_fd_send,&msg_send,0);

    recvmsg(sock_fd_receive, &msg_receive, 0);
    printf("Init OK from kernel\n");

    load_stored_values();


    free_struct(KERNEL, PDU_struct_INIT);
}

void load_stored_values(){
    regex_t regex;
    int reti;
    char msgbuf[KEY_SIZE];
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char *token;
    FILE *srcFile;
    srcFile = fopen(STORE_FILE, "r");
    if (srcFile == NULL){
        printf("Unable to open file(s).\n");
        printf("Please check you have read/write previleges.\n");
        return;
    }

    reti = regcomp(&regex, ".*:.*", 0);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }


    while ((read = getline(&line, &len, srcFile)) != -1) {
        reti = regexec(&regex, line, 0, NULL, 0);
        if (!reti) {
            PDU_struct *PDU_struct = calloc(1,sizeof(PDU_struct)+sizeof(data));
            INSERT_struct *insert_struct = calloc(1, sizeof(INSERT_struct));
            /* walk through other tokens */
            int token_num = 0;

            /* get the first token */
            token = strtok(line, ":");
            while( token != NULL ) {
                if(token_num == 0){
                    strncpy(insert_struct->key, token, KEY_SIZE);
                }else if(token_num == 1){
                    insert_struct->data_bytes = strlen(token)+1;
                    insert_struct->data = calloc(1, insert_struct->data_bytes);
                    strncpy(insert_struct->data, token, insert_struct->data_bytes);
                }

                token = strtok(NULL, ":");
                token_num++;
            }
            insert_struct->OP_code = INSERT;

            void *action = PDU_to_buffer_user(INSERT, insert_struct); // 'action' kan inte vara data måste vara void*....

            PDU_struct->OP_code = USER;
            PDU_struct->data_bytes = (insert_struct->data_bytes) + HEADERSIZE + KEY_SIZE;
            PDU_struct->data = action;

            memcpy(NLMSG_DATA(nlh_user_send), PDU_struct->data, PDU_struct->data_bytes);
            sendmsg(sock_fd_send,&msg_send,0);
            recvmsg(sock_fd_receive, &msg_receive, 0);

            free_struct(INSERT, insert_struct);
            free_struct(USER,PDU_struct);

        }
        else if (reti == REG_NOMATCH) {
            puts("No match");
        }
        else {
            regerror(reti, &regex, msgbuf, sizeof(msgbuf));
            fprintf(stderr, "Regex match failed: %s\n", msgbuf);
            exit(1);
        }
        //printf("Retrieved line of length %zu:\n", read);
        //printf("%s", line);
    }

    regfree(&regex);

}

void store_data(PDU_struct *PDU_struct_SEND, PDU_struct *PDU_struct_RECEIVE) {

    data recieve_data = PDU_struct_RECEIVE->data;
    data send_data = PDU_struct_SEND->data;
    //data head_recieve = recieve_data;
    //data head_send = send_data;
    uint8_t OP_code;
    uint16_t mess_len;
    char key[KEY_SIZE+1];
    key[KEY_SIZE] = '\0';
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    FILE *srcFile;
    FILE *tempFile;
    srcFile = fopen(STORE_FILE, "a+");
    tempFile = fopen(TMP_FILE, "w+");
    if (srcFile == NULL || tempFile == NULL){
        printf("Unable to open file(s).\n");
        printf("Please check you have read/write previleges.\n");

        exit(EXIT_FAILURE);
    }
    recieve_data+=HEADERSIZE;

    if(strncmp((char*)recieve_data, "Error", 5) == 0) {
        return;
    }
    else {
        memcpy(&OP_code, send_data, 1);
        send_data++;
        memcpy(&mess_len, send_data, 2);
        send_data+=3;
        memcpy(key, send_data, KEY_SIZE);
        if(OP_code == INSERT) {

            char mess[mess_len];
            send_data+=KEY_SIZE;
            memcpy(mess, send_data, mess_len);

            fprintf(srcFile, "%s:%s\n", key, mess);
            fclose(srcFile);

        }
        else if(OP_code == DELETE) {
            char *pt;
            while ((read = getline(&line, &len, srcFile)) != -1) {
                char copy[read];
                strncpy(copy, line, read);
                pt = strtok(copy,":");
                if(strcmp(pt, key) != 0){
                    fprintf(tempFile, "%s", line);
                }
            }
            // Move src file pointer to beginning
            rewind(tempFile);

            fclose(srcFile);
            FILE *newFile;
            newFile = fopen(STORE_FILE, "w");
            while ((read = getline(&line, &len, tempFile)) != -1) {
                fprintf(newFile, "%s", line);
            }
            fclose(newFile);
            fclose(tempFile);
        }

    }

}

void close_server() {
    PDU_struct *quit = create_QUIT_pdu();
    while(!llist_isEmpty(clients_sockets)) {
        int temp_socket= llist_removefirst(clients_sockets);
        printf("temp_socket i close server %d\n", temp_socket);
        send_pdu(temp_socket, quit);
    }
    free_struct(USER, quit);
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
            llist_insertlast(clients_sockets, &cti->client_sock);
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
    printf("Received JOIN from user: %s with socket number: %d\n", join_struct->client_ID, temp_socket);


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

        PDU_struct_SEND = receive_pdu(temp_socket);
        if((int *)PDU_struct_SEND==(int*)-100){
            llist_insertfirst(terminatedThreads, &cti->thread_num);
            pthread_cond_broadcast(&terminatedThreads->cond);
            close_connected_client(temp_socket, 1);
            return NULL;
        }

        llist_insertlast(sender_list, PDU_struct_SEND);
        pthread_cond_broadcast(&sender_list->cond);
    }
    close_connected_client(temp_socket, 0);
    llist_insertfirst(terminatedThreads, &cti->thread_num);
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
    //if(cliC == 0)
        //sendQUIT(client_sock);

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
