#include "list.h"



llist *llist_empty(void) {
    llist *l=malloc(sizeof(llist));
    l->head=malloc(sizeof(node));
	l->tail=malloc(sizeof(node));
    l->head->next=NULL;
	l->tail->previous=NULL;
	pthread_mutex_init(&l->mtx, NULL);
	pthread_cond_init(&l->cond, NULL);
    return l;
}



llist_position llist_first(llist *l) {
	return l->head->next;
}

llist_position llist_last(llist *l) {
	return l->tail->previous;
}

llist_position llist_next(llist_position p) {
	return p->next;
}

llist_position llist_previous(llist_position p) {
	return p->previous;
}

bool llist_isEmpty(llist *l) {

	return(l->head->next == NULL);
}

data llist_inspect(llist_position p, llist *l) {
	pthread_mutex_lock(&l->mtx);

	if(llist_isEmpty(l)) {
		printf("kan inte inspecta tom lista\n");
		return NULL;
	}
	pthread_mutex_unlock(&l->mtx);
    return p->data;
}

bool llist_isEnd(llist_position p) {
	return(p->next == NULL);
}

llist_position createNewNode(data d) {
    llist_position newPos;
    if((newPos=calloc(1,sizeof(node)))==0){
        perror("calloc");
    }

	//newPos->data = malloc(sizeof(d));
	//memcpy(&newPos->data, &d,sizeof(d));
	newPos->data=d;
	newPos->next = NULL;
	newPos->previous = NULL;

	return newPos;
}

void llist_insertfirst(llist *l, data d) {
	pthread_mutex_lock(&l->mtx);
	llist_position newPos = createNewNode(d);


   if(llist_isEmpty(l)) {
	   l->head->next = newPos;
	   l->tail->previous = newPos;
       pthread_cond_broadcast(&l->cond);
	   pthread_mutex_unlock(&l->mtx);
	   return;
   }

    newPos->next = l->head->next;
    l->head->next->previous = newPos;
    l->head->next = newPos;
    pthread_cond_broadcast(&l->cond);
    pthread_mutex_unlock(&l->mtx);
}

void llist_insertlast(llist *l, data d) {

	pthread_mutex_lock(&l->mtx);
	llist_position newPos = createNewNode(d);

   if(llist_isEmpty(l)) {
	   l->tail->previous = newPos;
	   l->head->next = newPos;

	   pthread_cond_broadcast(&l->cond);
	   pthread_mutex_unlock(&l->mtx);

	   return;
   }

	newPos->previous = l->tail->previous;
	l->tail->previous->next = newPos;
	l->tail->previous = newPos;

	pthread_cond_broadcast(&l->cond);
	pthread_mutex_unlock(&l->mtx);


}

data llist_removefirst(llist *l) {
    pthread_mutex_lock(&l->mtx);
    llist_position p=llist_first(l);

    if(p->next != NULL && p->previous != NULL) {
        p->previous->next=p->next;
        p->next->previous=p->previous;
    }
    else if(p->next == NULL && p->previous == NULL) {
        l->tail->previous = p->previous;
        l->head->next = p->next;
    }
    else if(p->next == NULL) {
        l->tail->previous = p->previous;
        p->previous->next = NULL;
    }
    else if(p->previous == NULL) {
        l->head->next = p->next;
        p->next->previous = NULL;
    }
    void *temp = calloc(1, sizeof(char)*1024);
    memcpy(temp, p->data, sizeof(char)*1024);
    free(p->data);
    free(p);

    pthread_mutex_unlock(&l->mtx);
    return (data)temp;
}

data llist_removefirst_INT(llist *l) {
	pthread_mutex_lock(&l->mtx);
    llist_position p=llist_first(l);

	if(p->next != NULL && p->previous != NULL) {
		p->previous->next=p->next;
		p->next->previous=p->previous;
	}
	else if(p->next == NULL && p->previous == NULL) {
		l->tail->previous = p->previous;
		l->head->next = p->next;
	}
	else if(p->next == NULL) {
		l->tail->previous = p->previous;
		p->previous->next = NULL;
	}
	else if(p->previous == NULL) {
		l->head->next = p->next;
		p->next->previous = NULL;
	}
	void *temp = calloc(1, sizeof(int *));
	memcpy(temp, p->data, sizeof(int *));
	free(p->data);
	free(p);

	pthread_mutex_unlock(&l->mtx);
    return (data)temp;
}

