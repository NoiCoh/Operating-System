#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "message_slot.h"

int main(int argc, char **argv){
    
	char buffer[USER_BUFFER_SIZE];
	unsigned int channel_id;
    int write_len;
    int val;
	int file_d = -1;
    
	if (argc != 4) {
		perror("ERROR: Wrong number of parameters ");
		exit(1);
	}

	file_d = open(argv[1], O_RDWR);
	if (file_d == -1) {
        perror("ERROR: Open file failed ");
		exit(1);
	}

	channel_id = atoi(argv[2]);
	strncpy(buffer, argv[3], sizeof(buffer));
    val = ioctl(file_d, MSG_SLOT_CHANNEL, channel_id);
	if (val < 0) {
		perror("ERROR: ioctl failed ");
		close(file_d);
		exit(-1);
	}

    write_len = write(file_d, buffer, strlen(buffer));
	if ( write_len < 0) {
        perror("ERROR: write failed ");
		close(file_d);
		exit(-1);
	}
	
	close(file_d);
	exit(0);
}
