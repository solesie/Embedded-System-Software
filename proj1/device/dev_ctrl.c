/*
 * Implement device management and abstraction of device I/O logic.
 * All device inputs are defined upon release after being pressed.
 */

#include <stddef.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/input.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "dev_ctrl.h"
#include "../common/logging.h"

#define DEVICE_CNT 7

#define DEBOUNCING 10000 // 1ms

/* mmap */
#define PAGE_SIZE 4096 // mmap use 1 page(4KB)
#define FPGA_BASE_ADDRESS 0x08000000 // fpga_base address
#define LED_ADDR 0x16 

/* event button */
#define EV_BUFF_SIZE 64
#define VOL_DOWN_CODE 114
#define VOL_UP_CODE 115
#define BACK_CODE 158

/* switch */
#define S1_HOLD_REQUIRED_SEC 2

/* led */
#define LED_TOGGLE_MIN_INTERVAL 1

/* motor */
#define MOTOR_TOGGLE_MIN_INTERVAL 2

/* device files */
static const char DEVICES[DEVICE_CNT][25] = {
	// output devices
	"/dev/fpga_fnd",
	"/dev/mem",
	"/dev/fpga_text_lcd",
	"/dev/fpga_step_motor",
	// input devices
	"/dev/input/event0",
	"/dev/fpga_push_switch",
	"/dev/fpga_dip_switch"
};

enum device_idx {
	FND = 0,
	LED,
	TEXT_LCD,
	MOTOR,
	EVENT,
	SWITCH,
	DIP_SWITCH // RESET button
};

/* open flags of each device file */
static const int OPEN_FLAGS[DEVICE_CNT] = {
    O_RDWR,
    O_RDWR | O_SYNC,
    O_WRONLY,
    O_WRONLY,
    O_RDONLY | O_NONBLOCK,
    O_RDWR,
	O_RDWR
};

struct device_controller{
	int fd[DEVICE_CNT];
	
	// led 
	unsigned long* fpga_mmap_addr;
	time_t led_set_t;
	
	// motor 
	time_t motor_run_t;
};

static inline unsigned char* get_fpga_led_mmap_addr(struct device_controller* dc);
static enum input_type find_switch_input_type(struct device_controller* dc, enum led_action ac);
static inline int sw_buf_to_bits(unsigned char buf[SWITCH_CNT]);
static void set_led(struct device_controller* dc, enum led_action ac);
static inline void set_led_after_debounce(struct device_controller* dc, enum led_action ac);
static int wait_all_sw_release_on_undefined(const int prev_bits, const int pressing_bits, struct device_controller* dc, enum led_action ac);

struct device_controller* device_controller_create(){
	int i;
	struct device_controller* ret = (struct device_controller*)malloc(sizeof(struct device_controller));

	for(i = 0; i < DEVICE_CNT; ++i){
		ret->fd[i] = open(DEVICES[i], OPEN_FLAGS[i]);
		if(ret->fd[i] < 0){
            LOG(LOG_LEVEL_ERROR, "device_manager_create open: %d", errno);
            killpg(getpgrp(), SIGABRT);
        }
	}

    ret->fpga_mmap_addr = (unsigned long*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, ret->fd[LED], FPGA_BASE_ADDRESS);
	ret->led_set_t = time(NULL);
	ret->motor_run_t = time(NULL);
	return ret;
}

void device_controller_destroy(struct device_controller* dc){
	munmap((void*)dc->fpga_mmap_addr, PAGE_SIZE);
	int i;
	for(i = 0; i < DEVICE_CNT; ++i){
		close(dc->fd[i]);
	}
	free(dc);
}

static inline unsigned char* get_fpga_led_mmap_addr(struct device_controller* dc){
    return (unsigned char*)((void*)dc->fpga_mmap_addr + LED_ADDR);
}

static inline int sw_buf_to_bits(unsigned char sw_buf[SWITCH_CNT]){
	int ret = 0;
	int i;
	for(i = 0; i < SWITCH_CNT; ++i)
		ret |= (sw_buf[i] << (SWITCH_CNT - 1 - i));
	return ret;
}

