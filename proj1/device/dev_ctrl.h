#ifndef DEV_CTRL_H
#define DEV_CTRL_H

#include "../ipc/payload/record.h"

/* text lcd */
#define TEXT_LCD_MAX_BUFF 32
#define TEXT_LCD_MAX_LINE 16 // TEXT_LCD_MAX_BUFF/2

/* abstracted device Input*/
enum input_type {
	// switch
	S1_HOLD = 0,
	S1,
	S2,
	S3,
	S4,
	S5,
	S6,
	S7,
	S8,
	S9,
	S4_AND_S6,
	// dip_switch
	RESET,
	// event
	BACK,
	VOL_UP,
	VOL_DOWN,
	// nothing pressed
	DEFAULT
};

/* definition of LED actions to be executed concurrently with device Input. */
enum led_action {
	LED_OFF = 0,
	LED1_ON,
	LED3_AND_LED4_TOGGLE,
	LED7_AND_8_TOGGLE,
	LED5_ON
};

struct device_controller;
struct device_controller* device_controller_create();
void device_controller_destroy(struct device_controller* dc);
enum input_type device_controller_get_input(struct device_controller* dc, enum led_action ac);
void device_controller_fnd_print(struct device_controller* dc, char key[KEY_DIGIT + 1]);
void device_controller_fnd_off(struct device_controller* dc);
void device_controller_lcd_print(struct device_controller* dc, char mode[TEXT_LCD_MAX_LINE], char data[TEXT_LCD_MAX_LINE]);
void device_controller_lcd_off(struct device_controller* dc);

#endif // DEV_CTRL_H
