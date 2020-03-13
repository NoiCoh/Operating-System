#ifndef __MESSAGE_SLOT__
#define __MESSAGE_SLOT__

#include <linux/ioctl.h>

#define DEVICE_RANGE_NAME "message_slot"
#define USER_BUFFER_SIZE 128
#define MAJOR_NUM 240
#define ERROR -1

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)
#endif

