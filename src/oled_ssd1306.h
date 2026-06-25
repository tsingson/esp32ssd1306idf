#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include "esp_err.h"

#define OLED_SDA_PIN 33
#define OLED_SCL_PIN 32
#define OLED_I2C_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

esp_err_t oled_init(void);
void oled_clear(void);
void oled_show_string(int x, int y, const char *str);
void oled_refresh(void);
void oled_sleep_enter(void);
void oled_sleep_exit(void);

// 🌟 追加这一行高级换行接口声明
void oled_show_string_wrap(int start_x, int start_y, const char *str);
// 追加图形接口声明
void oled_draw_pixel(int x, int y, uint8_t color);
void oled_draw_rectangle(int x, int y, int width, int height);




// 追加实心矩形填充接口声明
void oled_fill_rectangle(int x, int y, int width, int height);

// 升级后的字符串显示接口声明，带反色控制
void oled_show_string_ex(int start_x, int start_y, const char *str, uint8_t invert);


#endif
