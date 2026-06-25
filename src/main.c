#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/i2c_master.h" // 更改为新版 I2C 主机驱动
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "main";

// 物理接线管脚设定
#define PIN_NUM_SDA           33  // 确保屏幕 SDA 接入 GPIO 21
#define PIN_NUM_SCL           32  // 确保屏幕 SCL 接入 GPIO 22
#define PIN_NUM_RST           -1  // 无 Reset 引脚填 -1

#define LCD_PIXEL_CLOCK_HZ    (400 * 1000) // 400kHz
#define LCD_H_RES             128
#define LCD_V_RES             64
#define SSD1306_I2C_ADDRESS   0x3C

// 基础 8x16 字符点阵
const uint8_t font_8x16_H[] = {0x00,0x00,0xE0,0x07,0xE0,0x07,0x00,0x00,0x00,0x00,0xE0,0x07,0xE0,0x07,0x00,0x00};
const uint8_t font_8x16_e[] = {0x00,0x00,0x00,0x03,0xC0,0x05,0x40,0x05,0x40,0x05,0xC0,0x05,0x80,0x02,0x00,0x00};
const uint8_t font_8x16_l[] = {0x00,0x00,0xE0,0x07,0xE0,0x07,0x00,0x04,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00};
const uint8_t font_8x16_o[] = {0x00,0x00,0x00,0x03,0x80,0x04,0x80,0x04,0x80,0x04,0x80,0x04,0x00,0x03,0x00,0x00};
const uint8_t font_8x16_W[] = {0x00,0x01,0xE0,0x07,0xE0,0x06,0x00,0x01,0x00,0x01,0xE0,0x06,0xE0,0x07,0x00,0x01};
const uint8_t font_8x16_r[] = {0x00,0x00,0x00,0x07,0x00,0x07,0x00,0x04,0x00,0x04,0x00,0x04,0x00,0x00,0x00,0x00};
const uint8_t font_8x16_d[] = {0x00,0x00,0x00,0x03,0x80,0x04,0x80,0x04,0x00,0x04,0xE0,0x07,0xE0,0x07,0x00,0x00};
const uint8_t font_8x16_sp[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

static uint8_t screen_buffer[LCD_H_RES * LCD_V_RES / 8] = {0};

void draw_char_8x16(int x, int y, const uint8_t *font_glyph) {
    for (int col = 0; col < 8; col++) {
        if ((x + col) >= LCD_H_RES) break;
        uint8_t byte1 = font_glyph[col * 2];
        uint8_t byte2 = font_glyph[col * 2 + 1];
        int page1 = y / 8;
        int page2 = page1 + 1;
        if (page1 < 8) screen_buffer[page1 * LCD_H_RES + (x + col)] |= byte1;
        if (page2 < 8) screen_buffer[page2 * LCD_H_RES + (x + col)] |= byte2;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing New I2C Bus Master...");
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1, // 自动选择空闲端口
        .scl_io_num = PIN_NUM_SCL,
        .sda_io_num = PIN_NUM_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, // 启用内部弱上拉作双重保险
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    ESP_LOGI(TAG, "Installing panel IO via New I2C driver...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = SSD1306_I2C_ADDRESS,
        .scl_speed_hz = LCD_PIXEL_CLOCK_HZ,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    // 使用新版 i2c_bus 句柄创建底层 IO
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    ESP_LOGI(TAG, "Installing SSD1306 driver panel...");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = PIN_NUM_RST,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Clearing buffer and drawing 'Hello World'...");
    draw_char_8x16(20, 24, font_8x16_H);
    draw_char_8x16(28, 24, font_8x16_e);
    draw_char_8x16(36, 24, font_8x16_l);
    draw_char_8x16(44, 24, font_8x16_l);
    draw_char_8x16(52, 24, font_8x16_o);
    draw_char_8x16(60, 24, font_8x16_sp);
    draw_char_8x16(68, 24, font_8x16_W);
    draw_char_8x16(76, 24, font_8x16_o);
    draw_char_8x16(84, 24, font_8x16_r);
    draw_char_8x16(92, 24, font_8x16_l);
    draw_char_8x16(100, 24, font_8x16_d);

    ESP_LOGI(TAG, "Refreshing screen display data...");
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, screen_buffer));

    ESP_LOGI(TAG, "Done. Loop waiting.");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
