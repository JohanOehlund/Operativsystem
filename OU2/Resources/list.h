#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

typedef void* data;


int waitingthreads;
int NRTHR;

typedef struct PDU_struct {
	uint8_t OP_code;
	uint16_t data_bytes;
	data data;
}PDU_struct;

typedef struct node {
	data data;
	struct node* next;
	struct node* previous;
}node;

typedef struct llist {
	struct node *head;
	struct node *tail;
	pthread_mutex_t mtx;
	pthread_cond_t cond;
}llist;

typedef struct SLISTSERVERS_struct {
    uint8_t ipv4_adress[4];
    uint16_t port;
    uint8_t num_clients;
    uint8_t servername_len;
    char *servername;

}SLISTSERVERS_struct;

typedef node* llist_position;


llist *llist_empty(void);

llist_position llist_first(llist *l);

llist_position llist_last(llist *l);

llist_position llist_next(llist_position p);

llist_position llist_previous(llist_position p);

bool llist_isEmpty(llist *l);

void llist_insertfirst(llist *l,data d);

void llist_insertlast(llist *l,data d);

data llist_removefirst(llist *l);

data llist_removefirst_INT(llist *l);

PDU_struct *llist_removefirst_PDU(llist *l);

llist_position llist_remove(llist *l, llist_position p);

void llist_free_slist(llist *l);

void llist_free(llist *l);

data llist_inspect(llist_position p, llist *l);

bool llist_isEnd(llist_position p);

void print_list(llist *l);

int list_length(llist *l);

void llist_switch(llist *l, llist_position p);

llist_position createNewNode(data d);


void printServerlist(llist *serverlist);
