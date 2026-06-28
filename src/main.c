#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "i2c_bus.h"                /* 提供 i2c_bus_create */

static const char *TAG = "main";

/* 硬件配置 */
#define I2C_SDA        21
#define I2C_SCL        22
#define OLED_ADDRESS   0x3C
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64

/* 用于演示的图片数据（16x16 笑脸，可替换） */
LV_IMG_DECLARE(smile_img);
static const uint8_t smile_map[] = {
    0x00,0x00, 0x0F,0xF0, 0x18,0x18, 0x30,0x0C,
    0x60,0x06, 0x6C,0x36, 0xCC,0x33, 0xCC,0x33,
    0xCC,0x33, 0xCC,0x33, 0x6C,0x36, 0x60,0x06,
    0x30,0x0C, 0x18,0x18, 0x0F,0xF0, 0x00,0x00
};
lv_img_dsc_t smile_img = {
    .header.always_zero = 0,
    .header.w = 16,
    .header.h = 16,
    .data_size = 16 * 16 * LV_IMG_PIXEL_ALPHA_BYTE,
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .data = smile_map,
};

void app_main(void)
{
    ESP_LOGI(TAG, "Start LVGL demo with SSD1306");

    /* 1. 创建 I2C 总线 */
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_bus_handle_t i2c_bus = i2c_bus_create(&i2c_conf);
    assert(i2c_bus != NULL);

    /* 2. 创建 LCD 面板 I/O 句柄 (I2C) */
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = OLED_ADDRESS,
        .scl_speed_hz = 400000,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    /* 3. 创建 SSD1306 面板驱动 */
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_ssd1306_config_t panel_config = {
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    /* 4. 初始化 LVGL 库 */
    const esp_lvgl_port_config_t lvgl_cfg = {
        .task_priority = 2,
        .task_stack = 4096,
        .task_affinity = 0,
        .timer_period_ms = 10,
    };
    ESP_ERROR_CHECK(esp_lvgl_port_init(&lvgl_cfg));

    /* 5. 注册显示驱动到 LVGL */
    const esp_lvgl_port_display_cfg_t disp_cfg = {
        .panel_handle = panel_handle,
        .buffer_size = SCREEN_WIDTH * SCREEN_HEIGHT,   // 单色每像素1字节
        .double_buffer = false,
    };
    esp_lvgl_port_register_display(&disp_cfg);

    /* 6. 启动 LVGL 时钟（必须） */
    esp_lvgl_port_tick_init();

    /* 7. 创建 UI（必须在锁保护下） */
    lv_lock();

    /* ---- 显示文字 ---- */
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello ESP-IDF!");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, -20);

    /* ---- 显示图片 ---- */
    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &smile_img);
    lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 20);

    lv_unlock();

    ESP_LOGI(TAG, "UI created, running...");
}
