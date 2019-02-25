#include "netlinkKernel.h"

//https://lwn.net/Articles/751374/


static void recieve_data(struct sk_buff *skb) {

    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    //char *msg="Hello from kernel";
    int res;

    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    //msg_size=strlen(msg);

    nlh=(struct nlmsghdr*)skb->data;
    void* test = nlmsg_data(nlh);

    read_exactly(test);
    //INIT_struct *test2 = (INIT_struct*)test->created_struct;
    //printk(KERN_INFO "Netlink received <hhhhhhhh> Opcode: %s\n",test);
    //printk(KERN_INFO "Netlink received GEN_struct test: %s\n",test->test);
    /*pid = nlh->nlmsg_pid; //pid of sending process

    skb_out = nlmsg_new(msg_size,0);

    if(!skb_out)
    {

        printk(KERN_ERR "Failed to allocate new skb\n");
        return;

    }*/
    /*nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);
    NETLINK_CB(skb_out).dst_group = 0;
    strncpy(nlmsg_data(nlh),msg,msg_size);

    res=nlmsg_unicast(nl_sk,skb_out,pid);

    if(res<0)
        printk(KERN_INFO "Error while sending bak to user\n");*/
}


/* Reads the desired PDU.
 * @param    data - The data.
 * @return   pdu - The pdu.
 */
static void *read_exactly(void *data){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    uint8_t OPCode;
    memcpy(&OPCode,data,1);
    char *pdu=NULL;
    switch (OPCode){
        case INIT:
            printk(KERN_INFO "Opcode in read_exactly: %u\n", OPCode);
            if((pdu=read_INIT_struct(data))==NULL){
                //free(header);
                return NULL;
            }

            break;

        default:
            printk(KERN_ALERT "Error creating PDU.\n");
            return NULL;
    }
    return pdu;
}



static INIT_struct* read_INIT_struct(void* data){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    uint8_t OPCode;
    memcpy(&OPCode,data,1);
    printk(KERN_INFO "OPCODE: %u\n", OPCode);

    INIT_struct* init_struct = kmalloc(sizeof(INIT_struct),GFP_KERNEL);
    if(init_struct == (void *) 0){

        printk(KERN_ALERT "Memory allocation failed.\n");

        return 0;

    }


    memcpy(&init_struct->OPCode,data,1);

    printk(KERN_INFO "Opcode in read_INIT_struct: %u\n", init_struct->OPCode);

    kfree(init_struct);
    return init_struct;
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

