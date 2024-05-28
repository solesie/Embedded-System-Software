/*
 * Stopwatch Device Driver
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
#include "stopwatch_ctrl.h"
#include "logging.h"

#define STOPWATCH_MAJOR 242
#define STOPWATCH_NAME "stopwatch"
#define IOCTL_RUN _IO(STOPWATCH_MAJOR, 0)
const static unsigned int num_of_dev = 1;

static dev_t stopwatch_devt;
static struct cdev stopwatch_cdev;
static struct class* stopwatch_class;
static atomic_t already_open = ATOMIC_INIT(0);

static int stopwatch_open(struct inode* inode, struct file* fp){
	if(atomic_cmpxchg(&already_open, 0, 1))
		return -EBUSY;
	try_module_get(THIS_MODULE);
	return 0;
}

static int stopwatch_close(struct inode* inode, struct file* fp){
	// kernel fs will handle security.
	atomic_set(&already_open, 0);
	module_put(THIS_MODULE);
	return 0;
}

static long stopwatch_ioctl(struct file* fp, unsigned int cmd, unsigned long arg){
	switch(cmd){
		case IOCTL_RUN:
			stopwatch_run();
			return 0;

		default:
			LOG(LOG_LEVEL_INFO, "ioctl command not found");
			return -ENOTTY;
	}
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = stopwatch_open,
	.release = stopwatch_close,
	.unlocked_ioctl = stopwatch_ioctl,
};

static int __init stopwatch_driver_init(void){
	stopwatch_devt = MKDEV(STOPWATCH_MAJOR, 0);
	stopwatch_class = NULL;

	if(register_chrdev_region(stopwatch_devt, num_of_dev, STOPWATCH_NAME))
		goto error;

	cdev_init(&stopwatch_cdev, &fops);
	if(cdev_add(&stopwatch_cdev, stopwatch_devt, num_of_dev))
		goto unreg_driver;

	stopwatch_class = class_create(THIS_MODULE, STOPWATCH_NAME);
	if(stopwatch_class == NULL)
		goto unreg_cdev;

	if(device_create(stopwatch_class, NULL, stopwatch_devt, NULL, STOPWATCH_NAME) == NULL)
		goto unreg_class;

	stopwatch_init();

	LOG(LOG_LEVEL_INFO, "Driver %s(major: %d, minor: %d) registered"
			, STOPWATCH_NAME, MAJOR(stopwatch_devt), MINOR(stopwatch_devt));
	LOG(LOG_LEVEL_INFO, "Device file created on /dev/%s", STOPWATCH_NAME);
	return 0;

unreg_class:
	class_destroy(stopwatch_class);

unreg_cdev:
	cdev_del(&stopwatch_cdev);

unreg_driver:
	unregister_chrdev_region(stopwatch_devt, num_of_dev);

error:
	LOG(LOG_LEVEL_ERROR, "Failed to install device and create device file");
	return -1;
}

static void __exit stopwatch_driver_exit(void){
	device_destroy(stopwatch_class, stopwatch_devt);
	class_destroy(stopwatch_class);
	cdev_del(&stopwatch_cdev);
	unregister_chrdev_region(stopwatch_devt, num_of_dev);
	stopwatch_exit();
	LOG(LOG_LEVEL_INFO, "Driver %s unregistered", STOPWATCH_NAME);
	LOG(LOG_LEVEL_INFO, "Device file removed from /dev/%s", STOPWATCH_NAME);
}

module_init(stopwatch_driver_init);
module_exit(stopwatch_driver_exit);

MODULE_AUTHOR("SangYun Lee");
MODULE_LICENSE("GPL");
