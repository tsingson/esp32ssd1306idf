#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled_ssd1306.h"

void app_main(void) {
    if (oled_init() != ESP_OK) return;

    while (1) {
        // 正常工作
        oled_clear();
        oled_show_string(16, 24, "Hello World!");
        oled_refresh();
        vTaskDelay(pdMS_TO_TICKS(4000));

        // 模拟休眠
        oled_sleep_enter();
        vTaskDelay(pdMS_TO_TICKS(4000));

        // 模拟唤醒
        oled_sleep_exit();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
