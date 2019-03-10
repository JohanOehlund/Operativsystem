#include "netlinkKernel.h"
//https://lwn.net/Articles/751374/
//https://github.com/intel/linux-intel-4.9/blob/master/lib/test_rhashtable.c

static void recieve_data(struct sk_buff *skb) {
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    /*struct sk_buff *copy = skb_unshare(skb, GFP_KERNEL);
    if(copy == NULL){
        printk(KERN_ERR "Error while copying skb struct...\n");
        return NULL;
    }*/
    struct work_data * my_data;
    my_data = kmalloc(sizeof(struct work_data), GFP_KERNEL);
    my_data->my_data = skb;
    INIT_WORK(&my_data->my_work, work_handler);
    queue_work(wq, &my_data->my_work);
}

static void work_handler(struct work_struct *work){
    printk("Entering: %s\n",__FUNCTION__);
    struct work_data * my_data;
    my_data = container_of(work, struct work_data,  my_work);

    struct sk_buff *skb = (struct sk_buff *)my_data->my_data;
    /*struct sk_buff *copy = skb_copy(skb, GFP_KERNEL);
    if(copy == NULL){
        printk(KERN_ERR "Error while copying skb struct...\n");
        return NULL;
    }*/
    nlh_kernel_receive=(struct nlmsghdr*)skb->data;
    nlh_kernel_send=(struct nlmsghdr*)skb->data;
    PDU_struct *response = read_exactly_from_user(nlmsg_data(nlh_kernel_receive));
    if(response == NULL){
        printk(KERN_ERR "Error while reading from user space\n");
        return NULL;
    }
    data response_buffer = PDU_to_buffer_kernel(response);
    printk(KERN_ERR "HEJ1\n");
    pid = nlh_kernel_send->nlmsg_pid; //pid of sending process
    printk(KERN_ERR "HEJ2\n");
    msg_size=(response->data_bytes)+HEADERSIZE;
    printk(KERN_ERR "msg_size %d\n", msg_size);
    skb_out = nlmsg_new(msg_size,0);
    if(skb_out == NULL){
        printk(KERN_ERR "Error when nlmsg_new\n");
        return NULL;
    }
    printk(KERN_ERR "HEJ4\n");
    if(!skb_out){
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }
    printk(KERN_ERR "HEJ5\n");
    nlh_kernel_send=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);
    printk(KERN_ERR "HEJ6\n");
    NETLINK_CB(skb_out).dst_group = 0;
    printk(KERN_ERR "HEJ7\n");
    memcpy(nlmsg_data(nlh_kernel_send), response_buffer, msg_size);
    printk(KERN_ERR "HEJ8\n");
    int res=nlmsg_unicast(nl_sk_send,skb_out,pid);
    printk(KERN_ERR "pid %d\n", pid);
    printk(KERN_ERR "HEJ9\n");

    if(res<0)
        printk(KERN_ERR "Error while sending bak to user\n");

}