PDU_struct *llist_removefirst_PDU(llist *l) {
    pthread_mutex_lock(&l->mtx);
    llist_position p=llist_first(l);

    if(p->next != NULL && p->previous != NULL) {
        p->previous->next=p->next;
        p->next->previous=p->previous;
    }
    else if(p->next == NULL && p->previous == NULL) {
        l->tail->previous = p->previous;
        l->head->next = p->next;
    }
    else if(p->next == NULL) {
        l->tail->previous = p->previous;
        p->previous->next = NULL;
    }
    else if(p->previous == NULL) {
        l->head->next = p->next;
        p->next->previous = NULL;
    }
    /*
    uint8_t OP_code;
    uint16_t data_bytes;
    data data;
    */
    PDU_struct *pdu_struct_data = p->data;
    data pdu_data=calloc(1,pdu_struct_data->data_bytes);
    memcpy(pdu_data,pdu_struct_data->data,pdu_struct_data->data_bytes);

    PDU_struct *pdu_struct = calloc(1,sizeof(PDU_struct)+sizeof(data));
    pdu_struct->OP_code = pdu_struct_data->OP_code;
    pdu_struct->data_bytes = pdu_struct_data->data_bytes;
    pdu_struct->data = pdu_data;

    free(pdu_struct_data->data);
    free(pdu_struct_data);
    //free(p->data);
    free(p);
    pthread_mutex_unlock(&l->mtx);
    return pdu_struct;
}

llist_position llist_remove(llist *l, llist_position p) {

	pthread_mutex_lock(&l->mtx);

	if(llist_isEmpty(l)) {
		printf("Kan inte ta bort ur tom lista\n");
		pthread_mutex_unlock(&l->mtx);
		return p;

	}
		llist_position nextpos=p->next;

	if(p->next != NULL && p->previous != NULL) {
		p->previous->next=p->next;
		p->next->previous=p->previous;
	}
	else if(p->next == NULL && p->previous == NULL) {
		l->tail->previous = p->previous;
		l->head->next = p->next;
	}
	else if(p->next == NULL) {
		l->tail->previous = p->previous;
		p->previous->next = NULL;
	}
	else if(p->previous == NULL) {
		l->head->next = p->next;
		p->next->previous = NULL;
	}

	free(p);
	pthread_mutex_unlock(&l->mtx);
    return nextpos;
}


void llist_switch(llist *l, llist_position p) {

	if(p->next != NULL && p->previous != NULL) {
		p->next->previous = p->previous;
		p->previous->next = p->next;
		p->next = p->next->next;
			if(p->next->next == NULL) {
				p->previous = l->tail->previous;
				l->tail->previous = p;

			}
			else {
				p->previous = p->next->next->previous;
			}
	}

	if(p->previous == NULL) {
		p->next->previous = p->previous;
		l->head->next = p->next;
		p->next->next->previous = p;
		p->next->next = p;
		p->next = p->next->next;

	}
	if(p->next == NULL) {
		printf("Cant switch last node\n");
		return;
	}

}

void llist_free_slist(llist *l) {

	llist_position p=llist_first(l);
	SLISTSERVERS_struct *temp=llist_inspect(p,l);
	while(p != NULL) {
		free(temp->servername);
		free(p->data);
		p=llist_remove(l, p);
		if(!llist_isEmpty(l))
			temp=llist_inspect(p,l);
	}
	free(l->head);
	free(l->tail);
	free(l);

}



void llist_free(llist *l) {

	llist_position p=llist_first(l);
	while(p != NULL) {
		free(p->data);
		p=llist_remove(l, p);
	}
	free(l->head);
	free(l->tail);
	free(l);

}


void print_list(llist *l) {
	llist_position pos = llist_first(l);
	bool t = true;

	if(llist_isEmpty(l)) {
		printf("Can't print an empty list\n");
		return;
	}

	do {
		t = true;
		printf("%s\n", (char *) llist_inspect(pos, l));
		if(pos->next == NULL)
		t = false;
			pos = llist_next(pos);
	} while (t == true);
}

int list_length(llist *l) {
    pthread_mutex_lock(&l->mtx);
	node* current = l->head->next;
	int count = 0;
	while (current != NULL) {
	count++;
	current = current->next;
	}
    pthread_mutex_unlock(&l->mtx);
return count;
}




void printServerlist(llist *serverlist){
	llist_position p=llist_first(serverlist);
	bool t = true;
	int i=0;
	if(llist_isEmpty(serverlist)) {
		printf("Can't print an empty list\n");
		return;
	}
	printf("\nAvailable servers\n");
	do {
        i++;
		t = true;
    	SLISTSERVERS_struct *s=llist_inspect(p,serverlist);
    	/*printf("Adress: %u.%u.%u.%u\n",s->ipv4_adress[0],s->ipv4_adress[1],
    			s->ipv4_adress[2],s->ipv4_adress[3]);
    	printf("Port: %u\n",s->port);
    	printf("Num_clients: %u\n",s->num_clients);
    	printf("servername_len: %u\n",s->servername_len);*/
    	printf("%d: %s\n",i,s->servername);
		if(p->next == NULL)
			t = false;
		p = llist_next(p);
	} while (t == true);

    printf("\nChoose server by entering the servers number\n");

}
