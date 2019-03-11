//
// Created by c15jod on 2017-09-19.
//

#ifndef DODOU2_SERVER_H
#define DODOU2_SERVER_H
#include "../Resources/socket.h"

#define THREADS 10
#define NETLINK_USER_SEND 30
#define NETLINK_USER_RECEIVE 31
#define STORE_FILE "stored_values.txt"
#define TMP_FILE "delete-line.tmp"
int sock_fd_send;
struct nlmsghdr *nlh_user_send = NULL;
struct sockaddr_nl src_addr_send, dest_addr_send;
struct iovec iov_send;
struct msghdr msg_send;

int sock_fd_receive;
struct nlmsghdr *nlh_user_receive = NULL;
struct sockaddr_nl src_addr_receive, dest_addr_receive;
struct iovec iov_receive;
struct msghdr msg_receive;



typedef struct clientThreadInfo {
    int thread_num;
    int client_sock;

}clientThreadInfo;

void init_hashtable();

data accept_connections(void *arg);

data client_listener(void *arg);

int setup_listening_socket(char **argv);

int *find_free_thread();

void print_wrong_params(char *progName);

void close_connected_client(int client_sock, int cliC);

data send_to_clients(void *arg);

PDU_struct *client_JOIN(int client_sock);

void close_server();

data server_writer(void *arg);

data join_threads(void *arg);

void store_data(PDU_struct *PDU_struct_SEND, PDU_struct *PDU_struct_RECEIVE);

void load_stored_values();

size_t strnlen(const char *s, size_t maxlen);

data kernel_communication(data arg);

int setup_netlink_send();
void reset_netlink_send();

int setup_netlink_receive();
void reset_netlink_receive();

#endif //DODOU2_SERVER_H
