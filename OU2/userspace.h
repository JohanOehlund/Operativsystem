//
// Created by root on 2/18/19.
//

#ifndef OU2_USERSPACE_H
#define OU2_USERSPACE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>

/* Protocol family, consistent in both kernel prog and user prog. */
#define MYPROTO NETLINK_USERSOCK
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYMGRP 21

int open_netlink(void);
void read_event(int sock);


#endif //OU2_USERSPACE_H
