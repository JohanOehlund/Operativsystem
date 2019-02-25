
#include "netlinkUser.h"



int main() {
    sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if(sock_fd<0)
        return -1;

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */

    bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    /*nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;


    strcpy(NLMSG_DATA(nlh), "Hello");
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    */
    init_rhashtable();


/* Read message from kernel */
    recvmsg(sock_fd, &msg, 0);
    printf("Received message payload: %s\n", (char *)NLMSG_DATA(nlh));
    close(sock_fd);
}

void init_rhashtable() {
    /*GEN_struct *gen_struct = calloc(1, sizeof(GEN_struct));
    gen_struct->OPCode = INIT;

    INIT_struct* init = calloc(1, sizeof(INIT_struct));
    init->OPCode = INIT;
    init->test = 255;
    gen_struct->created_struct=init;*/




    void* test = calloc(1,4);
    memset(test,INIT,1);
    test++;
    memset(test,255,1);
    test--;
    //memcpy(gen_struct->test,"hej", sizeof(char*));
    printf("test: %p\n", test);


    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    printf("bytes: %u\n", NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;


    memcpy(NLMSG_DATA(nlh), test, strlen(test));
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);
    printf("Waiting for message from kernel\n");


}
