/*
 * Game1 Device Driver
 */
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include "timer_ctrl.h"
#include "switch_ctrl.h"
#include "../logging.h"

#define GAME1_MAJOR 242
#define GAME1_NAME "game1"
#define IOCTL_RUN_NONBLOCK _IO(GAME1_MAJOR, 0)
#define IOCTL_STOP_NONBLOCK _IO(GAME1_MAJOR, 1)

const static unsigned int NUM_OF_DEV = 1;

static dev_t game1_devt;
static struct cdev game1_cdev;
static struct class* game1_class;
static atomic_t already_open = ATOMIC_INIT(0);

static int open(struct inode* inode, struct file* fp){
	if(atomic_cmpxchg(&already_open, 0, 1))
		return -EBUSY;
	try_module_get(THIS_MODULE);

	timer_init();
	push_switch_init();

	return 0;
}

static int close(struct inode* inode, struct file* fp){
	timer_del();
	push_switch_del();
	
	atomic_set(&already_open, 0);
	module_put(THIS_MODULE);
	return 0;
}

static long ioctl_timer(struct file* fp, unsigned int cmd, unsigned long arg){
	switch(cmd){
		case IOCTL_RUN_NONBLOCK:
			timer_run();
			return 0;
		case IOCTL_STOP_NONBLOCK:
			timer_pause();
			return 0;
		default:
			LOG(LOG_LEVEL_INFO, "ioctl command not found");
			return -ENOTTY;
	}
}

static ssize_t read_switch(struct file *inode, char *gdata, size_t length, loff_t *off_what) {
	enum direction d = push_switch_read();
	unsigned char ret = d;

	if (copy_to_user(gdata, &ret, 1))
	    return -EFAULT;

	return 1;	
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = open,
	.release = close,
	.unlocked_ioctl = ioctl_timer,
	.read = read_switch,
};

static int __init init(void){
	game1_devt = MKDEV(GAME1_MAJOR, 0);
	game1_class = NULL;

	if(register_chrdev_region(game1_devt, NUM_OF_DEV, GAME1_NAME))
		goto error;

	cdev_init(&game1_cdev, &fops);
	if(cdev_add(&game1_cdev, game1_devt, NUM_OF_DEV))
		goto unreg_driver;

	game1_class = class_create(THIS_MODULE, GAME1_NAME);
	if(game1_class == NULL)
		goto unreg_cdev;

	if(device_create(game1_class, NULL, game1_devt, NULL, GAME1_NAME) == NULL)
		goto unreg_class;

	LOG(LOG_LEVEL_INFO, "Driver %s(major: %d, minor: %d) registered"
			, GAME1_NAME, MAJOR(game1_devt), MINOR(game1_devt));
	LOG(LOG_LEVEL_INFO, "Device file created on /dev/%s", GAME1_NAME);
	return 0;

unreg_class:
	class_destroy(game1_class);

unreg_cdev:
	cdev_del(&game1_cdev);

unreg_driver:
	unregister_chrdev_region(game1_devt, NUM_OF_DEV);

error:
	LOG(LOG_LEVEL_ERROR, "Failed to install device and create device file");
	return -1;
}

static void __exit del(void){
	device_destroy(game1_class, game1_devt);
	class_destroy(game1_class);
	cdev_del(&game1_cdev);
	unregister_chrdev_region(game1_devt, NUM_OF_DEV);

	LOG(LOG_LEVEL_INFO, "Driver %s unregistered", GAME1_NAME);
	LOG(LOG_LEVEL_INFO, "Device file removed from /dev/%s", GAME1_NAME);
}

module_init(init);
module_exit(del);

MODULE_AUTHOR("SangYun Lee");
MODULE_LICENSE("GPL");
