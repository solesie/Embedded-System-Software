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
#include "text_lcd_ctrl.h"
#include "led_ctrl.h"
#include "interrupt_ctrl.h"
#include "logging.h"

#define MINIGAME_MAJOR 242
#define MINIGAME_NAME "minigame"
#define IOCTL_RUN_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 0)
#define IOCTL_STOP_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 1)
#define IOCTL_OFF_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 2)
#define IOCTL_RESET_TIMER_NONBLOCK _IO(MINIGAME_MAJOR, 3)
#define IOCTL_SET_LED_NONBLOCK _IOW(MINIGAME_MAJOR, 4, int)
#define IOCTL_SET_TEXT_LCD_NONBLOCK _IOW(MINIGAME_MAJOR, 5, char*)
#define IOCTL_WAIT_BACK_INTERRUPT _IOR(MINIGAME_MAJOR, 6, int)

const static unsigned int NUM_OF_DEV = 1;

static dev_t minigame_devt;
static struct cdev minigame_cdev;
static struct class* minigame_class;
static atomic_t already_open = ATOMIC_INIT(0);

static int open(struct inode* inode, struct file* fp){
	if(atomic_cmpxchg(&already_open, 0, 1))
		return -EBUSY;
	try_module_get(THIS_MODULE);

	interrupt_request();

	return 0;
}

static int close(struct inode* inode, struct file* fp){
	interrupt_free();
	
	timer_off();
	text_lcd_set("");
	led_set(0);

	atomic_set(&already_open, 0);
	module_put(THIS_MODULE);
	LOG(LOG_LEVEL_INFO, "closed");
	return 0;
}

static long ioctl(struct file* fp, unsigned int cmd, unsigned long arg){
	char text[TEXT_LCD_MAX_BUFF + 1] = {};
	int led;
	int waked_by_intr;
	switch(cmd){
		case IOCTL_RUN_TIMER_NONBLOCK:
			timer_run();
			return 0;
		case IOCTL_STOP_TIMER_NONBLOCK:
			timer_pause();
			// When the app is forcefully terminated (even if onDestroy() is not called in Java), 
			// it has been observed that the close() function is invoked. 
			// However, if there are any zombie threads remaining, close() is not called 
			// until those threads have been cleared. 
			// This seems to be due to the influence of Java Garbage Collection.
			interrupt_wake_back_waiting_thread();
			return 0;
		case IOCTL_OFF_TIMER_NONBLOCK:
			timer_off();
			return 0;
		case IOCTL_RESET_TIMER_NONBLOCK:
			timer_reset();
			return 0;
		case IOCTL_SET_LED_NONBLOCK:
			if(copy_from_user(&led, (void __user*)arg, sizeof(led)))
				return -EFAULT;
			led_set(led);
			return 0;
		case IOCTL_SET_TEXT_LCD_NONBLOCK:
			if(copy_from_user(text, (char __user*)arg, TEXT_LCD_MAX_BUFF + 1))
				return -EFAULT;
			text[TEXT_LCD_MAX_BUFF] = 0;
			text_lcd_set(text);
			return 0;
		case IOCTL_WAIT_BACK_INTERRUPT:
			waked_by_intr = interrupt_wait_back_intr();
			if(waked_by_intr)
				LOG(LOG_LEVEL_INFO, "Waked by back interrupt");
			else
				LOG(LOG_LEVEL_INFO, "Waked by close");
			if(copy_to_user((void __user*)arg, &waked_by_intr, sizeof(waked_by_intr)))
				return -EFAULT;
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
	.unlocked_ioctl = ioctl,
	.read = read_switch,
};

static int __init mg_init(void){
	minigame_devt = MKDEV(MINIGAME_MAJOR, 0);
	minigame_class = NULL;

	if(register_chrdev_region(minigame_devt, NUM_OF_DEV, MINIGAME_NAME))
		goto error;

	cdev_init(&minigame_cdev, &fops);
	if(cdev_add(&minigame_cdev, minigame_devt, NUM_OF_DEV))
		goto unreg_driver;

	minigame_class = class_create(THIS_MODULE, MINIGAME_NAME);
	if(minigame_class == NULL)
		goto unreg_cdev;

	if(device_create(minigame_class, NULL, minigame_devt, NULL, MINIGAME_NAME) == NULL)
		goto unreg_class;

	timer_init();
	push_switch_init();
	led_init();
	text_lcd_init();
	interrupt_init();

	LOG(LOG_LEVEL_INFO, "Driver %s(major: %d, minor: %d) registered"
			, MINIGAME_NAME, MAJOR(minigame_devt), MINOR(minigame_devt));
	LOG(LOG_LEVEL_INFO, "Device file created on /dev/%s", MINIGAME_NAME);
	return 0;

unreg_class:
	class_destroy(minigame_class);

unreg_cdev:
	cdev_del(&minigame_cdev);

unreg_driver:
	unregister_chrdev_region(minigame_devt, NUM_OF_DEV);

error:
	LOG(LOG_LEVEL_ERROR, "Failed to install device and create device file");
	return -1;
}

static void __exit mg_exit(void){
	timer_exit();
	push_switch_exit();
	text_lcd_exit();
	led_exit();

	device_destroy(minigame_class, minigame_devt);
	class_destroy(minigame_class);
	cdev_del(&minigame_cdev);
	unregister_chrdev_region(minigame_devt, NUM_OF_DEV);

	LOG(LOG_LEVEL_INFO, "Driver %s unregistered", MINIGAME_NAME);
	LOG(LOG_LEVEL_INFO, "Device file removed from /dev/%s", MINIGAME_NAME);
}

module_init(mg_init);
module_exit(mg_exit);

MODULE_AUTHOR("SangYun Lee");
MODULE_LICENSE("GPL");
