#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled_ssd1306.h"

void app_main(void) {
  if (oled_init() != ESP_OK)
    return;

  while (1) {
    // 正常工作
    oled_clear();
    oled_show_string(16, 24, "design by gemini!");
    vTaskDelay(pdMS_TO_TICKS(1000));
    oled_clear();

    // 测试 1：传入超长字符串，它会在右侧边界自动换行
    oled_show_string_wrap(0, 0, "This is an extremely long string that will wrap automatically.");

    // 测试 2：传入包含 \n 的格式化字符串
    oled_show_string_wrap(0, 32, "Line 1\nLine 2\nLine 3!");

    vTaskDelay(pdMS_TO_TICKS(1000));

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
