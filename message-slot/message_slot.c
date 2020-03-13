#undef __KERNEL__
#define __KERNEL__ 
#undef MODULE
#define MODULE 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/init.h>

#include "message_slot.h"

MODULE_LICENSE("GPL");

typedef struct message{
    unsigned int minor;
    char buffer[USER_BUFFER_SIZE];
    size_t size;
    struct message* next;
} Message;

typedef struct message_slot {
    int minor;
    Message* message;
    struct message_slot* next;
} Message_Slot;

/* Define head of the message slot */
static Message_Slot* message_slot_head = NULL;
static Message_Slot* message_slot_tail = NULL;

static Message_Slot* found_curr_msg_slot(int minor){
    Message_Slot* curr_node;
    curr_node = message_slot_head;
    while (curr_node) {
        if (curr_node->minor == minor) {
            return curr_node;
        }
        curr_node = curr_node->next;
    }
    return curr_node;
}
static Message* get_message(Message_Slot* message_slot, unsigned int channel_id){
    Message* message;
    message = message_slot->message;
    while (message) {
        if (message->minor == channel_id){
            return message;
        }
        message = message->next;
    }
    return 0;
}

static Message_Slot* create_msg_slot(int minor){
    Message_Slot* new_msg_slot;
    new_msg_slot = (Message_Slot*) kmalloc(sizeof(Message_Slot), GFP_KERNEL);
    if (!new_msg_slot) {
        return NULL;
    }
    memset(new_msg_slot, 0, sizeof(Message_Slot));
    new_msg_slot->minor = minor;
    return new_msg_slot;
}

static Message* create_msg(Message_Slot* msg_slot, int minor){
    Message* new_msg;
    Message* tmp_msg;
    new_msg = (Message *)kmalloc(sizeof(Message),GFP_KERNEL);
    if (!new_msg){
        return NULL;
    }
    memset(new_msg, 0, sizeof(Message));
    memset(new_msg->buffer,0,USER_BUFFER_SIZE);
    new_msg->minor = minor;
    
    tmp_msg = msg_slot->message;
    if (!tmp_msg) {
    	msg_slot->message = new_msg;
    } else {
        while (tmp_msg->next != NULL){
            tmp_msg = tmp_msg->next;
        }
        tmp_msg->next = new_msg;
    }
    return new_msg;
}

static int device_open(struct inode *inode, struct file *file){
    int minor;
    Message_Slot* new_node;
    minor = iminor(inode);
    if(!message_slot_head){
        message_slot_head = create_msg_slot(minor);
        if (!message_slot_head) {
			return -EINVAL;
		}
        message_slot_tail = message_slot_head;
    } else { // if head exists
        if(!found_curr_msg_slot(minor)){
            new_node = create_msg_slot(minor);
            if(!new_node){
                return -EINVAL;
            }
            message_slot_tail->next = new_node;
            message_slot_tail = new_node;
        }
    }
	return 0;
}

static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param) {
	Message_Slot *curr_slot;
    if (MSG_SLOT_CHANNEL == ioctl_command_id){
        int minor;
        minor = iminor(file->f_path.dentry->d_inode);
        curr_slot = found_curr_msg_slot(minor);
        if (!curr_slot){
            return -EINVAL;
        }
        file->private_data = (void*)ioctl_param;
    }
    else {
        return -EINVAL;
    }
    return 0;
}

static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset) {
    
    /* init variables */
    int minor;
    int i;
    unsigned int channel_id;
    Message_Slot *curr_slot;
    Message* message;
    
    minor = iminor(file->f_path.dentry->d_inode);
    curr_slot = found_curr_msg_slot(minor);
    if (!curr_slot){
        return -EINVAL;
    }
    
    channel_id = (unsigned int)(uintptr_t) file->private_data;
    if (!channel_id) {
        return -EINVAL;
    }
    
    message = get_message(curr_slot, channel_id);
    if(!message){   
        return -EINVAL;
    }
    if(message->size == 0){
        return -EWOULDBLOCK;
    }
    if(length < message->size){
        return -ENOSPC;
    }
    
    for (i = 0; i < message->size; i++) {
        if (put_user(message->buffer[i], &buffer[i]) != 0) {
            return -EFAULT;
        }
    }
	return message->size;
}

static ssize_t device_write( struct file* file, const char __user* buffer, size_t length, loff_t* offset) {
    
    /* init variables */
	int minor;
    int i;
    unsigned int channel_id;
    Message_Slot *curr_slot;
    Message* message;
    
    minor = iminor(file->f_path.dentry->d_inode);
    curr_slot = found_curr_msg_slot(minor);
    if (!curr_slot){
        return -EINVAL;
    }
    
    channel_id = (unsigned int)(uintptr_t) file->private_data;
    if (!channel_id) {
        return -EINVAL;
    }
    
    message = get_message(curr_slot, channel_id);
    if(!message){   
        message = create_msg(curr_slot, channel_id);
        if(!message){
            return -EINVAL;
        }
    }
    if(length <= 0 || length > USER_BUFFER_SIZE){
        return -EMSGSIZE;
    }
    
    for (i=0; i < length; i++) {
        if (get_user(message->buffer[i], &buffer[i]) != 0) {
            return -EFAULT;
        }
    }
    message->size = i;
	return message->size;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
{
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .owner          = THIS_MODULE
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
  int rc = -1;

  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

  // Negative values signify an error
  if( rc < 0 )
  {
    printk( KERN_ALERT "Module registraion failed for  %d\n", MAJOR_NUM );
    return rc;
  }
  return 0;
}

//---------------------------------------------------------------
static void free_module(void){
    Message_Slot *message_slot;
    Message *curr_message;
    Message *message;
    while (message_slot_head != NULL){
        curr_message = message_slot_head->message;
        while (curr_message != NULL){
            message = curr_message;
            curr_message = curr_message->next;
            kfree(message);
        }
		message_slot = message_slot_head;
		message_slot_head = message_slot_head->next;
        kfree(message_slot);
    }
}

static void __exit simple_cleanup(void) {
    free_module();
    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);
