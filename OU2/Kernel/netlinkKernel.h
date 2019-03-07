//
// Created by hasse on 2/25/19.
//

#ifndef OU2_NETLINKKERNEL_H
#define OU2_NETLINKKERNEL_H

#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/jhash.h>
#include <linux/rhashtable.h>
#include "../Resources/PDU_kernel.h"
//dmesg | tail

#define MAX_ENTRIES	1000000
#define HT_SIZE	8
#define NETLINK_USER 31
#define HEADERSIZE 4

#define TEST_DATA ("Jag heter HASSE!!!")

struct sock *nl_sk;

static struct rhashtable ht;

struct nlmsghdr *nlh_kernel;
int pid;
struct sk_buff *skb_out;
int msg_size;



static void recieve_data(struct sk_buff *skb);

static PDU_struct *read_exactly_from_user(data data);

static data PDU_to_buffer_kernel(PDU_struct *pdu);

static void read_DELETE_struct(PDU_struct *response, data request);

static void read_INSERT_struct(PDU_struct *response, data data);

static void read_INIT_struct(PDU_struct *response, data data);

static void read_GET_struct(PDU_struct *response, data request);

static int __init init(void);

static void __exit exit(void);



struct rhash_object {
	char key[KEY_SIZE];
	struct rhash_head node;
	u16 data_bytes;
	data data;
};

static const struct rhashtable_params test_rht_params = {
	.nelem_hint = HT_SIZE,
	.head_offset = offsetof(struct rhash_object, node),
	.key_offset = offsetof(struct rhash_object, key),
	.key_len = KEY_SIZE,
	.hashfn = jhash,
	.nulls_base = (3U << RHT_BASE_SHIFT),
};


module_init(init);
module_exit(exit);


MODULE_LICENSE("GPL");

#endif //OU2_NETLINKKERNEL_H
