/*
 * ioctl_timer_ctrl.c implements MAIN logic and is used by timer_driver.c.
 * This is thread safe data structure.
 */
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/atomic.h>
#include "ioctl_timer_ctrl.h"
#include "logging.h"

#define COUNTDOWN 3

static struct timer_list timer;
static struct completion over;
static atomic_t in_use = ATOMIC_INIT(0);

static unsigned int timer_interval;
static unsigned int timer_cnt;
static char timer_init[FND_MAX + 1];

static int validate(void){
	int i;
    int	v = 0;
	if(timer_interval < 1 || timer_interval > 100) return 0;
	if(timer_cnt < 1 || timer_cnt > 200) return 0;
	for(i = 0; i < FND_MAX; ++i){
		if(timer_init[i] != '0'){
			if(v) return 0;
			v = 1;
		}
	}
	return 1;
}

static void wait_dip_switch(void){
	while(1){
		usleep_range(1000, 1010);
		if(!fpga_dip_switch_read()) continue;

		while(1){
			usleep_range(1000, 1010);
			if(fpga_dip_switch_read()) continue;
			LOG(LOG_LEVEL_DEBUG, "RESET button pressed");
			return;
		}
	}
}

/* countdown callback function(per 1sec) */
static void countdown(unsigned long cur){
	if(++cur >= COUNTDOWN){
		complete(&over);
		return;
	}
	fpga_countdown();

	timer.expires = get_jiffies_64() + HZ; // 1sec
	timer.data = cur;
	timer.function = countdown;
	add_timer(&timer);
}

/* timer callback function*/
static void increase(unsigned long cur){
	if(++cur >= timer_cnt){
		fpga_set_countdown();

		timer.expires = get_jiffies_64() + HZ; // 1sec
		timer.data = 0;
		timer.function = countdown;
		add_timer(&timer);
		return;
	}
	fpga_increase();
	
	timer.expires = get_jiffies_64() + (timer_interval * HZ / 10);
	timer.data = cur;
	timer.function = increase;
	add_timer(&timer);
}

void create_timer_ctrl(void){
	init_timer(&timer);
	init_completion(&over);
	fpga_ioremap();
}

void destroy_timer_ctrl(void){
	del_timer_sync(&timer);
	fpga_iounmap();
}

/* Return 0 on ERROR. */
int set_timer_ctrl(unsigned int _timer_interval, unsigned int _timer_cnt, char _timer_init[FND_MAX + 1]){
	int i;
	if(atomic_cmpxchg(&in_use, 0, 1))
		return 0;

	timer_interval = _timer_interval;
	timer_cnt = _timer_cnt;
	for(i = 0; i < FND_MAX; ++i) timer_init[i] = _timer_init[i];
	if(!validate()) {
		timer_interval = 0;
		timer_cnt = 0;
		memset(timer_init, 0, sizeof(timer_init));
		atomic_set(&in_use, 0);
		return 0;
	}

	fpga_on(timer_cnt, COUNTDOWN, timer_init);

	return 1;
}

/* Return 0 on ERROR. */
int run_timer_ctrl(void){
	if(atomic_read(&in_use) == 0) 
		return 0;

	// wait until the RESET button is pressed and released.
	wait_dip_switch();

	// Let's run timer...
	del_timer_sync(&timer);
	timer.expires = get_jiffies_64() + (timer_interval * HZ / 10);
	timer.data = 0;
	timer.function = increase;
	add_timer(&timer);

	// Sleep until timer is completed.
	wait_for_completion(&over);

	// Turn off timer controller.
	fpga_off();
	timer_interval = 0;
	timer_cnt = 0;
	memset(timer_init, 0, sizeof(timer_init));
	atomic_set(&in_use, 0);
	
	return 1;
}
