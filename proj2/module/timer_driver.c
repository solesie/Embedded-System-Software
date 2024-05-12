/*
 * Timer Device Driver
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
#include "logging.h"
#include "ioctl_timer_ctrl.h"

struct ioctl_set_option_arg {
	unsigned int timer_interval, timer_cnt;
	char timer_init[FND_MAX + 1];
};

#define TIMER_MAJOR 242
#define TIMER_NAME "dev_driver"
#define IOCTL_SET_OPTION _IOW(TIMER_MAJOR, 0, struct ioctl_set_option_arg)
#define IOCTL_COMMAND _IO(TIMER_MAJOR, 1)
const static unsigned int num_of_dev = 1;

static dev_t timer_devt;
static struct cdev timer_cdev;
static struct class* timer_class;
static atomic_t already_open = ATOMIC_INIT(0);

static int timer_open(struct inode* inode, struct file* fp){
	if(atomic_cmpxchg(&already_open, 0, 1))
		return -EBUSY;
	try_module_get(THIS_MODULE);
	return 0;
}

static int timer_close(struct inode* inode, struct file* fp){
	// kernel fs will handle security.
	atomic_set(&already_open, 0);
	module_put(THIS_MODULE);
	return 0;
}

static long timer_ioctl(struct file* fp, unsigned int cmd, unsigned long arg){
	struct ioctl_set_option_arg data;
	memset(&data, 0, sizeof(data));

	switch(cmd){
		case IOCTL_SET_OPTION:
			if(copy_from_user(&data, (struct ioctl_set_option_arg __user *)arg, sizeof(data)))
				return -EFAULT;	
			if(set_timer_ctrl(data.timer_interval, data.timer_cnt, data.timer_init) == 0)
				return -EINVAL;
			return 0;

		case IOCTL_COMMAND:
			if(run_timer_ctrl() == 0){
				LOG(LOG_LEVEL_INFO, "SET_OPTION missing");
				return -EINVAL;
			}
			return 0;
		
		default:
			return -ENOTTY;
	}
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = timer_open,
	.release = timer_close,
	.unlocked_ioctl = timer_ioctl,
};

static int __init timer_init(void){
	timer_devt = MKDEV(TIMER_MAJOR, 0);
	timer_class = NULL;

	if(register_chrdev_region(timer_devt, num_of_dev, TIMER_NAME)) 
		goto error;
	
	cdev_init(&timer_cdev, &fops);
	if(cdev_add(&timer_cdev, timer_devt, num_of_dev)) 
		goto unreg_driver;

	timer_class = class_create(THIS_MODULE, TIMER_NAME);
	if(timer_class == NULL) 
		goto unreg_cdev;

	if(device_create(timer_class, NULL, timer_devt, NULL, TIMER_NAME) == NULL) 
		goto unreg_class;

	create_timer_ctrl();

	LOG(LOG_LEVEL_INFO, "Driver %s(major: %d, minor: %d) registered"
			, TIMER_NAME, MAJOR(timer_devt), MINOR(timer_devt));
	LOG(LOG_LEVEL_INFO, "Device file created on /dev/%s", TIMER_NAME);
	return 0;

unreg_class:
	class_destroy(timer_class);

unreg_cdev:
	cdev_del(&timer_cdev);

unreg_driver:
	unregister_chrdev_region(timer_devt, num_of_dev);

error:
	LOG(LOG_LEVEL_ERROR, "Failed to install device and create device file");
	return -1;
}

static void __exit timer_exit(void){
	device_destroy(timer_class, timer_devt);
	class_destroy(timer_class);
	cdev_del(&timer_cdev);
	unregister_chrdev_region(timer_devt, num_of_dev);
	destroy_timer_ctrl();
	LOG(LOG_LEVEL_INFO, "Driver %s unregistered", TIMER_NAME);
	LOG(LOG_LEVEL_INFO, "Device file removed from /dev/%s", TIMER_NAME);
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_AUTHOR("SangYun Lee");
MODULE_LICENSE("GPL");
