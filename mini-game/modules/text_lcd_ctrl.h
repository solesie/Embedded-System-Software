#ifndef TEXT_LCD_CTRL_H
#define TEXT_LCD_CTRL_H

#define TEXT_LCD_MAX_BUFF 32

void text_lcd_init(void);
void text_lcd_set(const char* str);
void text_lcd_exit(void);

#endif // TEXT_LCD_CTRL_H