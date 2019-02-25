//
// Created by hasse on 2/25/19.
//

#ifndef OU2_NETLINKKERNEL_H
#define OU2_NETLINKKERNEL_H

#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include "PDU_kernel.h"

//dmesg | tail

#define NETLINK_USER 31

struct sock *nl_sk;

static void recieve_data(struct sk_buff *skb);

static void *read_exactly(void *data);

static INIT_struct* read_INIT_struct(void* data);

static int __init init(void);

static void __exit exit(void);


module_init(init);
module_exit(exit);


MODULE_LICENSE("GPL");

#endif //OU2_NETLINKKERNEL_H
