//
// Created by hasse on 2/25/19.
//

#ifndef OU2_NETLINKUSER_H
#define OU2_NETLINKUSER_H

#include "../Resources/PDU_user.h"
#define NETLINK_USER 31

int sock_fd;
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh_user = NULL;
struct iovec iov;
struct msghdr msg;


size_t strnlen(const char *s, size_t maxlen);

void *test_rhashtable(data arg);
void delete_rhashtable(char* key);
void get_rhashtable(char* key);
void init_rhashtable();
void insert_rhashtable(char* key);


int setup_netlink();
void reset_netlink();



#endif //OU2_NETLINKUSER_H
