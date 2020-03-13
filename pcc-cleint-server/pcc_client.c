#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>

#define ARGS 4

int send_len(int sockfd, uint32_t N){
    char *buf;
    uint32_t left_to_send, network_byte_order;
    long bytes_send;
    
    left_to_send = sizeof(uint32_t);
    network_byte_order = htonl(N);
    buf = (char*) &network_byte_order;
    
    while (left_to_send > 0){
        bytes_send = write(sockfd, buf, left_to_send);
        if (bytes_send < 0) {
          fprintf(stderr,"Error: write N failed\n");
          return 1;
        }
        else{
            buf += (uint32_t) bytes_send;
            left_to_send -= (uint32_t) bytes_send;
        }
    }
    return 0;
}

int read_before_send_message(int fd, char *buf, uint32_t N) {
    long bytes_read;
    uint32_t left;
    left = N;
    while(left > 0) {
        bytes_read = read(fd, buf, left);
        if( bytes_read < 0 ){
            fprintf(stderr,"Error: read failed\n");
            return 1;
        }
        else{
            buf  += (uint32_t)bytes_read;
            left -= (uint32_t)bytes_read;
        }
    }   
    return 0;
}

int write_before_send_message(int sockfd, char *buf, uint32_t N) {
    long bytes_send;
    uint32_t left;
    left = N;
    while(left > 0){
        bytes_send = write(sockfd, buf, left);
        if( bytes_send < 0 ){
            fprintf(stderr,"Error: write failed\n");
            return 1;
        }
        else{
            buf  += (uint32_t)bytes_send;
            left -= (uint32_t)bytes_send;
        }
    }
    return 0;
}

int send_message(int fd, int sockfd, uint32_t N){
    char* buf;
    buf = calloc(N, 1);
    
    if (read_before_send_message(fd, buf, N)) {
        return 1;
    }
    if (write_before_send_message(sockfd, buf, N)) {
        return 1;
    }
    free(buf);
    return 0;
}

uint32_t printable_characters(int sockfd){
    uint32_t printable, left_to_read;
    long bytes_read;
    char *buf;
    
    buf = (char*)&printable;
    left_to_read = sizeof(uint32_t);
    while (left_to_read > 0){
        bytes_read = read(sockfd, buf, left_to_read);
        if (bytes_read < 0) {
            fprintf(stderr,"Error: read printable characters failed\n");
            return 1;
        }
        else{
            buf += (uint32_t)bytes_read;
            left_to_read -= (uint32_t)bytes_read;
        }
    }
    buf-=sizeof(uint32_t);
    printable = ntohl(*((uint32_t*)(buf)));
    return printable;
}

int server_side(int fd, int sockfd){
    struct stat st;
    uint32_t N;
    int error;

    fstat(fd, &st);
    N = st.st_size;

    error = send_len(sockfd, N);
    if(error != 0){
        return 1;
    }
    error = send_message(fd, sockfd, N);
    if(error != 0){
        return 1;
    }
    return 0;
}


int main(int argc,char* argv[]){
    
	struct in_addr serverIP;
	unsigned short serverPort;
	char *file_path;
	int sockfd, fd, check;
    uint32_t printable;
    
	if (argc != ARGS) {
		fprintf(stderr,"Error: wrong number of arguments\n");
		exit(1);
	}

	//init variables
    inet_aton(argv[1], &serverIP);
	serverPort = (unsigned short) atoi(argv[2]);
	file_path = argv[3];

	//create a socket
    sockfd = -1;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,"Error: creating socket failed\n");
		exit(1);
	}
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(serverPort);
    server_addr.sin_addr = serverIP;
    
    //try to connect
    check = connect(sockfd,(struct sockaddr*)&server_addr, sizeof(server_addr));
    if(check < 0){
        fprintf(stderr, "Error: connection failed\n");
        exit(1);
    }
	
	//connected to the server
    fd = open(file_path, O_RDONLY);
    if(fd < 0){
        fprintf(stderr, "Error: opening file failed\n");
        exit(1);
    }
    check = server_side(fd, sockfd);
    if (check != 0){
        exit(1);
    }
   
    printable = printable_characters(sockfd);
    printf("# of printable characters: %u\n", printable);
    
    close(sockfd);
    close(fd);
	exit(0);
}