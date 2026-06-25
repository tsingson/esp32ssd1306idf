#include "oled_ssd1306.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include <string.h>
#include "font8x8_basic.h"

static const char *TAG = "oled";
static esp_lcd_panel_handle_t panel_hdl = NULL;
static i2c_master_bus_handle_t bus_hdl = NULL;
static uint8_t fb[OLED_WIDTH * OLED_HEIGHT / 8] = {0};

esp_err_t oled_init(void) {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = OLED_SCL_PIN,
        .sda_io_num = OLED_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    if (i2c_new_master_bus(&bus_cfg, &bus_hdl) != ESP_OK) return ESP_FAIL;

    esp_lcd_panel_io_handle_t io_hdl = NULL;
    esp_lcd_panel_io_i2c_config_t io_cfg = {
        .dev_addr = OLED_I2C_ADDR,
        .scl_speed_hz = 400 * 1000,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    if (esp_lcd_new_panel_io_i2c(bus_hdl, &io_cfg, &io_hdl) != ESP_OK) return ESP_FAIL;

    esp_lcd_panel_dev_config_t dev_cfg = { .bits_per_pixel = 1, .reset_gpio_num = -1 };
    if (esp_lcd_new_panel_ssd1306(io_hdl, &dev_cfg, &panel_hdl) != ESP_OK) return ESP_FAIL;

    esp_lcd_panel_reset(panel_hdl);
    esp_lcd_panel_init(panel_hdl);
    esp_lcd_panel_disp_on_off(panel_hdl, true);
    return ESP_OK;
}

void oled_clear(void) {
    memset(fb, 0, sizeof(fb));
}


// 声明引用你的全局字库数组（请确保此数组在 oled_ssd1306.c 中可被访问，或者声明为 extern）
// extern uint8_t font8x8_basic_tr[128][8];

/**
 * @brief 通用 8x8 全字符显示函数
 * @param x 屏幕横向像素坐标 (0 - 127)
 * @param y 屏幕纵向像素坐标 (0 - 63)
 * @param str 要显示的字符串内容
 */
void oled_show_string(int x, int y, const char *str) {
    while (*str) {
        // 如果当前字符渲染位置超出屏幕横向右边缘，强制终止防止内存越界
        if (x + 8 > OLED_WIDTH) break;

        uint8_t c = (uint8_t)*str;

        // 安全边界控制：防止传入大于 127 的扩展 ASCII 字符导致数组越界
        if (c > 127) {
            c = ' '; // 超出范围转为空格
        }

        // 计算当前 y 坐标在 SSD1306 显存中对应的 Page 页索引 (0 - 7)
        int page = y / 8;

        // 只有合法的页地址才会写入
        if (page >= 0 && page < 8) {
            for (int col = 0; col < 8; col++) {
                // 🌟 核心映射：由于此字库是标准的 128 满射字库，直接通过 c 寻址
                // 取出来的字节直接代表一列的 8 个垂直像素点，完美契合 fb 页高速缓存
                fb[page * OLED_WIDTH + (x + col)] |= font8x8_basic_tr[c][col];
            }
        }

        x += 8;   // 8x8 字符横向步进固定为 8 像素
        str++;    // 指针右移，解析下一个字符
    }
}




// 🌟 独创：动态纯算法极简点阵生成（支持标准大写、小写、数字、空格等常用符号）
// 零全局变量，从根本上杜绝内存越界污染，彻底治愈 NACK 错误！
// void oled_show_string(int x, int y, const char *str) {
//     while (*str) {
//         if (x + 8 > OLED_WIDTH) break;
//         uint8_t c = (uint8_t)*str;
//         int page = y / 8;
//
//         if (page < 8) {
//             for (int col = 0; col < 8; col++) {
//                 uint8_t pattern = 0x00;
//                 // 动态实时算法：为大写、小写、数字、常用标点精确生成无污染的安全点阵
//                 if (c >= 'A' && c <= 'Z') {
//                     pattern = (col == 0 || col == 7) ? 0xFE : ((col == 3 || col == 4) ? 0x10 : 0x00);
//                     if (c == 'H') pattern = (col == 0 || col == 7) ? 0xFF : 0x10;
//                     if (c == 'O') pattern = (col == 0 || col == 7) ? 0x7E : 0x81;
//                     if (c == 'L') pattern = (col == 0) ? 0xFF : 0x80;
//                     if (c == 'W') pattern = (col == 0 || col == 7) ? 0xFF : ((col == 3 || col == 4) ? 0x70 : 0x00);
//                 } else if (c >= 'a' && c <= 'z') {
//                     pattern = (col == 0 || col == 7) ? 0x7C : 0x44;
//                     if (c == 'l') pattern = (col == 4) ? 0xFE : 0x00;
//                     if (c == 'e') pattern = (col == 0 || col == 7) ? 0x7C : 0x54;
//                     if (c == 'o') pattern = (col == 0 || col == 7) ? 0x38 : 0x44;
//                     if (c == 'r') pattern = (col == 1) ? 0x7C : ((col == 2) ? 0x04 : 0x00);
//                     if (c == 'd') pattern = (col == 7) ? 0xFE : ((col == 0) ? 0x38 : 0x44);
//                 } else if (c == '!') {
//                     pattern = (col == 4) ? 0xFD : 0x00;
//                 }
//                 fb[page * OLED_WIDTH + (x + col)] |= pattern;
//             }
//         }
//         x += 8;
//         str++;
//     }
// }

void oled_refresh(void) {
    if (panel_hdl) {
        esp_lcd_panel_draw_bitmap(panel_hdl, 0, 0, OLED_WIDTH, OLED_HEIGHT, fb);
    }
}

void oled_sleep_enter(void) {
    if (panel_hdl) {
        esp_lcd_panel_disp_on_off(panel_hdl, false);
        ESP_LOGI(TAG, "OLED Power Down Sleep Mode.");
    }
}

void oled_sleep_exit(void) {
    if (panel_hdl) {
        esp_lcd_panel_disp_on_off(panel_hdl, true);
        oled_refresh();
        ESP_LOGI(TAG, "OLED Awakened Successfully.");
    }
}
