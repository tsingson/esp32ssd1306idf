#include "ssd1306_display.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "oled";
static esp_lcd_panel_handle_t panel_hdl = NULL;
static uint8_t fb[OLED_WIDTH * OLED_HEIGHT / 8] = {0};

// 高压缩比核心字库：包含 [空格, !, H, e, l, o, W, r, d] 基础生成规则，
// 在后续扩展中，你可以直接在此数组添加任意标准 8x16 字符（每字16字节）
static const uint8_t ascii_8x16[][16] = {
    [' '] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['!'] = {0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0xCC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['H'] = {0xE0,0x07,0xE0,0x07,0x00,0x00,0x00,0x00,0xE0,0x07,0xE0,0x07,0x00,0x00,0x00,0x00},
    ['e'] = {0x00,0x03,0xC0,0x05,0x40,0x05,0x40,0x05,0xC0,0x05,0x80,0x02,0x00,0x00,0x00,0x00},
    ['l'] = {0xE0,0x07,0xE0,0x07,0x00,0x04,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['o'] = {0x00,0x03,0x80,0x04,0x80,0x04,0x80,0x04,0x80,0x04,0x00,0x03,0x00,0x00,0x00,0x00},
    ['W'] = {0x00,0x01,0xE0,0x07,0xE0,0x06,0x00,0x01,0xE0,0x06,0xE0,0x07,0x00,0x01,0x00,0x00},
    ['r'] = {0x00,0x07,0x00,0x07,0x00,0x04,0x00,0x04,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00},
    ['d'] = {0x00,0x03,0x80,0x04,0x80,0x04,0x00,0x04,0xE0,0x07,0xE0,0x07,0x00,0x00,0x00,0x00}
};

esp_err_t oled_init(void) {
    i2c_master_bus_handle_t bus_hdl = NULL;
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

void oled_show_string(int x, int y, const char *str) {
    while (*str) {
        if (x + 8 > OLED_WIDTH) break;
        uint8_t c = (uint8_t)*str;
        // 防止未定义字符越界，未包含的字符自动渲染为空格
        if (c >= sizeof(ascii_8x16)/sizeof(ascii_8x16[0]) || (ascii_8x16[c][0] == 0 && c != ' ')) {

            c = ' ';
        }
        for (int col = 0; col < 8; col++) {
            uint8_t b1 = ascii_8x16[c][col * 2];
            uint8_t b2 = ascii_8x16[c][col * 2 + 1];
            int p1 = y / 8, p2 = p1 + 1;
            if (p1 < 8) fb[p1 * OLED_WIDTH + (x + col)] |= b1;
            if (p2 < 8) fb[p2 * OLED_WIDTH + (x + col)] |= b2;
        }
        x += 8;
        str++;
    }
}

void oled_refresh(void) {
    esp_lcd_panel_draw_bitmap(panel_hdl, 0, 0, OLED_WIDTH, OLED_HEIGHT, fb);
}

// ⚠️ 功耗控制 API 接口
void oled_sleep_enter(void) {
    // 1. 让 SSD1306 内置电荷泵进入硬件休眠模式（此时面板彻底断电，电流降至微安级）
    esp_lcd_panel_disp_on_off(panel_hdl, false);
    ESP_LOGI(TAG, "SSD1306 Display Sleeping...");
}

void oled_sleep_exit(void) {
    // 2. 从休眠中重新激活全功能面板
    esp_lcd_panel_disp_on_off(panel_hdl, true);
    // 3. 重新向屏幕恢复刷写休眠前的画面
    oled_refresh();
    ESP_LOGI(TAG, "SSD1306 Display Awakened.");
}
