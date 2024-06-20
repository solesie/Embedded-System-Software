#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/string.h>
#include "switch_ctrl.h"
#include "logging.h"

#define IOM_LED_ADDRESS 0x08000016
#define IOM_FPGA_TEXT_LCD_ADDRESS 0x08000090

#define LED_MAX 8
#define TEXT_LCD_MAX_BUFF 32
#define TEXT_LCD_MAX_LINE 16
#define INITIAL_LIFE 3 // max = LED_MAX

const static char* TEXT1 = "LIFECOUNT: ";
const static char* TEXT2 = "GAMEOVER";
const static char* TEXT3 = "";

static unsigned char *iom_fpga_led_addr;
static unsigned char *iom_fpga_text_lcd_addr;

static int lifecount;

static void fpga_print_led(void){
	unsigned short _s_value = 0;
	int i;
	for(i = 1; i <= lifecount; ++i)
		_s_value |= (1 << (LED_MAX - i));
	outw(_s_value, (unsigned int)iom_fpga_led_addr);
}

static void fpga_print_text_lcd(const char* TEXT){
	char text_lcd[TEXT_LCD_MAX_BUFF + 1];
	unsigned short int _s_value;
	int i;
	int len = strlen(TEXT);
	
	memcpy(text_lcd, TEXT, len);
	if(strcmp(TEXT, TEXT1) == 0){
		text_lcd[len] = lifecount + '0';
		++len;
	}
	memset(text_lcd + len, ' ', TEXT_LCD_MAX_BUFF - len);

	for(i = 0; i < TEXT_LCD_MAX_BUFF; i += 2){
		// 16bit
		_s_value = ((text_lcd[i] & 0xFF) << 8) | (text_lcd[i + 1] & 0xFF);
		outw(_s_value,(unsigned int)iom_fpga_text_lcd_addr + i);
	}
}

void lifecount_init(void){
	// init MMIO
	iom_fpga_led_addr = ioremap(IOM_LED_ADDRESS, 0x1);
	iom_fpga_text_lcd_addr = ioremap(IOM_FPGA_TEXT_LCD_ADDRESS, 0x32);
	lifecount = INITIAL_LIFE;
}

void lifecount_decrease(void){
	if(lifecount <= 0) return;
	--lifecount;
	fpga_print_led();
	fpga_print_text_lcd(TEXT1);
}

void lifecount_reset(void){
	lifecount = INITIAL_LIFE;
	fpga_print_led();
	fpga_print_text_lcd(TEXT1);
}

void lifecount_off(void){
	lifecount = 0;
	fpga_print_led();
	fpga_print_text_lcd(TEXT3);
}

void lifecount_gameover(void){
	lifecount = 0;
	fpga_print_led();
	fpga_print_text_lcd(TEXT2);
}

void lifecount_exit(void){
	lifecount = 0;
	fpga_print_led();
	fpga_print_text_lcd(TEXT3);
	iounmap(iom_fpga_led_addr);
	iounmap(iom_fpga_text_lcd_addr);
}