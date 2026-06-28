#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

static const char *TAG = "main";

#define I2C_HOST           0
#define PIN_I2C_SDA        21
#define PIN_I2C_SCL        22
#define LCD_PIXEL_CLOCK_HZ (400 * 1000)
#define LCD_H_RES          128
#define LCD_V_RES          64

// SSD1306 的 I2C 地址通常为 0x3C
#define ESP_LCD_TOUCH_I2C_ADDRESS 0x3C

// 声明外部图片数据 (由 LVGL 图片转换工具生成)
LV_IMG_DECLARE(my_img_dsc);

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // 将 LVGL 缓冲区数据拷贝到 OLED 屏幕
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void increase_lvgl_tick(void *arg) {
    /* 告诉 LVGL 过去了多少毫秒 */
    lv_tick_inc(2);
}

void app_main(void) {
    ESP_LOGI(TAG, "Initializing I2C Bus...");
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = LCD_PIXEL_CLOCK_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, conf.mode, 0, 0, 0));

    ESP_LOGI(TAG, "Installing panel IO...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = ESP_LCD_TOUCH_I2C_ADDRESS,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .on_color_trans_done = notify_lvgl_flush_ready,
    };

    // 创建 SSD1306 Panel IO 句柄
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Installing SSD1306 driver...");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1, // 单色屏
        .reset_gpio_num = -1, // 如果有复位引脚填对应数字，没有填 -1
    };

    // 使用官方标准的 SSD1306 驱动初始化
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Initializing LVGL library...");
    lv_init();

    // 分配 LVGL 显存缓冲区 (单色屏建议分配全屏大小)
    lv_color_t *buf1 = malloc(LCD_H_RES * LCD_V_RES * sizeof(lv_color_t));
    assert(buf1 != NULL);

    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, LCD_H_RES * LCD_V_RES);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Registering LVGL tick timer...");
    // 建立 2ms 的定时器为 LVGL 提供时钟步进
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 2000)); // 2000us = 2ms

    ESP_LOGI(TAG, "Creating UI Elements...");

    /* 1. 显示一行文字 */
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello ESP32 & LVGL");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);

    /* 2. 显示一张图片 */
    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &my_img_dsc);
    lv_obj_align(img, LV_ALIGN_BOTTOM_MID, 0, -5);

    ESP_LOGI(TAG, "Running LVGL loop...");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler(); // 处理 LVGL 任务
    }
}