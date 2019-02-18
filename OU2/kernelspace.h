//
// Created by root on 2/18/19.
//

#ifndef OU2_KERNELSPACE_H
#define OU2_KERNELSPACE_H

/*#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <net/netlink.h>
#include <net/net_namespace.h>*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <net/sock.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <lzma.h>

/* Protocol family, consistent in both kernel prog and user prog. */
#define MYPROTO NETLINK_USERSOCK
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYGRP 21

static struct sock *nl_sk = NULL;
static void send_to_user(void);
static int __init hello_init(void);
static void __exit hello_exit(void)

#endif //OU2_KERNELSPACE_H