static void set_led(struct device_controller* dc, enum led_action ac){
	const static unsigned char LED1 = 0x80;
	const static unsigned char LED3 = 0x20;
	const static unsigned char LED4 = 0x10;
	const static unsigned char LED5 = 0x08;
	const static unsigned char LED7 = 0x02;
	const static unsigned char LED8 = 0x01;
	unsigned char cur_data = 0;
	unsigned char data;
	time_t cur_t = time(NULL);
	volatile unsigned char* led_addr = get_fpga_led_mmap_addr(dc);
	cur_data = *led_addr; // read LED
	
	switch(ac){
		case LED_OFF:
			data = 0;
			break;
		case LED1_ON:
			data = LED1;
			break;
		case LED3_AND_LED4_TOGGLE:
			data = cur_data == LED3 || cur_data == LED4 ? cur_data : LED3;
			if(cur_t - dc->led_set_t >= LED_TOGGLE_MIN_INTERVAL){
				data = cur_data == LED3 ? LED4 : LED3;
			}
			break;
		case LED7_AND_8_TOGGLE:
			data = cur_data == LED7 || cur_data == LED8 ? cur_data : LED7;
			if(cur_t - dc->led_set_t >= LED_TOGGLE_MIN_INTERVAL)
				data = cur_data == LED7 ? LED8 : LED7;
			break;
		case LED5_ON:
			data = LED5;
			break;
		case LED_ALL:
			data = 0xFF;
			break;
	}
	// write LED.
	// but if the reset button is pressed, it has been observed that the value 22 is overwritten on the LED...
	*led_addr = data;
	dc->led_set_t = cur_t;
}

static inline void set_led_after_debounce(struct device_controller* dc, enum led_action ac){
	usleep(DEBOUNCING);
	set_led(dc, ac);
}

/* if the switch state becomes undefined, wait until all switchs are released, then return 1. */
static int wait_all_sw_release_on_undefined(const int prev_bits, const int pressing_bits, struct device_controller* dc, enum led_action ac){
	if((pressing_bits | prev_bits) != prev_bits){
		unsigned char buf[SWITCH_CNT];
		size_t size = SWITCH_CNT * sizeof(unsigned char);
		LOG(LOG_LEVEL_DEBUG, "undefined switch pressed");
		while(1){
			set_led_after_debounce(dc, ac);
			read(dc->fd[SWITCH], buf, size);
			if(sw_buf_to_bits(buf) == 0){
				LOG(LOG_LEVEL_DEBUG, "undefined switch released");
				return 1;
			}
		}
	}
	return 0;
}

/* if undefined switch pressed, then return DEFAULT */
static enum input_type find_switch_input_type(struct device_controller* dc, enum led_action ac) {
	const static int S4_AND_S6_BITS = 0x28; // 0b000101000
	const static int S1_BITS = 0x100; // 0b100000000
	const static int S2_BITS = 0x80; // 0b010000000
	const static int S9_BITS = 0x1; // 0b000000001
	unsigned char buf[SWITCH_CNT];
	size_t size = SWITCH_CNT * sizeof(unsigned char);
	int buf_bits, pressing_bits; // if buf_bits != pressing_bits, then return DEFAULT
	int i;
	
	read(dc->fd[SWITCH], buf, size);
	buf_bits = sw_buf_to_bits(buf);
	
	// if the DEBOUNCING is increased, the s4 and s6 can be pressed simultaneously.
	if(buf_bits == S4_AND_S6_BITS){
		LOG(LOG_LEVEL_DEBUG, "s4 and s6 pressed");
		while(1){
			set_led_after_debounce(dc, ac);
			read(dc->fd[SWITCH], buf, size);
			pressing_bits = sw_buf_to_bits(buf);
			if(wait_all_sw_release_on_undefined(S4_AND_S6_BITS, pressing_bits, dc, ac)){
				return DEFAULT;
			}
			if(pressing_bits == 0){
				LOG(LOG_LEVEL_DEBUG, "s4 and s6 released");
				return S4_AND_S6;
			}
		}
	}

