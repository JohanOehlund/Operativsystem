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
#include "PDU_kernel.h"
//dmesg | tail

#define MAX_ENTRIES	1000000
#define TEST_HT_SIZE	8
#define TEST_PTR	("Jag heter Hasse och jag gillar att fiska fggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg...")

#define NETLINK_USER 31

struct sock *nl_sk;






static void recieve_data(struct sk_buff *skb);

static PDU_kernel_struct *read_exactly(data data);

static void read_INIT_struct(PDU_kernel_struct *response, data data);

static int insert_retry(struct rhashtable *ht, struct rhash_head *obj,
                        const struct rhashtable_params params);

static int __init init(void);

static void __exit exit(void);

static struct rhashtable ht;

struct test_obj {
	void *data;
	int key;
	struct rhash_head	node;
};

static const struct rhashtable_params test_rht_params = {
	.nelem_hint = TEST_HT_SIZE,
	.head_offset = offsetof(struct test_obj, node),
	.key_offset = offsetof(struct test_obj, key),
	.key_len = sizeof(int),
	.hashfn = jhash,
	.nulls_base = (3U << RHT_BASE_SHIFT),
};

static struct test_obj array[MAX_ENTRIES];



module_init(init);
module_exit(exit);


MODULE_LICENSE("GPL");

#endif //OU2_NETLINKKERNEL_H
