#include "netlinkKernel.h"

//https://lwn.net/Articles/751374/
//https://github.com/intel/linux-intel-4.9/blob/master/lib/test_rhashtable.c


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

  /*INIT_struct* init_struct = kmalloc(sizeof(INIT_struct),GFP_KERNEL);
  if(init_struct == (void *) 0){

      printk(KERN_ALERT "Memory allocation failed.\n");

      return 0;

  }


  memcpy(&init_struct->OPCode,data,1);

  printk(KERN_INFO "Opcode in read_INIT_struct: %u\n", init_struct->OPCode);

  kfree(init_struct);*/

  int err = 0;
  memset(&array, 0, sizeof(array));
  printk(KERN_INFO "array[50]: %d\n", array[50]);
  err = rhashtable_init(&ht, &test_rht_params);
  if (err < 0) {
      printk(KERN_ALERT "Test failed: Unable to initialize hashtable: %d\n",
    		err);

  	return NULL;
  }

  /*unsigned int i, insert_retries = 0;
  s64 start, end;

  struct test_obj *obj = &array[0];
  obj->value = 1337;
  printk(KERN_INFO "obj->node: %p\n", obj->node);*/

  int num_items = 5;
  int i = 0;
  while(i <= num_items){
    struct test_obj *obj = kmalloc(sizeof(struct test_obj),GFP_KERNEL);
      if (!obj) {
      err = -ENOMEM;
    }

    obj->ptr = TEST_PTR;
    obj->value = i * 2;
    printk(KERN_INFO "obj->ptr value: %d\n", obj->value);

    err = rhashtable_insert_fast(&ht, &obj->node, test_rht_params);
    i++;
  }
  int j = 0;
  while(j < num_items){
    struct test_obj *obj_get = kmalloc(sizeof(struct test_obj),GFP_KERNEL);;
    u32 key_get = j * 2;
    printk(KERN_INFO "key_get: %d\n", key_get);
    obj_get = rhashtable_lookup_fast(&ht, &key_get, test_rht_params);
    if (obj_get->ptr != TEST_PTR || obj_get->value != (j * 2)) {
      printk(KERN_INFO "obj->ptr or obj->value did not match.\n");

    }else{
      printk(KERN_INFO "IT MATCHES!!!\n");
    }
    //printk(KERN_INFO "obj value: %p\n", obj_get);
    printk(KERN_INFO "obj string: %s\n", (char*) obj_get->ptr);
    j++;
  }
  //kfree(obj);
  return NULL;
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
