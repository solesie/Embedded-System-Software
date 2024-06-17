#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/irq.h>
#include "../logging.h"

enum dot_shape{
	ZERO = 0, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, BLANK, DOT_SHAPE_CNT
};

struct single_thread_wq_elem{
	struct work_struct work;
	unsigned int data;
};

#define IOM_FPGA_DOT_ADDRESS 0x08000210
#define IOM_FND_ADDRESS 0x08000004
#define FND_CNT 4

static const unsigned char DOT_TABLE[DOT_SHAPE_CNT][10] = {
	{0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
	{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
	{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
	{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
	{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
	{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
	{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03}, // 9
	
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // blank
};
static const unsigned int TIME_MAX = 36000; // 3540.0sec(59min) 59.0sec 0.9sec
static const unsigned int DEBOUNCING = 10; // just 10 jiffies(0.01sec)

static void fpga_print_dot(enum dot_shape dot);
static void fpga_print_fnd(unsigned int time);

static void fpga_timer_callback(unsigned long unused);

static void fpga_timer_do_wq(struct work_struct* work);

// MMIO
static unsigned char *iom_fpga_dot_addr;
static unsigned char *iom_fpga_fnd_addr;
// Workqueue
static struct workqueue_struct* single_thread_wq;
// Timer
static struct timer_list fpga_timer;

static unsigned int cur_time; // 0<=time<TIME_MAX

static void fpga_print_dot(enum dot_shape dot){
	int i;
	for(i = 0; i < 10; i++)
		outw(DOT_TABLE[dot][i] & 0x7F, (unsigned int) iom_fpga_dot_addr + i * 2);
}

static void fpga_print_fnd(unsigned int time){
	unsigned char fnd[FND_CNT];
	unsigned short int fnd_short;
	fnd[0] = (time/600)/10, fnd[1] = (time/600)%10; // minutes
	fnd[2] = ((time%600)/10)/10, fnd[3] = ((time%600)/10)%10; // seconds
	fnd_short = fnd[0] << 12 | fnd[1] << 8 | fnd[2] << 4 | fnd[3];
	outw(fnd_short, (unsigned int)iom_fpga_fnd_addr);
}

/* 
 * It is a callback function that is called every 0.1 seconds and modifies the FPGA.
 */
static void fpga_timer_callback(unsigned long unused){
	struct single_thread_wq_elem* w; 
	
	cur_time = (cur_time + 1) % TIME_MAX;
	
	w = kmalloc(sizeof(struct single_thread_wq_elem), GFP_ATOMIC);
	if(!w){
		LOG(LOG_LEVEL_INFO, "Heap lack");
		return;
	}
    w->data = cur_time;
	INIT_WORK(&w->work, fpga_timer_do_wq);
	queue_work(single_thread_wq, &w->work);
	return;
}

/*
 * Work of fpga_timer_callback().
 * This function prints fpga and adds timer.
 */
static void fpga_timer_do_wq(struct work_struct* work){
	struct single_thread_wq_elem* w = container_of(work, struct single_thread_wq_elem, work);
	fpga_print_dot((w->data % 600) % 10);
	fpga_print_fnd(w->data);
	fpga_timer.expires = get_jiffies_64() + HZ/10;
	fpga_timer.function = fpga_timer_callback;
	add_timer(&fpga_timer);
	kfree(w);
}

/* 
 * Called once by module_init. 
 * Return 0 on Error.
 */
int timer_init(void){
	// init MMIO
	iom_fpga_dot_addr = ioremap(IOM_FPGA_DOT_ADDRESS, 0x10);
    iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
	// init timer
	init_timer(&fpga_timer);
	// init wq
	single_thread_wq = create_singlethread_workqueue("single_thread_wq");	
	if(!single_thread_wq){
		LOG(LOG_LEVEL_INFO, "Heap lack");
		return 0;
	}
	return 1;
}

/* 
 * Called once by module_exit. 
 */
void timer_del(void){
	// timer
	del_timer_sync(&fpga_timer);
    cur_time = 0;
	fpga_print_dot(BLANK);
	fpga_print_fnd(cur_time);
	// wq
	flush_workqueue(single_thread_wq);
	destroy_workqueue(single_thread_wq);
	// MMIO
	iounmap(iom_fpga_dot_addr);
	iounmap(iom_fpga_fnd_addr);
}

void timer_run(void){
    fpga_timer.expires = get_jiffies_64() + HZ/10;
    fpga_timer.function = fpga_timer_callback;
    add_timer(&fpga_timer);
	LOG(LOG_LEVEL_INFO, "timer is registered");
}

void timer_pause(void){
	del_timer_sync(&fpga_timer);
	LOG(LOG_LEVEL_INFO, "timer is paused");
}