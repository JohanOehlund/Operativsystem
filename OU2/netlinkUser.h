//
// Created by hasse on 2/25/19.
//

#ifndef OU2_NETLINKUSER_H
#define OU2_NETLINKUSER_H

#include "PDU_user.h"

#define NETLINK_USER 31

#define TEST_DATA	("Jag")


#define MAX_PAYLOAD 1024 /* maximum payload size*/
int sock_fd;
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh_user = NULL;
struct iovec iov;
struct msghdr msg;

size_t strnlen(const char *s, size_t maxlen);
void init_rhashtable();

void insert_rhashtable();

#endif //OU2_NETLINKUSER_H
