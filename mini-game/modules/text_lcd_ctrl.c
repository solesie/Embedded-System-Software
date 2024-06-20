#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/string.h>
#include "text_lcd_ctrl.h"
#include "logging.h"

#define IOM_FPGA_TEXT_LCD_ADDRESS 0x08000090

#define TEXT_LCD_MAX_LINE 16

static unsigned char *iom_fpga_text_lcd_addr;

static void fpga_print_text_lcd(const char* str){
	char text_lcd[TEXT_LCD_MAX_BUFF + 1];
	unsigned short int _s_value;
	int i;
	int len = strlen(str);
	memcpy(text_lcd, str, len);
	
	memset(text_lcd + len, ' ', TEXT_LCD_MAX_BUFF - len);

	for(i = 0; i < TEXT_LCD_MAX_BUFF; i += 2){
		// 16bit
		_s_value = ((text_lcd[i] & 0xFF) << 8) | (text_lcd[i + 1] & 0xFF);
		outw(_s_value,(unsigned int)iom_fpga_text_lcd_addr + i);
	}
}

void text_lcd_init(void){
	// init MMIO
	iom_fpga_text_lcd_addr = ioremap(IOM_FPGA_TEXT_LCD_ADDRESS, 0x32);
}

void text_lcd_set(const char* str){
	fpga_print_text_lcd(str);
}

void text_lcd_exit(void){
	fpga_print_text_lcd("");
	iounmap(iom_fpga_text_lcd_addr);
}