#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include "esp_err.h"

#define OLED_SDA_PIN      33
#define OLED_SCL_PIN      32
#define OLED_I2C_ADDR     0x3C
#define OLED_WIDTH        128
#define OLED_HEIGHT       64

esp_err_t oled_init(void);
void oled_clear(void);
void oled_show_string(int x, int y, const char *str);
void oled_refresh(void);
void oled_sleep_enter(void);
void oled_sleep_exit(void);

#endif