	// s1
	if(buf_bits == S1_BITS){
		LOG(LOG_LEVEL_DEBUG, "s1 pressed. start stop watch");
		time_t start_t, end_t;
		start_t = time(NULL);

		while(1){
			set_led_after_debounce(dc, ac);
			read(dc->fd[SWITCH], buf, size);
			pressing_bits = sw_buf_to_bits(buf);
			if(wait_all_sw_release_on_undefined(S1_BITS, pressing_bits, dc, ac)){
				return DEFAULT;
			}
			if(pressing_bits == 0){
				end_t = time(NULL);
				LOG(LOG_LEVEL_DEBUG, "s1 released(elapsed sec %ld)", end_t - start_t);
				if (end_t - start_t >= S1_HOLD_REQUIRED_SEC) {
                	return S1_HOLD;
            	}
				return S1;
			}
		}
	}

	// s2 ~ s9
	unsigned int mask;
	for(i = 2, mask = S2_BITS; mask >= S9_BITS; mask >>= 1, ++i){
		if(buf_bits == mask){
			LOG(LOG_LEVEL_DEBUG, "s%d pressed", i);
			while(1){
				set_led_after_debounce(dc, ac);
				read(dc->fd[SWITCH], buf, size);
				pressing_bits = sw_buf_to_bits(buf);
				// if there is even a slight timing overlap where s4 and s6 are pressed simultaneously, 
				// it is considered that s4 and s6 have been pressed at the same time.
				if(pressing_bits == S4_AND_S6_BITS){
					LOG(LOG_LEVEL_DEBUG, "s4 and s6 presssed");
					while(1){
						set_led_after_debounce(dc, ac);
						read(dc->fd[SWITCH], buf, size);
						pressing_bits = sw_buf_to_bits(buf);
						if(wait_all_sw_release_on_undefined(S4_AND_S6_BITS, pressing_bits, dc, ac)){
							return DEFAULT;
						}
						if(pressing_bits == 0){
							LOG(LOG_LEVEL_DEBUG, "s4 and s6 released");
							return S4_AND_S6;
						}
					}
				}
				if(wait_all_sw_release_on_undefined(mask, pressing_bits, dc, ac)){
					return DEFAULT;
				}
				if(pressing_bits == 0){
					LOG(LOG_LEVEL_DEBUG, "s%d released", i);
					switch(i){
						case 2: return S2;
						case 3: return S3;
						case 4: return S4;
						case 5: return S5;
						case 6: return S6;
						case 7: return S7;
						case 8: return S8;
						case 9: return S9;
					}
				}
			}
		}
	}
	
	return DEFAULT;
}

/* if an input is pressed, waits until the input is released and then returns the input_type. 
 * led_action means the action of the LED during input.
 * return abstracted device Input.*/
enum input_type device_controller_get_input(struct device_controller* dc, enum led_action ac){
	struct input_event ev[EV_BUFF_SIZE];
	int size = sizeof(struct input_event);
	int rd;
	unsigned char dip_sw_buff = 0;

	set_led_after_debounce(dc, ac);

	// event: BACK, VOL_UP, VOL_DOWN
	if((rd = read(dc->fd[EVENT], ev, size * EV_BUFF_SIZE)) >= size){
		enum input_type ret = DEFAULT;
		if(ev[0].code == VOL_DOWN_CODE) ret = VOL_DOWN;
		if(ev[0].code == VOL_UP_CODE) ret = VOL_UP;
		if(ev[0].code == BACK_CODE) ret = BACK;
		if(ret == DEFAULT) return ret; // PROG button ignore
		LOG(LOG_LEVEL_DEBUG, "event %d pressed", ev[0].code);
		while(1){
			set_led_after_debounce(dc, ac);
			read(dc->fd[EVENT], ev, sizeof(struct input_event) * EV_BUFF_SIZE);
			if(ev[0].value == 0){ // key release
				LOG(LOG_LEVEL_DEBUG, "event %d released", ev[0].code);
				return ret;
			}
		}
	}

