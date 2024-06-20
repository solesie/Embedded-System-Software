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
#include "logging.h"

enum dot_shape{
	ZERO = 0, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, BLANK, DOT_SHAPE_CNT
};

struct stopwatch{
	unsigned int cur_time; // 0<=time<TIME_MAX
	int is_started; // boolean
	int is_paused; // boolean
	int is_over_timer_running; // boolean
	int is_over; // boolean
};

struct wq_elem{
	struct work_struct work;
	struct stopwatch data;
};

#define IOM_FPGA_DOT_ADDRESS 0x08000210
#define IOM_FND_ADDRESS 0x08000004
#define FND_CNT 4

static const unsigned char dot_table[DOT_SHAPE_CNT][10] = {
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

static irqreturn_t home_inter_handler(int irq, void* dev_id);
static irqreturn_t back_inter_handler(int irq, void* dev_id);
static irqreturn_t volup_inter_handler(int irq, void* dev_id);
static irqreturn_t voldown_inter_handler(int irq, void* dev_id);
static void fpga_timer_callback(unsigned long unused);
static void over_timer_callback(unsigned long unused);

static void home_do_wq(struct work_struct* unused);
static void back_do_wq(struct work_struct* work);
static void volup_do_wq(struct work_struct* work);
static void voldown_do_wq(struct work_struct* work);
static void fpga_timer_do_wq(struct work_struct* work);
static void over_timer_do_wq(struct work_struct* work);

// MMIO
static unsigned char *iom_fpga_dot_addr;
static unsigned char *iom_fpga_fnd_addr;
// Blocking kernel process
static struct completion over;
// Workqueue
static struct workqueue_struct* single_thread_wq;
// Timer
static struct timer_list fpga_timer;
static struct timer_list over_timer;
// Stopwatch
static struct stopwatch sw;
// Debouncing
static unsigned int home_prev_pressed;
static unsigned int home_prev_released; 
static unsigned int back_prev_pressed;
static unsigned int back_prev_released; 
static unsigned int volup_prev_pressed;
static unsigned int volup_prev_released; 
static unsigned int voldown_prev_pressed;
static unsigned int voldown_prev_released; 

static void fpga_print_dot(enum dot_shape dot){
	int i;
	for(i = 0; i < 10; i++)
		outw(dot_table[dot][i] & 0x7F, (unsigned int) iom_fpga_dot_addr + i * 2);
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
 * It has been observed that interrupt handlers, 
 * despite being able to directly read from hardware registers whether the signal is FALLING or RISING, 
 * often retrieve incorrect values due to time delays. 
 * Therefore, if the interval between interrupts is too short, it should be ignored.
 * This logic is included in top half interrupt handler.
 */

/*
 * IRQ handler for home button.
 * The initial values of sw are set and go to bottom half phase.
 * The bottom half will print dot(ZERO) and start stopwatch.
 */
static irqreturn_t home_inter_handler(int irq, void* dev_id){
	int v = gpio_get_value(IMX_GPIO_NR(1, 11));
	unsigned int cur = get_jiffies_64();
	struct wq_elem* w;
	// FALLING: v = 0, RISING: v = 1
	if(v == 0){
		// FALLING -> FALLING ignored
		if(home_prev_released < home_prev_pressed) return IRQ_HANDLED;
		// Too fast RISING -> FALLING ignored
		if(cur - home_prev_released <= DEBOUNCING) return IRQ_HANDLED;
		home_prev_pressed = cur;
	}
	else{
		// RISING -> RISING ignored
		if(home_prev_pressed < home_prev_released) return IRQ_HANDLED;
		home_prev_released = cur;
		return IRQ_HANDLED;
	}
	
	if(sw.is_started || sw.is_over) return IRQ_HANDLED;
	home_prev_pressed = cur;
	sw.is_started = 1;
	
	w = kmalloc(sizeof(struct wq_elem), GFP_ATOMIC);
	if(!w) {
		LOG(LOG_LEVEL_INFO, "Heap lack");
		return IRQ_HANDLED;
	}
	INIT_WORK(&w->work, home_do_wq);
	queue_work(single_thread_wq, &w->work);

	return IRQ_HANDLED;
}

/*
 * IRQ handler for back button.
 * Values for pause are saved and go to bottom half phase.
 * The bottom half will pause stopwatch.
 */
static irqreturn_t back_inter_handler(int irq, void* dev_id){
	int v = gpio_get_value(IMX_GPIO_NR(1, 12));
	unsigned int cur = get_jiffies_64();
	struct wq_elem* w;
	if(v == 0){
		if(back_prev_released < back_prev_pressed) return IRQ_HANDLED;
		if(cur - back_prev_released <= DEBOUNCING) return IRQ_HANDLED;
		back_prev_pressed = cur;
	}
	else{
		if(back_prev_pressed < back_prev_released) return IRQ_HANDLED;
		back_prev_released = cur;
		return IRQ_HANDLED;
	}
	
	if(!sw.is_started) return IRQ_HANDLED;
	back_prev_pressed = cur;
	sw.is_paused = !sw.is_paused;
	
	w = kmalloc(sizeof(struct wq_elem), GFP_ATOMIC);
	if(!w) {
		LOG(LOG_LEVEL_INFO, "Heap lack");
		return IRQ_HANDLED;
	}
	memcpy(&w->data, &sw, sizeof(struct stopwatch));
	INIT_WORK(&w->work, back_do_wq);
	queue_work(single_thread_wq, &w->work);
	return IRQ_HANDLED;
}

/*
 * IRQ handler for vol up button.
 * The sw is reset and goto bottom half phase.
 */
static irqreturn_t volup_inter_handler(int irq, void* dev_id){
	int v = gpio_get_value(IMX_GPIO_NR(2, 15));
	unsigned int cur = get_jiffies_64();
	struct wq_elem* w;
	if(v == 0){
		if(volup_prev_released < volup_prev_pressed) return IRQ_HANDLED;
		if(cur - volup_prev_released <= DEBOUNCING) return IRQ_HANDLED;
		volup_prev_pressed = cur;
	}
	else{
		if(volup_prev_pressed < volup_prev_released) return IRQ_HANDLED;
		volup_prev_released = cur;
		return IRQ_HANDLED;
	}

	if(!sw.is_started) return IRQ_HANDLED;
	volup_prev_pressed = cur;
	sw.cur_time = 0;
	sw.is_started = 0;
	sw.is_paused = 0;
	
	w = kmalloc(sizeof(struct wq_elem), GFP_ATOMIC);
	if(!w){
		LOG(LOG_LEVEL_INFO, "Heap lack");
		return IRQ_HANDLED;
	}
	memcpy(&w->data, &sw, sizeof(struct stopwatch));
	INIT_WORK(&w->work, volup_do_wq);
	queue_work(single_thread_wq, &w->work);
	return IRQ_HANDLED;
}

/*
 * IRQ handler for vol down button.
 * The bottom half will start stopwatch over logic.
 */
static irqreturn_t voldown_inter_handler(int irq, void* dev_id){
	int v = gpio_get_value(IMX_GPIO_NR(5, 14));
	unsigned int cur = get_jiffies_64();
	struct wq_elem* w = kmalloc(sizeof(struct wq_elem), GFP_ATOMIC);
	if(!w){
		LOG(LOG_LEVEL_INFO, "Heap lack");
		return IRQ_HANDLED;
	}
	if(sw.is_over) return IRQ_HANDLED;
	
	if(v == 0){
		if(voldown_prev_released < voldown_prev_pressed) return IRQ_HANDLED;
		if(cur - voldown_prev_released <= DEBOUNCING) return IRQ_HANDLED;
		voldown_prev_pressed = cur;
		sw.is_over_timer_running = 1;
	}
	else{
		if(voldown_prev_pressed < voldown_prev_released) return IRQ_HANDLED;
		voldown_prev_released = cur;
		sw.is_over_timer_running = 0;
	}
	memcpy(&w->data, &sw, sizeof(struct stopwatch));
	INIT_WORK(&w->work, voldown_do_wq);
	queue_work(single_thread_wq, &w->work);
	return IRQ_HANDLED;
}

/* 
 * It is a callback function that is called every 0.1 seconds and modifies the FPGA.
 */
static void fpga_timer_callback(unsigned long unused){
	struct wq_elem* w; 
	
	// Managing timers in the bottom half can lead to a situation 
	// where a timer that should not be executed might run 
	// if the bottom half has not yet been scheduled. 
	// Consider this race condition.
	if(sw.is_over) return;
	if(sw.is_paused) return;
	if(!sw.is_started) return;
	
	sw.cur_time = (sw.cur_time + 1) % TIME_MAX;
	
	w = kmalloc(sizeof(struct wq_elem), GFP_ATOMIC);
	if(!w){
		LOG(LOG_LEVEL_INFO, "Heap lack");
		return;
	}
	memcpy(&w->data, &sw, sizeof(struct stopwatch));
	INIT_WORK(&w->work, fpga_timer_do_wq);
	queue_work(single_thread_wq, &w->work);
	return;
}

/*
 * It is a callback function that conducts a termination countdown.
 */
static void over_timer_callback(unsigned long unused){
	struct wq_elem* w; 
	
	// Consider the race condition.
	if(!sw.is_over_timer_running) return;

	sw.is_over = 1;
	
	w = kmalloc(sizeof(struct wq_elem), GFP_ATOMIC);
	if(!w){
		LOG(LOG_LEVEL_INFO, "Heap lack");
		return;
	}
	INIT_WORK(&w->work, over_timer_do_wq);
	queue_work(single_thread_wq, &w->work);
	return;
}

/* 
 * Work of home_inter_handler().
 * This function prints dot(ZERO) and start fpga_timer.
 */
static void home_do_wq(struct work_struct* work){
	struct wq_elem* w = container_of(work, struct wq_elem, work);
	fpga_print_dot(ZERO);
	fpga_timer.expires = get_jiffies_64() + HZ/10;
	fpga_timer.function = fpga_timer_callback;
	add_timer(&fpga_timer);
	kfree(w);
}

/*
 * Work of back_inter_handler().
 * This function deletes or restarts fpga_timer.
 */
static void back_do_wq(struct work_struct* work){
	struct wq_elem* w = container_of(work, struct wq_elem, work);
	if(w->data.is_paused){
		del_timer_sync(&fpga_timer);
	}
	else{
		fpga_timer.expires = get_jiffies_64() + HZ/10;
		fpga_timer.function = fpga_timer_callback;
		add_timer(&fpga_timer);
	}
	kfree(w);
}

/*
 * Work of volup_inter_handler().
 * This function deletes fpga_timer and reset fpga.
 */
static void volup_do_wq(struct work_struct* work){
	struct wq_elem* w = container_of(work, struct wq_elem, work);
	del_timer_sync(&fpga_timer);
	fpga_print_fnd(w->data.cur_time);
	fpga_print_dot(ZERO);
	kfree(w);
}

/*
 * Work of voldown_inter_handler().
 * If vol down is pressed, over_timer is added.
 * In released case, over_timer is deleted.
 */
static void voldown_do_wq(struct work_struct* work){
	struct wq_elem* w = container_of(work, struct wq_elem, work);
	if(w->data.is_over_timer_running){
		over_timer.expires = get_jiffies_64() + 3 * HZ;
		over_timer.function = over_timer_callback;
		add_timer(&over_timer);
	}
	else{
		del_timer_sync(&over_timer);
	}
	kfree(w);
}

/*
 * Work of fpga_timer_callback().
 * This function prints fpga and adds timer.
 */
static void fpga_timer_do_wq(struct work_struct* work){
	struct wq_elem* w = container_of(work, struct wq_elem, work);
	fpga_print_dot((w->data.cur_time % 600) % 10);
	fpga_print_fnd(w->data.cur_time);
	fpga_timer.expires = get_jiffies_64() + HZ/10;
	fpga_timer.function = fpga_timer_callback;
	add_timer(&fpga_timer);
	kfree(w);
}

/*
 * Work of over_timer_callback().
 * Reset fpga and wake kernel process.
 */
static void over_timer_do_wq(struct work_struct* work){
	struct wq_elem* w = container_of(work, struct wq_elem, work);
	complete(&over);
	kfree(w);
}

/* 
 * Called once by module_init. 
 * Return 0 on Error.
 */
int stopwatch_init(void){
	// init MMIO
	iom_fpga_dot_addr = ioremap(IOM_FPGA_DOT_ADDRESS, 0x10);
    iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
	// init timer
	init_timer(&fpga_timer);
	init_timer(&over_timer);
	// init wq
	single_thread_wq = create_singlethread_workqueue("single_thread_wq");	
	if(!single_thread_wq){
		LOG(LOG_LEVEL_INFO, "Heap lack");
		return 0;
	}

	init_completion(&over);
	return 1;
}

/* 
 * Called once by module_exit. 
 */
void stopwatch_exit(void){
	// timer
	del_timer_sync(&fpga_timer);
	del_timer_sync(&over_timer);
	// wq
	flush_workqueue(single_thread_wq);
	destroy_workqueue(single_thread_wq);
	// MMIO
	iounmap(iom_fpga_dot_addr);
	iounmap(iom_fpga_fnd_addr);
}

/* 
 * This function operates in a blocking(synchronous) manner and must be called after stopwatch_init.
 * Please note that the existing interrupt handlers for home, back, volup, and voldown may be reset.
 */
void stopwatch_run(void){
	int irq, req;
	
	// It is initialized in advance just in case.
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	
	// init home interrupt
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	req = request_irq(irq, home_inter_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "home", 0);
	if(req) LOG(LOG_LEVEL_INFO, "Home button is not available.");
	// init back interrupt
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	req = request_irq(irq, back_inter_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "back", 0);
	if(req) LOG(LOG_LEVEL_INFO, "Back button is not available.");
	// init volup interrupt
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	req = request_irq(irq, volup_inter_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "volup", 0);
	if(req) LOG(LOG_LEVEL_INFO, "Volup button is not available.");
	// init voldown interrupt
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	req = request_irq(irq, voldown_inter_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "voldown", 0);
	if(req) LOG(LOG_LEVEL_INFO, "Voldown button is not available.");
	
	home_prev_pressed = 0;
	home_prev_released = 0;
	back_prev_pressed = 0;
	back_prev_released = 0;
	volup_prev_pressed = 0;
	volup_prev_released = 0;
	voldown_prev_pressed = 0;
	voldown_prev_released = 0;
	sw.cur_time = 0;
	sw.is_started = 0;
	sw.is_paused = 0;
	sw.is_over_timer_running = 0;
	sw.is_over = 0;
	fpga_print_fnd(sw.cur_time);
	fpga_print_dot(ZERO);
	LOG(LOG_LEVEL_INFO, "stopwatch is ready");
	wait_for_completion(&over);
	
	flush_workqueue(single_thread_wq);
	del_timer_sync(&fpga_timer);
	del_timer_sync(&over_timer);
	home_prev_pressed = 0;
	home_prev_released = 0;
	back_prev_pressed = 0;
	back_prev_released = 0;
	volup_prev_pressed = 0;
	volup_prev_released = 0;
	voldown_prev_pressed = 0;
	voldown_prev_released = 0;
	sw.cur_time = 0;
	sw.is_started = 0;
	sw.is_paused = 0;
	sw.is_over_timer_running = 0;
	sw.is_over = 0;
	fpga_print_dot(BLANK);
	fpga_print_fnd(sw.cur_time);
	
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	
	LOG(LOG_LEVEL_INFO, "stopwatch is over");
}
