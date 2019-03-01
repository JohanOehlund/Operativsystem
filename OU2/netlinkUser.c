#include "netlinkUser.h"

int main(int argc, char **argv) {


    setup_netlink();

    init_rhashtable();


    test_rhashtable((void*)argv);

    free(nlh_user);
    close(sock_fd);
}
/*
[69833.065352] exiting hello module
[69833.092423] Entering: init
*/

void *test_rhashtable(void *arg) {
    uint16_t i = 10;
    for (i = 10; i < 15; i++) {
        reset_netlink();
        insert_rhashtable(i);
    }
    uint16_t j = 10;
    for (j = 10; j < 15; j++) {
        reset_netlink();
        get_rhashtable(j);
    }

    uint16_t k = 10;
    for (k = 10; k < 15; k++) {
        reset_netlink();
        delete_rhashtable(k);
    }

}

void delete_rhashtable(uint16_t key){
    printf("delete_rhashtable!\n");
    DELETE_struct *delete_struct = calloc(1,sizeof(DELETE_struct));

    delete_struct->OP_code = DELETE;
    delete_struct->key = key;

    data action = PDU_to_buffer_user(DELETE, delete_struct);

    memcpy(NLMSG_DATA(nlh_user), action, DELETE_HEADERSIZE);

    printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);

    printf("Waiting for message from kernel\n");
    recvmsg(sock_fd, &msg, 0);

    PDU_kernel_struct * pdu = read_exactly_from_kernel(nlh_user);

    free(delete_struct);
    free(action);
    free(pdu->data);
    free(pdu);
}

void get_rhashtable(uint16_t key){
    GET_struct *get_struct = calloc(1,sizeof(GET_struct));

    get_struct->OP_code = GET;
    get_struct->key = key;

    data action = PDU_to_buffer_user(GET, get_struct);

    memcpy(NLMSG_DATA(nlh_user), action, GET_HEADERSIZE);

    printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);

    printf("Waiting for message from kernel\n");
    recvmsg(sock_fd, &msg, 0);

    PDU_kernel_struct * pdu = read_exactly_from_kernel(nlh_user);

    free(get_struct);
    free(action);
    free(pdu->data);
    free(pdu);
}

void insert_rhashtable(uint16_t key){
    INSERT_struct *insert_struct = calloc(1,sizeof(INSERT_struct));
    insert_struct->OP_code=INSERT;
    insert_struct->key=key;
    insert_struct->data_bytes=strnlen(TEST_DATA, MAX_PAYLOAD)+1;
    insert_struct->data = calloc(1,(insert_struct->data_bytes));

    memcpy(insert_struct->data, TEST_DATA, insert_struct->data_bytes);
    data action = PDU_to_buffer_user(INSERT, insert_struct);

    memcpy(NLMSG_DATA(nlh_user), action, (insert_struct->data_bytes)+INSERT_HEADERSIZE);

    printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);

    printf("Waiting for message from kernel\n");
    recvmsg(sock_fd, &msg, 0);

    PDU_kernel_struct * pdu = read_exactly_from_kernel(nlh_user);

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

    memcpy(NLMSG_DATA(nlh_user), action, INIT_HEADERSIZE);

    printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);

    printf("Waiting for message from kernel\n");
    recvmsg(sock_fd, &msg, 0);

    PDU_kernel_struct *pdu = read_exactly_from_kernel(nlh_user);

    free(init_struct);
    free(action);
    free(pdu->data);
    free(pdu);

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