	// dip switch(=reset)
	read(dc->fd[DIP_SWITCH], &dip_sw_buff, 1);
	if(dip_sw_buff == 0){
		LOG(LOG_LEVEL_DEBUG, "reset pressed");
		while(1){
			set_led_after_debounce(dc, ac);
			read(dc->fd[DIP_SWITCH], &dip_sw_buff, 1);
			if(dip_sw_buff != 0){
				LOG(LOG_LEVEL_DEBUG, "reset released");
				return RESET;
			}
		}
	}

	// switch
	return find_switch_input_type(dc, ac);
}

/* although the LED action "during input" can be defined 
 * through device_controller_get_input(), 
 * it is declared because there is a need to define the LED behavior 
 * after the "input has ended" as well. */
void device_controller_led_off(struct device_controller* dc){
	set_led(dc, LED_OFF);
}

void device_controller_fnd_print(struct device_controller* dc, const char numbers[FND_MAX + 1]){
	unsigned char data[4];
	int i;
	for(i = 0; i < FND_MAX; ++i){
		if(numbers[i] < 0x30 || numbers[i] > 0x39){
			LOG(LOG_LEVEL_ERROR, "device_controller_fnd_print: numbers should be number %d", numbers[i]);
			killpg(getpgrp(), SIGABRT);
		}
		data[i] = numbers[i] - 0x30;
	}
	if(write(dc->fd[FND], data, FND_MAX) < 0){
		LOG(LOG_LEVEL_ERROR, "device_controller_fnd_print write: %d", errno);
		killpg(getpgrp(), SIGABRT);
	}
}

void device_controller_fnd_off(struct device_controller* dc){
	unsigned char data[4];
	memset(data, 0, sizeof(data));
	if(write(dc->fd[FND], data, FND_MAX) < 0){
		LOG(LOG_LEVEL_ERROR, "device_controller_fnd_off write: %d", errno);
		killpg(getpgrp(), SIGABRT);
	}
}

void device_controller_lcd_print(struct device_controller* dc, const char mode[TEXT_LCD_MAX_LINE], const char data[TEXT_LCD_MAX_LINE]){
	char str[TEXT_LCD_MAX_BUFF];
	int mode_len, data_len;
	memset(str, 0, sizeof(str));
	mode_len = strlen(mode);
	data_len = strlen(data);

	strncat(str, mode, mode_len);
	memset(str + mode_len, ' ', TEXT_LCD_MAX_LINE - mode_len);
	strncat(str, data, data_len);
	memset(str + TEXT_LCD_MAX_LINE + data_len, ' ', TEXT_LCD_MAX_LINE - data_len);
	
	write(dc->fd[TEXT_LCD], str, TEXT_LCD_MAX_BUFF);
}

void device_controller_lcd_off(struct device_controller* dc){
	char str[TEXT_LCD_MAX_BUFF];
	memset(str, ' ', TEXT_LCD_MAX_BUFF);
	write(dc->fd[TEXT_LCD], str, TEXT_LCD_MAX_BUFF);
}

void device_controller_motor_on(struct device_controller* dc){
	unsigned char motor_state[3];
	motor_state[0] = 1; // motor start
	motor_state[1] = 1; // motor direction
	motor_state[2] = 200; // motor speed
	dc->motor_run_t = time(NULL);
	write(dc->fd[MOTOR], motor_state, 3);
}

void device_controller_motor_off(struct device_controller* dc){
	time_t end;
	unsigned char motor_state[3];
	motor_state[0] = 0; // motor stop
	motor_state[1] = 1; // motor direction
	motor_state[2] = 200; // motor speed
	while(1){
		usleep(DEBOUNCING);
		end = time(NULL);
		if(end - dc->motor_run_t >= MOTOR_TOGGLE_MIN_INTERVAL){
			write(dc->fd[MOTOR], motor_state, 3);
			return;
		}
	}
}

void device_controller_led_all_on(struct device_controller* dc){
	time_t start, end;
	start = time(NULL);
	set_led(dc, LED_ALL);
	while(1){
		usleep(DEBOUNCING);
		end = time(NULL);
		if(end - start >= LED_TOGGLE_MIN_INTERVAL + 1){
			return;
		}
	}
}
