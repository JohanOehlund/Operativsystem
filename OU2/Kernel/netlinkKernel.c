#include "netlinkKernel.h"
//https://lwn.net/Articles/751374/
//https://github.com/intel/linux-intel-4.9/blob/master/lib/test_rhashtable.c

extern int errno = 0;

static void recieve_data(struct sk_buff *skb) {

    nlh_kernel=(struct nlmsghdr*)skb->data;
    PDU_kernel_struct *response = read_exactly_from_user(nlmsg_data(nlh_kernel));
    if(response == NULL){
        printk(KERN_ERR "Error while reading from user space\n");
        return NULL;
    }
    data response_buffer = PDU_to_buffer_kernel(response);

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
    int res=nlmsg_unicast(nl_sk,skb_out,pid);

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
        case GET:
            read_GET_struct(response, request);
            break;
        case DELETE:
            read_DELETE_struct(response, request);
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
    memcpy(response_buffer, &pdu->data_bytes, 4);
    response_buffer+=4;
    memcpy(response_buffer, pdu->data, (pdu->data_bytes));
    return head;
}

static void read_DELETE_struct(PDU_kernel_struct *response, data request){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    char key[KEY_SIZE];
    request++;
    memcpy(key, request, KEY_SIZE);
    printk(KERN_INFO "GET key: %s\n", key);
    //struct test_obj *obj_get;
    struct test_obj *obj;
    obj = rhashtable_lookup_fast(&ht, &key, test_rht_params);
    if(obj == NULL){
        printk(KERN_ALERT "Error when getting object.\n");
        response->error = 1;
        response->data = "Error when getting object.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
        return;
    }

    int err = rhashtable_remove_fast(&ht, &obj->node, test_rht_params);
    if(err < 0){
        printk(KERN_ALERT "Error when deleting from rhashtable: %d\n", err);
        response->error = err;
        response->data = "Error when deleting from rhashtable.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }else{
        printk(KERN_INFO "Successful deleting object from rhashtable.");
        response->error = err;
        response->data = "Successful deleting object from rhashtable.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }
    kfree(obj->data);
    kfree(obj);

}

static void read_GET_struct(PDU_kernel_struct *response, data request){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    char key[KEY_SIZE];
    request++;
    memcpy(key, request, KEY_SIZE);
    printk(KERN_INFO "GET key: %s\n", key);
    //struct test_obj *obj_get;
    struct test_obj *obj_get;
    obj_get = rhashtable_lookup_fast(&ht, &key, test_rht_params);
    if(obj_get == NULL){
        printk(KERN_ALERT "Error when getting object.\n");
        response->error = 1;
        response->data = "Error when getting object.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
        return;
    }

    if (memcmp(&obj_get->key, &key, KEY_SIZE) == 0) {
        printk(KERN_INFO "Key matches.\n");
        response->error = 0;
        response->data = obj_get->data;
        response->data_bytes = strnlen((char*)response->data, MAX_PAYLOAD)+1;
    }else{
        printk(KERN_INFO "Key does not match...\n");
        response->error = 1;
        response->data = "Key does not match...";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }

}

static void read_INSERT_struct(PDU_kernel_struct *response, data request){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    struct test_obj *obj = kmalloc(sizeof(struct test_obj),GFP_KERNEL);
    if (!obj) {
        printk(KERN_ALERT "Error when kmalloc in function %s\n", __FUNCTION__);
        response->error = 1;
        response->data = "Error when kmalloc.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
        return NULL;
    }

    u32 data_bytes;
    request++;
    memcpy(obj->key, request, KEY_SIZE);
    printk(KERN_INFO "obj->key: %s\n", obj->key);
    request+=KEY_SIZE;
    memcpy(&data_bytes, request, 4);
    printk(KERN_INFO "data_bytes: %d\n", data_bytes);
    request+=4;
    obj->data = kmalloc(data_bytes,GFP_KERNEL);
    memcpy(obj->data, request, (data_bytes));
    printk(KERN_INFO "obj->data: %s\n", (char*)obj->data);

    int err = rhashtable_lookup_insert_fast(&ht, &obj->node, test_rht_params);
    if(err == -EEXIST){
        printk(KERN_ALERT "Error, duplicated key.\n");
        response->error = err;
        response->data = "Error, duplicated key.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }else if(err < 0){
        printk(KERN_ALERT "Error when insert to rhashtable: %d\n", errno);
        response->error = err;
        response->data = "Error when insert to rhashtable.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }else{
        printk(KERN_INFO "Inserted object to rhashtable");
        response->error = err;
        response->data = "Inserted object to rhashtable";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }


}

static void read_INIT_struct(PDU_kernel_struct *response, data data){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    int err = rhashtable_init(&ht, &test_rht_params);
    if (err < 0) {
        printk(KERN_ALERT "Unable to initialize hashtable: %d\n", err);
        response->error = 1;
        response->data = "Unable to initialize hashtable.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }else{
        printk(KERN_INFO "Initialized hashtable: %d\n", err);
        response->error = 0;
        response->data = "Initialized hashtable.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }

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
