//
// Created by c15jod on 2017-10-02.
//



#include "socket.h"

/* Creates a socket for a server.
 * @param    sis - Socket information struct that have information
 *                  to create socket.
 * @return   mysocket - On success the the created socket is returned else -1.
 */
int createsocket_server(sock_init_struct *sis) {
    int mysocket=0;
    int status=0;
    int yes = 1;
    struct addrinfo hints, *result, *rp;
    char *listeningPort = calloc(sizeof(char),16);
    sprintf(listeningPort, "%d", *sis->nextportString);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = (sis->isUDP == 1) ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if((status=getaddrinfo(sis->nexthost, sis->nextportString, &hints, &result))!=0){
        fprintf(stderr, "Receiver, could not find host: %s\n", gai_strerror(status));
        return -1;
    }

    /* Find correct adress and try to bind a socket */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        mysocket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (mysocket == -1)
            continue;
        if (setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))==-1) {
            perror("setsockopt");
            return -1;
        }
        if (bind(mysocket, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        close(mysocket);
    }

    if (rp == NULL) {
        perror("Receiver, could not bind");
        exit(EXIT_FAILURE);
    }

    free(listeningPort);
    freeaddrinfo(result);
    return mysocket;
}

/* Creates a socket for a client.
 * @param    sis - Socket information struct that have information
 *                  to create socket.
 * @return   mysocket - On success the the created socket is returned else -1.
 * @comment mysocket connects to server socket.
 */
int createsocket_client(sock_init_struct *sis) {
    struct sockaddr_in addr;
    int mysocket;
    struct addrinfo *res, hints;
    int status=0;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = (sis->isUDP == 1) ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    if ((mysocket = socket(AF_INET,hints.ai_socktype,((sis->isUDP == 1) ? IPPROTO_UDP : IPPROTO_TCP))) == -1){
        perror("socket");
        return -1;
    }

    /* Build the network address of this client */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(mysocket, (struct sockaddr *)&addr, sizeof (addr)) != 0){
        perror("bind");
        return -1;
    }

    if((status=getaddrinfo(sis->nexthost, sis->nextportString, &hints, &res))!=0){
        fprintf(stderr, "getaddrinfo: %s",gai_strerror(status));
        freeaddrinfo(res);
        return -1;
    }
    int i=0;
    while(1){
        if(connect(mysocket, res->ai_addr, res->ai_addrlen)<0){
            if(++i>3){
                fprintf(stderr,"Failed to connect, exiting.\n");
                freeaddrinfo(res);
                return -1;
            }
            perror("send_pdu - Could not connect to host, sleep 1 sec");
            sleep(1);
            continue;
        }
        break;
    }

    freeaddrinfo(res);
    return mysocket;
}



int send_pdu(int sock, PDU_struct *pdu){

    if(send(sock,pdu->pdu,pdu->numbytes,MSG_NOSIGNAL)==-1){
        perror("send");
        return -1;
    }
    return 0;
}

PDU_struct *receive_pdu(int sock) {
    ssize_t nread = 0;
    char buffer_read[HEADERSIZE];
    PDU_struct *PDU_struct;
    while(nread<HEADERSIZE){
        nread=recv(sock,buffer_read,HEADERSIZE,MSG_PEEK);
        //sleep(3);
        if(nread == 0){
            return (void *)-100;
        }
        if(nread == -1)
            return NULL;


    }
    //char *pdu=NULL;
    PDU_struct = read_exactly(sock, (uint8_t)buffer_read[0]);

    return PDU_struct;
}
