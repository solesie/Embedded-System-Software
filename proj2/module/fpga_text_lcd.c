#include <linux/io.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include "fpga_text_lcd.h"
#include "logging.h"

#define IOM_FPGA_TEXT_LCD_ADDRESS 0x08000090 		// pysical address - 32 Byte (16 * 2)
#define TEXT_LCD_MAX_BUFF 32
#define TEXT_LCD_MAX_LINE 16
#define COUNTDOWN 3

static const char* STUDENT_ID = "20171664";
static const char* NAME = "LSY";

static unsigned char *iom_fpga_text_lcd_addr;
static char data[TEXT_LCD_MAX_BUFF + 1];
static int name_loc; // 0 ~ TEXT_LCD_MAX_LINE - strlen(NAME)
static int timer_cnt;
static int countdown = COUNTDOWN;

static void format_data(void){
	char cnt[4];
	int id_len = strlen(STUDENT_ID);
	int cnt_len = snprintf(cnt, sizeof(cnt), "%d", timer_cnt);
	int name_len = strlen(NAME);

	memset(data, 0, sizeof(data));
	strncat(data, STUDENT_ID, id_len);
	memset(data + id_len, ' ', TEXT_LCD_MAX_LINE - id_len - cnt_len);
	strncat(data, cnt, cnt_len);
	
	memset(data + TEXT_LCD_MAX_LINE + name_loc, ' ', name_loc);
	strncat(data, NAME, name_len);
	memset(data + TEXT_LCD_MAX_LINE + name_loc + name_len, ' ', TEXT_LCD_MAX_LINE - name_loc - name_len);
}

static void print_data(void){
	unsigned short int _s_value = 0;
	int i;
	for(i = 0; i < TEXT_LCD_MAX_BUFF; i += 2){
		// 16bit
		_s_value = ((data[i] & 0xFF) << 8) | (data[i + 1] & 0xFF);
		outw(_s_value,(unsigned int)iom_fpga_text_lcd_addr + i);
	}
}

void lcd_init(int _timer_cnt){
	iom_fpga_text_lcd_addr = ioremap(IOM_FPGA_TEXT_LCD_ADDRESS, 0x32);
	
	timer_cnt = _timer_cnt;

	format_data();
	print_data();
}

void lcd_increase(){
	static int dir = 1;
	dir ? ++name_loc : --name_loc;
	if(name_loc > TEXT_LCD_MAX_LINE - strlen(NAME)){
		name_loc -= 2;
		dir = 0;
	}
	if(name_loc < 0){
		name_loc += 2;
		dir = 1;
	}
	--timer_cnt;
	format_data();
	print_data();
}

void lcd_countdown(void){
	char c = countdown + '0';
	memset(data, 0, sizeof(data));
	sprintf(data, "Time's up!     0Shutdown in %c...", c);
	print_data();
	--countdown;
}

void lcd_del(void){
	memset(data, 0, sizeof(data));
	memset(data, ' ', TEXT_LCD_MAX_BUFF);
	print_data();
	iounmap(iom_fpga_text_lcd_addr);
}