/* Reads the desired PDU.
* @param    data - The data.
* @return   pdu - The pdu.
*/
static PDU_struct *read_exactly_from_user(data request){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    PDU_struct *response  = kmalloc(sizeof(PDU_struct), GFP_KERNEL);
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


static data PDU_to_buffer_kernel(PDU_struct *pdu){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    printk(KERN_INFO "opcode kernel: %u\n", pdu->OP_code);
    printk(KERN_INFO "data_bytes kernel: %u\n", pdu->data_bytes);
    printk(KERN_INFO "data kernel: %s\n", (char*)pdu->data);
    data response_buffer = kmalloc(((pdu->data_bytes)+HEADERSIZE), GFP_KERNEL);
    data head = response_buffer;
    memcpy(response_buffer, &pdu->OP_code, 1);
    response_buffer++;
    memcpy(response_buffer, &pdu->data_bytes, 2);
    response_buffer+=3;
    memcpy(response_buffer, pdu->data, (pdu->data_bytes));
    return head;
}

static void read_DELETE_struct(PDU_struct *response, data request){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    size_t arr_size = KEY_SIZE+1;
    char key[arr_size];
    request+=HEADERSIZE;
    memset(key,'\0',arr_size);
    memcpy(key, request, strlen((char*)request));
    key[KEY_SIZE] = '\0';

    printk(KERN_INFO "GET key: %s\n", key);
    //struct rhash_object *obj_get;
    struct rhash_object *obj;
    obj = rhashtable_lookup_fast(&ht, &key, test_rht_params);
    if(obj == NULL){
        printk(KERN_ALERT "Error when getting object.\n");
        response->OP_code = KERNEL;
        response->data = "Error when getting object.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
        return;
    }

    int err = rhashtable_remove_fast(&ht, &obj->node, test_rht_params);
    if(err < 0){
        printk(KERN_ALERT "Error when deleting from rhashtable: %d\n", err);
        response->OP_code = KERNEL;
        response->data = "Error when deleting from rhashtable.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }else{
        printk(KERN_INFO "Successful deleting object from rhashtable.");
        response->OP_code = KERNEL;
        response->data = "Successful deleting object from rhashtable.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }
    kfree(obj->data);
    kfree(obj);

}

static void read_GET_struct(PDU_struct *response, data request){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    size_t arr_size = KEY_SIZE+1;
    char key[arr_size];
    request+=HEADERSIZE;
    memset(key,'\0',arr_size);
    memcpy(key, request, strlen((char*)request));
    key[KEY_SIZE] = '\0';

    printk(KERN_INFO "GET key: %s\n", key);
    printk(KERN_INFO "Size of key2: %zu\n", sizeof(key));
    printk(KERN_INFO "strlen((char*)request: %zu\n", strlen((char*)request));
    //struct rhash_object *obj_get;
    struct rhash_object *obj_get = NULL;
    obj_get = rhashtable_lookup_fast(&ht, &key, test_rht_params);
    if(obj_get == NULL){
        printk(KERN_ALERT "Error when getting object.\n");
        response->OP_code = KERNEL;
        response->data = "Error when getting object.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
        return;
    }

    if (memcmp(obj_get->key, key, KEY_SIZE) == 0){
        printk(KERN_INFO "Key matches.\n");
        response->OP_code = KERNEL;
        response->data = obj_get->data;
        response->data_bytes = obj_get->data_bytes;
    }else{
        printk(KERN_INFO "Error, key does not match...\n");
        response->OP_code = KERNEL;
        response->data = "Error, key does not match...";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }

}

static void read_INSERT_struct(PDU_struct *response, data request){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);



    struct rhash_object *obj = kmalloc(sizeof(struct rhash_object),GFP_KERNEL);
    if (!obj) {
        printk(KERN_ALERT "Error when kmalloc in function %s\n", __FUNCTION__);
        response->OP_code = KERNEL;
        response->data = "Error when kmalloc.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
        return NULL;
    }


    request++;
    memcpy(&obj->data_bytes, request, 2);
    printk(KERN_INFO "obj->data_bytes: %u\n", obj->data_bytes);
    request+=3;
    size_t arr_size = KEY_SIZE+1;
    memset(obj->key,'\0',arr_size);
    memcpy(obj->key, request, strlen((char*)request));
    obj->key[KEY_SIZE] = '\0';

    //obj->key = 1337;
    printk(KERN_INFO "obj->key: %s\n", obj->key);
    request+=KEY_SIZE;
    obj->data = kmalloc(obj->data_bytes,GFP_KERNEL);
    memcpy(obj->data, request, (obj->data_bytes));
    //printk(KERN_INFO "obj->data: %s\n", (char*)obj->data);

    int err = rhashtable_lookup_insert_fast(&ht, &obj->node, test_rht_params);
    if(err == -EEXIST){
        printk(KERN_ALERT "Error, duplicated key.\n");
        response->OP_code = KERNEL;
        response->data = "Error, duplicated key.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }else if(err < 0){
        printk(KERN_ALERT "Error when insert to rhashtable: %d\n", errno);
        response->OP_code = KERNEL;
        response->data = "Error when insert to rhashtable.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }else{
        printk(KERN_INFO "Inserted object to rhashtable");
        response->OP_code = KERNEL;
        response->data = "Inserted object to rhashtable";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }


}

static void read_INIT_struct(PDU_struct *response, data data){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    if(hashtable_initialized == 1) {
        printk(KERN_ALERT "Hashtable already initialized.\n");
        response->OP_code = KERNEL;
        response->data = "Hashtable already initialized.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
        return;
    }

    int err = rhashtable_init(&ht, &test_rht_params);
    if (err < 0) {
        printk(KERN_ALERT "Unable to initialize hashtable: %d\n", err);
        response->OP_code = KERNEL;
        response->data = "Unable to initialize hashtable.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }else{
        hashtable_initialized = 1;
        printk(KERN_INFO "Initialized hashtable: %d\n", err);
        response->OP_code = KERNEL;
        response->data = "Initialized hashtable.";
        response->data_bytes = strnlen(response->data, MAX_PAYLOAD)+1;
    }

}
int my_compare_function(struct rhashtable_compare_arg *arg, const void *obj){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    struct rhashtable *ht = arg->ht;
	const char *ptr = obj;

	return memcmp(ptr + ht->p.key_offset, arg->key, ht->p.key_len);
    //return memcmp(arg->key, obj, KEY_SIZE);
}



static int __init init(void) {
    printk("Entering: %s\n",__FUNCTION__);
    hashtable_initialized = 0;
    /* This is for 3.6 kernels and above.*/

    struct netlink_kernel_cfg cfg_send = {
        .input = recieve_data,
    };

    struct netlink_kernel_cfg cfg_receive = {
        .input = recieve_data,
    };

    nl_sk_send = netlink_kernel_create(&init_net, NETLINK_USER_SEND, &cfg_send);

    if(!nl_sk_send){
        printk(KERN_ALERT "Error creating socket send.\n");

        return -10;
    }

    nl_sk_receive = netlink_kernel_create(&init_net, NETLINK_USER_RECEIVE, &cfg_receive);

    if(!nl_sk_receive){
        printk(KERN_ALERT "Error creating socket receive.\n");
        netlink_kernel_release(nl_sk_send);
        return -10;
    }

    //struct work_data * data;
    const char *name = "wq_test";
    wq = create_workqueue(name);
    //DECLARE_WORK(my_work, void (*work_handler)(void *), void *data);
    //data = kmalloc(sizeof(struct work_data), GFP_KERNEL);
    //INIT_WORK(&data->work, work_handler);
    //queue_work(wq, &data->work);

    return 0;
}

static void free_ht_objects(data ptr, data arg){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    struct rhash_object *data = (struct rhash_object*) ptr;
    kfree(data->data);
    kfree(data);
}

static void __exit exit(void) {

    printk(KERN_INFO "exiting module\n");
    netlink_kernel_release(nl_sk_send);
    netlink_kernel_release(nl_sk_receive);
    if(hashtable_initialized == 1){
        rhashtable_free_and_destroy(&ht, free_ht_objects, NULL);
    }
    flush_workqueue(wq);
    destroy_workqueue(wq);
}
