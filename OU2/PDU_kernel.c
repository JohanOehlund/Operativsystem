//
// Created by hasse on 2/19/19.
//

#include "PDU_kernel.h"


static void *create_init_PDU(void* answer){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    return NULL;
}

static void *create_insert_PDU(void* answer){

    return NULL;
}

static void *create_get_PDU(void* answer){

    return NULL;
}

static void *create_delete_PDU(void* answer){

    return NULL;
}


/* Free a struct.
 * @param   p -  The generic struct that will be free'd.
 * @return
 */
static void free_struct(GEN_struct *p){

    if(p->OPCode==INIT){
        /*REG_struct *temp_struct=(REG_struct *) p->created_struct;
        free(temp_struct->servername);
        free(temp_struct);
        free(p);*/
    }else if(p->OPCode==INSERT){

    }else if(p->OPCode==GET){

    }else if(p->OPCode==DELETE){

    }else{
        printk(KERN_INFO "INVALID OP-code\n");
    }
}

/* Serialization of a pdu.
 * If error while creating the pdu then every resources of creating struct will be free'd
 * even the struct.
 * @param   p -  Struct with data for the pdu.
 * @return  tempPduStruct - A struct that contains the pdu and size (in bytes).
 */

static PDU_kernel_struct *pdu_kernel_creater(GEN_struct *p){
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    void *tempPduStruct=NULL;
    switch (p->OPCode){
        case INIT:
            printk(KERN_INFO "Entering INIT in: %s\n", __FUNCTION__);
            /*if((tempPduStruct=create_init_PDU(p->created_struct))==NULL){
                free_struct(p);
                return NULL;
            }*/
            break;
        case INSERT:

            break;
        case GET:

            break;
        case DELETE:

            break;

        default:
            printk(KERN_INFO "INVALID OP-code\n");

    }
    free_struct(p);
    return tempPduStruct;
}








