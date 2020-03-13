#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#define PRINTABLE 95		// printable chars [32,126]
#define PRINTABLE_START 32
#define PRINTABLE_END 126
#define BACKLOG 10
#define ARGS 2

struct pcc_total{
	unsigned int table[PRINTABLE];
};

/* global variables */
struct pcc_total pcc;
int accept_tcp; //if allow accepting TCP connections - 1 , else - 0.
/*----------------------*/


void sighandler(int signum){
	accept_tcp = 0;
}

int initsiginthandler(void){
	struct sigaction sigint_action; 	
	memset(&sigint_action, 0, sizeof(sigint_action));
	sigint_action.sa_handler = sighandler;
	if (sigaction(SIGINT, &sigint_action, NULL) == -1){
		fprintf(stderr, "ERROR: sigaction failed - %s", strerror(errno));
		return 1;
	}
	return 0;
}

int recv_len(int fd, uint32_t* result){
    int left_to_read, bytes_read;
    uint32_t N;
    char *buf;
    
    left_to_read = sizeof(uint32_t);
    buf = (char*) &N;
    while (left_to_read > 0){
        bytes_read = read(fd, buf, left_to_read);
        if (bytes_read < 0) {
            fprintf(stderr,"Error: read faied - %s", strerror(errno));
            return 1;
        }
        else{
            left_to_read -= bytes_read;
            buf += bytes_read;  
        }
    }
    buf -= sizeof(uint32_t);
    *result = ntohl(*((uint32_t*)(buf)));
    return 0;
}

int recv_message(int fd, char* message, uint32_t N){
    uint32_t left_to_read;
    long bytes_read;
    left_to_read = N;
    while(left_to_read > 0){
        bytes_read = read(fd, message, left_to_read);
        if (bytes_read < 0) {
            fprintf(stderr,"Error: read faied 1 - %s", strerror(errno));
            return 1;
        }
        else{
            left_to_read -= (uint32_t)bytes_read;
            message += (uint32_t) bytes_read;
        }
    }
    return 0;
}

uint32_t update_printable(char* message, uint32_t N){
    uint32_t printable, i;
    printable = 0;
    for (i = 0; i < N; i++){
        if((message[i] >= PRINTABLE_START) && (message[i] <= PRINTABLE_END)){
            printable++;
            pcc.table[(int)message[i]]++;
        }
    }
    return printable;
}

int send_printable(int fd, uint32_t printable){
    char *buf;
    long bytes_send;
    uint32_t left_to_send, network_byte_order;
    
    left_to_send = sizeof(uint32_t);
    network_byte_order = htonl(printable);
    buf = (char*)&network_byte_order;
    
    while (left_to_send > 0){
        bytes_send = write(fd, buf, left_to_send);
        if (bytes_send < 0) {
            fprintf(stderr,"Error: read faied - %s", strerror(errno));
            return 1;
        }
        else{
            buf += (uint32_t) bytes_send;
            left_to_send -= (uint32_t) bytes_send;
        }
    }
    return 0;
}

int client_side(int fd){
    uint32_t N, printable;
    char* message;
    int error;
    
    error = recv_len(fd,&N);
    if(error == 0){
        message = malloc(N);
        if(!message){
            fprintf(stderr,"Error: malloc failed\n");
            return 1;
        }
        error = recv_message(fd, message, N);
        if(error == 0){
            printable = update_printable(message, N);
            send_printable(fd, printable);
        }
        free(message);
    }
    return 0;
}

int main(int argc,char* argv[]){
    unsigned short serverPort;
    int serverSock, clinetSock,option, check;
    struct sockaddr_in server_address;
    struct sockaddr_in clientaddr;
    socklen_t size;
    
	initsiginthandler();	//set action for ctrl+c
	if (argc != ARGS) {
		fprintf(stderr,"Error: wrong number of arguments\n");
		exit(1);
	}
   
	//init variables and pcc table
	serverPort = (unsigned short)(atoi(argv[1]));
    size = sizeof(struct sockaddr_in);
    option = 1;
    accept_tcp = 1;
	for (int i=0; i<PRINTABLE; i++){
		pcc.table[i] = 0;
	}

	//create socket
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    check = setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &option , sizeof(int));
    if (check < 0){
        fprintf(stderr,"Error: bind failed\n");
    }
	
	//bind
    memset(&server_address, 0, size);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(serverPort);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSock, (struct sockaddr*) &(server_address), sizeof(server_address)) == -1){
    	fprintf(stderr,"Error: bind failed\n");
    	exit(1);
    }
    
	//listen
	if (listen(serverSock, BACKLOG) != 0){
		fprintf(stderr,"Error: listening failed\n");
    	exit(1);
	}
    
	while (accept_tcp){
		clinetSock = accept(serverSock, (struct sockaddr*) &clientaddr, &size);
		if (clinetSock < 0){
            if(errno != EINTR){
                fprintf(stderr,"Error: accept failed %s\n", strerror(errno));
            }
        }
        else{
            check = client_side(clinetSock);
            close(clinetSock);
            if (check != 0){
                exit(1);
            }
        }
	}
    
	//print overall statistics
	for (int i=0; i<PRINTABLE; i++){
		printf("char '%c' : %u times\n", (char) (i+32), pcc.table[i]);
	}
    close(serverSock);
	exit(0);
}
