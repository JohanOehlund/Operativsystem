#include "netlinkKernel.h"
//https://lwn.net/Articles/751374/
//https://github.com/intel/linux-intel-4.9/blob/master/lib/test_rhashtable.c


static void recieve_data(struct sk_buff *skb) {
    //char *msg="Hello from kernel";
    int res;



    nlh_kernel=(struct nlmsghdr*)skb->data;
    PDU_kernel_struct *response = read_exactly_from_user(nlmsg_data(nlh_kernel));
    if(response == NULL){
        printk(KERN_ERR "Error while reading from user space\n");
        return NULL;
    }
    data response_buffer = PDU_to_buffer_kernel(response);
    //INIT_struct *test2 = (INIT_struct*)test->created_struct;
    //printk(KERN_INFO "Netlink received <hhhhhhhh> Opcode: %s\n",test);
    //printk(KERN_INFO "Netlink received GEN_struct test: %s\n",test->test);
    pid = nlh_kernel->nlmsg_pid; //pid of sending process
    msg_size=(response->data_bytes)+KERNEL_HEADERSIZE;
    skb_out = nlmsg_new(msg_size,0);

    if(!skb_out){
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    nlh_kernel=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);

    NETLINK_CB(skb_out).dst_group = 0;
    memcpy(nlmsg_data(nlh_kernel), response_buffer, msg_size);
    res=nlmsg_unicast(nl_sk,skb_out,pid);

    if(res<0)
        printk(KERN_ERR "Error while sending bak to user\n");
}


/* Reads the desired PDU.
* @param    data - The data.
* @return   pdu - The pdu.
*/
static PDU_kernel_struct *read_exactly_from_user(data request){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    PDU_kernel_struct *response  = kmalloc(sizeof(PDU_kernel_struct), GFP_KERNEL);
    if (!response){
        printk(KERN_ALERT "Memory allocation failed.\n");
        return NULL;
    }
    uint8_t OP_code;
    memcpy(&OP_code,request,1);
    printk(KERN_ALERT "OP_code %u\n", OP_code);
    switch (OP_code){
        case INIT:
            read_INIT_struct(response, request);
            break;
        case INSERT:
            read_INSERT_struct(response, request);
            break;

        default:
            printk(KERN_ALERT "Error creating PDU.\n");
            return NULL;
    }
    return response;
}


static data PDU_to_buffer_kernel(PDU_kernel_struct *pdu){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    printk(KERN_INFO "error kernel: %u\n", pdu->error);
    printk(KERN_INFO "data kernel: %s\n", (char*)pdu->data);
    data response_buffer = kmalloc(((pdu->data_bytes)+KERNEL_HEADERSIZE), GFP_KERNEL);
    data head = response_buffer;
    memcpy(response_buffer, &pdu->error, 1);
    response_buffer++;
    memcpy(response_buffer, &pdu->data_bytes, 2);
    response_buffer+=2;
    memcpy(response_buffer, pdu->data, (pdu->data_bytes));
    return head;
}



static void read_INSERT_struct(PDU_kernel_struct *response, data request){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    struct test_obj *obj = kmalloc(sizeof(struct test_obj),GFP_KERNEL);
    if (!obj) {
        printk(KERN_ALERT "Error when kmalloc in function %s\n", __FUNCTION__);
        response->error = 1;
        response->data = "Error when kmalloc.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD);
        return NULL;
    }
    /*
    uint8_t OP_code;
    uint16_t key;
    uint16_t data_bytes;
    data data;
    */
    uint16_t data_bytes;
    request++;
    memcpy(&obj->key, request, 2);
    printk(KERN_INFO "obj->key: %u\n", obj->key);
    request+=2;
    memcpy(&data_bytes, request, 2);
    printk(KERN_INFO "data_bytes: %d\n", data_bytes);
    request+=2;
    obj->data = kmalloc(data_bytes,GFP_KERNEL);
    memcpy(obj->data, request, (data_bytes));
    printk(KERN_INFO "obj->data: %s\n", (char*)obj->data);

    /*int err = rhashtable_insert_fast(&ht, &obj->node, test_rht_params);
    if(err < 0){
        printk(KERN_ALERT "Error when insert to rhashtable: %d\n", err);
        response->error = err;
        response->data = "Error when insert to rhashtable.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD);
    }else{
        printk(KERN_INFO "Inserted object to rhashtable");
        response->error = err;
        response->data = "Inserted object to rhashtable";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD);
    }*/
    //printk(KERN_INFO "Inserted object to rhashtable");
    response->error = 1;
    response->data = "TESTING!!!!!!";
    response->data_bytes = strnlen(response->data, MAX_PAYLOAD);
}

static void read_INIT_struct(PDU_kernel_struct *response, data data){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    int err = rhashtable_init(&ht, &test_rht_params);
    if (err < 0) {
        printk(KERN_ALERT "Unable to initialize hashtable: %d\n", err);
        response->error = 1;
        response->data = "Unable to initialize hashtable.";
        response->data_bytes = strlen(response->data);
    }else{
        printk(KERN_INFO "Initialized hashtable: %d\n", err);
        response->error = 0;
        response->data = "Initialized hashtable.";
        response->data_bytes = strlen(response->data);
    }

}


static int insert_retry(struct rhashtable *ht, struct rhash_head *obj,
    const struct rhashtable_params params){
        printk("Entering: %s\n",__FUNCTION__);
        int err, retries = -1;
        int enomem_retries = 0;
        err = rhashtable_insert_fast(ht, obj, params);
        if (err == -ENOMEM){
            printk(KERN_INFO "err: %d\n", err);
        }
        printk(KERN_INFO "err value: %d\n", err);

        /*do {
        retries++;
        cond_resched();
        err = rhashtable_insert_fast(ht, obj, params);
        if (err == -ENOMEM && enomem_retry == 1) {
        enomem_retries++;
        err = -EBUSY;
    }
} while (err == -EBUSY);*/

return 0;
}



static int __init init(void) {
    printk("Entering: %s\n",__FUNCTION__);
    /* This is for 3.6 kernels and above.*/
    struct netlink_kernel_cfg cfg = {
        .input = recieve_data,
    };

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);

    if(!nl_sk)
    {

        printk(KERN_ALERT "Error creating socket.\n");
        return -10;

    }

    return 0;
}

static void __exit exit(void) {

    printk(KERN_INFO "exiting hello module\n");
    netlink_kernel_release(nl_sk);
}
