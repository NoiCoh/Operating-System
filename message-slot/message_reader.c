#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "message_slot.h"

int main(int argc, char* argv[]){
    char buffer[USER_BUFFER_SIZE];
    int read_len;
    int val;
    int file_d = -1;
    unsigned int channel_id;
    
    if (argc != 3 ){
        perror("ERROR: Wrong number of parameters ");
		exit(1);
    }

    file_d = open(argv[1], O_RDWR);
	if (file_d == -1) {
        perror("ERROR: open failed ");
		exit(1);
	}
    channel_id = atoi(argv[2]);
    val = ioctl(file_d, MSG_SLOT_CHANNEL, channel_id);
	if (val < 0) {
		perror("ERROR: ioctl failed ");
		close(file_d);
		exit(1);
	}
    
    read_len = read(file_d, buffer, sizeof(buffer));
    if (read_len < 0) {
        perror("ERROR: read failed ");
		close(file_d);
		exit(1);
	}
    if (write(STDOUT_FILENO, buffer, read_len) < 0) {
        perror("ERROR: print message failed ");
        exit(1);
    }
    
    close(file_d);
    exit(0);
}