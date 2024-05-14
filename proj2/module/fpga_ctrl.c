/*
 * fpga controller
 */
#include <linux/io.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include "fpga_ctrl.h"
#include "logging.h"

#define DOT_TABLE_MAX 8
#define LED_MAX 8
#define TEXT_LCD_MAX_BUFF 32
#define TEXT_LCD_MAX_LINE 16

#define IOM_FPGA_DIP_SWITCH_ADDRESS 0x08000000
#define IOM_FPGA_DOT_ADDRESS 0x08000210
#define IOM_FND_ADDRESS 0x08000004
#define IOM_LED_ADDRESS 0x08000016
#define IOM_FPGA_TEXT_LCD_ADDRESS 0x08000090

static const unsigned char dot_table[DOT_TABLE_MAX + 2][10] = {
	{0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
	{0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06}, // 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
	{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
	{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
	{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
	{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
	{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8

    // memset(array,0x00,sizeof(array));
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // blank
};
static const char* STUDENT_ID = "20171664";
static const char* NAME = "LSY";

static unsigned char *iom_fpga_dip_switch_addr;
static unsigned char *iom_fpga_dot_addr;
static unsigned char *iom_fpga_fnd_addr;
static unsigned char *iom_fpga_led_addr;
static unsigned char *iom_fpga_text_lcd_addr;

/* represent current fpga status */
static int dot;
static unsigned char fnd[FND_MAX];
static unsigned char pivot;
static int led;
static char text_lcd[TEXT_LCD_MAX_BUFF + 1];
static int name_loc;
static int timer_cnt;
static int countdown;
static int dir = 1;

static void format_text_lcd(void){
	char cnt[4] = {0, };
	int id_len = strlen(STUDENT_ID);
	int cnt_len = snprintf(cnt, sizeof(cnt), "%d", timer_cnt);
	int name_len = strlen(NAME);

	memset(text_lcd, 0, sizeof(text_lcd));
	strncat(text_lcd, STUDENT_ID, id_len);
	memset(text_lcd + id_len, ' ', TEXT_LCD_MAX_LINE - id_len - cnt_len);
	strncat(text_lcd, cnt, cnt_len);
	
	memset(text_lcd + TEXT_LCD_MAX_LINE, ' ', name_loc);
	text_lcd[TEXT_LCD_MAX_LINE + name_loc] = 0;
	strncat(text_lcd, NAME, name_len);
	memset(text_lcd + TEXT_LCD_MAX_LINE + name_loc + name_len, ' ', TEXT_LCD_MAX_LINE - name_loc - name_len);
}

static void print(void){
	int i;
	unsigned short int fnd_short;
	unsigned short value;
	unsigned short int _s_value;

    // print dot
	for(i = 0; i < 10; i++)
		outw(dot_table[dot][i] & 0x7F, (unsigned int) iom_fpga_dot_addr + i * 2);

    // print fnd
    fnd_short = fnd[0] << 12 | fnd[1] << 8 | fnd[2] << 4 | fnd[3];
	outw(fnd_short,(unsigned int)iom_fpga_fnd_addr);

    // print led
    value = led > LED_MAX ? 0 : 1 << (LED_MAX- led);
    outw(value, (unsigned int) iom_fpga_led_addr);

    // print text lcd
    _s_value = 0;
	for(i = 0; i < TEXT_LCD_MAX_BUFF; i += 2){
		// 16bit
		_s_value = ((text_lcd[i] & 0xFF) << 8) | (text_lcd[i + 1] & 0xFF);
		outw(_s_value,(unsigned int)iom_fpga_text_lcd_addr + i);
	}
}

/* Let's ready to run timer */
void fpga_on(int _timer_cnt, int _countdown, char _timer_init[FND_MAX + 1]){
    // init dot
    int i;
    for(i = 0; i < FND_MAX; ++i) if(_timer_init[i] != '0') dot = _timer_init[i] - '0';

    // init fnd
    for(i = 0; i < FND_MAX; ++i) {
		if(_timer_init[i] != '0') pivot = _timer_init[i] - '0';
		fnd[i] = _timer_init[i] - '0';
	}

    // init led
    for(i = 0; i < FND_MAX; ++i) if(_timer_init[i] != '0') led = _timer_init[i] - '0';

    // init text lcd
    timer_cnt = _timer_cnt;
	countdown = _countdown;
    format_text_lcd();

    print();
}

/* Timer increase callback */
void fpga_increase(void){
    int i;

    // increase dot
    ++dot;
	if(dot > DOT_TABLE_MAX) dot = 1;

    // increase fnd
    for(i = 0; i < FND_MAX; ++i){
		if(fnd[i] != 0){
			++fnd[i];
			if(fnd[i] > DOT_TABLE_MAX) fnd[i] = 1;
			if(fnd[i] == pivot){
				memset(fnd, 0, sizeof(fnd));
				i = (i + 1) % FND_MAX;
				fnd[i] = pivot;
			}
			break;
		}
	}

    // increase led
    ++led;
	if(led > LED_MAX) led = 1;

    // increase text lcd
	dir ? ++name_loc : --name_loc;
	if(name_loc > TEXT_LCD_MAX_LINE - (int)strlen(NAME)){
		name_loc -= 2;
		dir = 0;
	}
	if(name_loc < 0){
		name_loc += 2;
		dir = 1;
	}
	--timer_cnt;
    format_text_lcd();

    print();
}

/* Turn off fpga devices. */
void fpga_off(void){
    dot = DOT_TABLE_MAX + 1;
    memset(fnd, 0, sizeof(fnd));
    led = LED_MAX + 1;
    memset(text_lcd, 0, sizeof(text_lcd));
	memset(text_lcd, ' ', TEXT_LCD_MAX_BUFF);
	
	pivot = 0;
	name_loc = 0;
	timer_cnt = 0;
	countdown = 0;
	dir = 1;

    print();
}

/* Let's ready to countdown */
void fpga_set_countdown(void){
	dot = DOT_TABLE_MAX + 1;
    memset(fnd, 0, sizeof(fnd));
    led = LED_MAX + 1;

    memset(text_lcd, 0, sizeof(text_lcd));
	sprintf(text_lcd, "Time's up!     0Shutdown in %d...", countdown);

	print();
}

/* Timer countdown callback */
void fpga_countdown(void){
    memset(text_lcd, 0, sizeof(text_lcd));
	sprintf(text_lcd, "Time's up!     0Shutdown in %d...", --countdown);

    print();
}

/* Interrupt context not safe */
void fpga_ioremap(void){
	iom_fpga_dip_switch_addr = ioremap(IOM_FPGA_DIP_SWITCH_ADDRESS, 0x1);
    iom_fpga_dot_addr = ioremap(IOM_FPGA_DOT_ADDRESS, 0x10);
    iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
    iom_fpga_led_addr = ioremap(IOM_LED_ADDRESS, 0x1);
    iom_fpga_text_lcd_addr = ioremap(IOM_FPGA_TEXT_LCD_ADDRESS, 0x32);
}

/* Interrupt context not safe */
void fpga_iounmap(void){
	iounmap(iom_fpga_dip_switch_addr);
    iounmap(iom_fpga_dot_addr);
    iounmap(iom_fpga_fnd_addr);
    iounmap(iom_fpga_led_addr);
    iounmap(iom_fpga_text_lcd_addr);
}

/* Wait until the RESET button is pressed and released.
 * Interrupt context not safe */
void fpga_dip_switch_read_sync(void){
	unsigned char dip_sw_value;	
	unsigned short _s_dip_sw_value;

	while(1){
		usleep_range(1000, 1010);
		_s_dip_sw_value = inw((unsigned int)iom_fpga_dip_switch_addr);
		dip_sw_value = _s_dip_sw_value & 0xFF;
		if(dip_sw_value != 0) continue;

		while(1){
			usleep_range(1000, 1010);
			_s_dip_sw_value = inw((unsigned int)iom_fpga_dip_switch_addr);
			dip_sw_value = _s_dip_sw_value & 0xFF;
			if(dip_sw_value == 0) continue;
			return;
		}
	}
}
