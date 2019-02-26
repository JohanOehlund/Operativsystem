//
// Created by hasse on 2/25/19.
//

#ifndef OU2_NETLINKUSER_H
#define OU2_NETLINKUSER_H

#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include "PDU_user.h"

#define NETLINK_USER 31

#define MAX_PAYLOAD 1024 /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

size_t strnlen(const char *s, size_t maxlen);
void init_rhashtable();

#endif //OU2_NETLINKUSER_H
