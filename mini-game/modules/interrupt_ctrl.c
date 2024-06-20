#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/irq.h>
#include "logging.h"

static const unsigned int DEBOUNCING = 10; // just 10 jiffies(0.01sec)

static struct completion over; // To block JAVA's thread.
static int waked_by_back_handler;
static spinlock_t sl; // atomic context

/*
 * It has been observed that interrupt handlers, 
 * despite being able to directly read from hardware registers whether the signal is FALLING or RISING, 
 * often retrieve incorrect values due to time delays. 
 * Therefore, if the interval between interrupts is too short, it should be ignored.
 */
static unsigned long long back_prev_pressed;
static unsigned long long back_prev_released;

static irqreturn_t back_inter_handler(int irq, void* dev_id){
	int v;
	unsigned long long cur;
	spin_lock(&sl);
	v = gpio_get_value(IMX_GPIO_NR(1, 12));
	cur = get_jiffies_64();
	if(v == 0){
		if(back_prev_released < back_prev_pressed || cur - back_prev_released <= DEBOUNCING) {
			spin_unlock(&sl);
			return IRQ_HANDLED;
		}
		back_prev_pressed = cur;
	}
	else{
		if(back_prev_pressed < back_prev_released) {
			spin_unlock(&sl);
			return IRQ_HANDLED;
		}
		back_prev_released = cur;
		spin_unlock(&sl);
		return IRQ_HANDLED;
	}
	
	back_prev_pressed = cur;
	
	waked_by_back_handler = 1;
	complete(&over);
	spin_unlock(&sl);
	return IRQ_HANDLED;
}

/*
 * Block JAVA's thread.
 */
int interrupt_wait_back_intr(void){
	init_completion(&over);
	wait_for_completion(&over);
	return waked_by_back_handler;
}
void interrupt_wake_back_waiting_thread(void){
	spin_lock(&sl);
	waked_by_back_handler = 0;
	complete(&over);
	spin_unlock(&sl);
}

/*
 * Called once by module_init.
 */
void interrupt_init(void){
	spin_lock_init(&sl);
}

/*
 * The correct place to call request_irq is when the device is first opened.
 * Called by open().
 */
void interrupt_request(void){
	int irq, req;
	// init back interrupt
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	req = request_irq(irq, back_inter_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "back", 0);
	if(req) LOG(LOG_LEVEL_INFO, "Back button is not available.");

	back_prev_pressed = 0;
	back_prev_released = 0;
}

/*
 * Called by close().
 */
void interrupt_free(void){
	back_prev_pressed = 0;
	back_prev_released = 0;
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
}