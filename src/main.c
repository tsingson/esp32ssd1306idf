#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "oled_ssd1306.h"

static const char *TAG = "main";

#define WAKEUP_BUTTON_GPIO    4

static QueueHandle_t data_queue = NULL;
static SemaphoreHandle_t display_done_sem = NULL;
static TaskHandle_t ssd_task_handle = NULL;

static void configure_wakeup_button(void) {
    gpio_config_t btn_config = {
        .pin_bit_mask = (1ULL << WAKEUP_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&btn_config));
    ESP_ERROR_CHECK(gpio_wakeup_enable(WAKEUP_BUTTON_GPIO, GPIO_INTR_LOW_LEVEL));
}

/**
 * @brief 🌟 彻底修复后的 SSD 屏幕数据处理与渲染任务
 */
// void ssd_task(void *pvParameters) {
//     int received_val = 0;
//     // 🌟 修复 1：明确定义包含 10 个物理空间的整型本地缓冲区数组
//     int data_buffer[10] = {0};
//     // 🌟 修复 2：将单字符变量修正为 32 字节大小的合法字符数组，彻底封堵内存溃决漏洞
//     char display_str[32] = {0};
//
//     while (1) {
//         // ==========================================================
//         // 阶段一：进度条加载阶段 (只缓存数据，数字暂不登场)
//         // ==========================================================
//         for (int i = 1; i <= 10; i++) {
//             if (xQueueReceive(data_queue, &received_val, portMAX_DELAY) == pdTRUE) {
//                 // 安全存入本地临时数组
//                 data_buffer[i - 1] = received_val;
//                 ESP_LOGI(TAG, "Queue buffered [%d]: %d", i - 1, received_val);
//
//                 oled_clear();
//                 oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);
//                 oled_show_string_ex(0, 20, "Buffering data...", 0);
//
//                 // 进度条顺次递增渲染
//                 oled_draw_progress_bar(0, 44, 128, 12, i, 10);
//                 oled_refresh();
//
//                 vTaskDelay(pdMS_TO_TICKS(200));
//             }
//         }
//
//         // 🌟 物理转场：进度条拉满瞬间，触发全屏反色轰炸特效（持续 60 毫秒）
//         oled_flash_screen(1, 60);
//
//         // ==========================================================
//         // 阶段二：数字轮播显示阶段 (进度条彻底隐藏，数字伴随强烈仿生震动登场)
//         // ==========================================================
//         for (int j = 0; j < 10; j++) {
//             // 🌟 此时 display_str 作为合法指针传入，snprintf 执行绝对安全
//             snprintf(display_str, sizeof(display_str), "queue rcv: %d", data_buffer[j]);
//             ESP_LOGI(TAG, "OLED Print: %s", display_str);
//
//             oled_clear();
//             oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);
//             // 进度条隐藏后，文字完美居中呈现
//             oled_show_string_ex(0, 28, display_str, 0);
//             oled_refresh();
//
//             // 🌟 仿生动效：数字蹦出的刹那，屏幕物理剧烈抖动 120ms
//             oled_shake_screen(4, 120);
//
//             // 精准时间对齐：扣除震动消耗的 120ms，静止维持 280ms，凑满 400ms 最佳阅读帧率
//             vTaskDelay(pdMS_TO_TICKS(280));
//         }
//
//         // ==========================================================
//         // 结束收尾：保持最终画面 2 秒后，同步释放信号量并挂起
//         // ==========================================================
//         vTaskDelay(pdMS_TO_TICKS(2000));
//
//         oled_clear();
//         oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);
//         oled_show_string_ex(0, 28, "Batch Complete!", 0);
//         oled_refresh();
//
//         // 释放信号，放行 main 线程去配置休眠寄存器
//         xSemaphoreGive(display_done_sem);
//
//         // 🌟 自我挂起：完美交出时钟控制权，不消耗 1 滴电量，等待下个大周期被唤醒
//         ESP_LOGI(TAG, "SSD Task layout sequence done. Suspending...");
//         vTaskSuspend(NULL);
//     }
// }

/**
 * @brief SSD 屏幕数据处理与渲染任务
 * 升级后逻辑：进度条满 -> 闪烁转场 -> 🌟切换至 16x16 巨型粗体 ＋ 伴随强震动登场
 */
void ssd_task(void *pvParameters) {
    int received_val = 0;
    int data_buffer[10] = {0};
    char display_str[32] = {0};

    while (1) {
        // ==========================================================
        // 阶段一：进度条加载阶段 (使用原有 8x8 字体提示)
        // ==========================================================
        for (int i = 1; i <= 10; i++) {
            if (xQueueReceive(data_queue, &received_val, portMAX_DELAY) == pdTRUE) {
                data_buffer[i - 1] = received_val;
                ESP_LOGI(TAG, "Queue buffered [%d]: %d", i - 1, received_val);

                oled_clear();
                oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);
                oled_show_string_ex(0, 20, "Buffering data...", 0);
                oled_draw_progress_bar(0, 44, 128, 12, i, 10);
                oled_refresh();

                vTaskDelay(pdMS_TO_TICKS(200));
            }
        }

        // 转场特效：闪烁一下
        oled_flash_screen(1, 60);

        // ==========================================================
        // 阶段二：数字轮播显示阶段 (🌟 隐形进度条，升级为 16x16 轰炸粗体)
        // ==========================================================
        for (int j = 0; j < 10; j++) {
            // 由于 16x16 字体较大，简练格式化输出内容，确保不超过屏幕 128 像素宽
            snprintf(display_str, sizeof(display_str), "RCV: %d", data_buffer[j]);
            ESP_LOGI(TAG, "OLED Print Big Bold: %s", display_str);

            oled_clear();
            oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);

            // 🌟 调用全新算法函数：从 y=24 坐标开始，渲染霸气的 16x16 粗体数字
            oled_show_string_16x16_bold(24, 24, display_str);
            oled_refresh();

            // 🌟 核心动效组合：巨型粗体数字蹦出的瞬间，配合 4 像素强震动 120 毫秒
            // 大字形加上硬件物理抖动，视觉张力和极客感极其强烈！
            oled_shake_screen(4, 120);

            vTaskDelay(pdMS_TO_TICKS(280)); // 维持显示，凑满 400ms
        }

        // ==========================================================
        // 结束收尾：2秒延迟后进入全局休眠
        // ==========================================================
        vTaskDelay(pdMS_TO_TICKS(2000));

        oled_clear();
        oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);
        // 完成提示也同步使用 16x16 粗体在中央醒目放大显示
        oled_show_string_16x16_bold(8, 24, "SUCCESS!");
        oled_refresh();

        xSemaphoreGive(display_done_sem);

        ESP_LOGI(TAG, "SSD Task layout sequence done. Suspending...");
        vTaskSuspend(NULL);
    }
}

void app_main(void) {
    if (oled_init() != ESP_OK) {
        ESP_LOGE(TAG, "OLED Core Engine Init Failed!");
        return;
    }

    configure_wakeup_button();

    data_queue = xQueueCreate(10, sizeof(int));
    display_done_sem = xSemaphoreCreateBinary();

    if (data_queue == NULL || display_done_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create OS primitives!");
        return;
    }

    xTaskCreate(ssd_task, "ssd_task", 3072, NULL, 5, &ssd_task_handle);

    while (1) {
        ESP_LOGI(TAG, "Starting new processing cycle...");

        for (int val = 1; val <= 10; val++) {
            xQueueSend(data_queue, &val, portMAX_DELAY);
        }

        xSemaphoreTake(display_done_sem, portMAX_DELAY);

        ESP_LOGI(TAG, "Putting OLED to hardware sleep mode...");
        oled_sleep_enter();

        uint64_t sleep_time_us = 30ULL * 1000ULL * 1000ULL;
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(sleep_time_us));
        ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());

        ESP_LOGI(TAG, "ESP32 Entering Light Sleep. Wait 30s OR press Button on GPIO 4...");

        // 🌟 完美兼容你环境中的单参数 5.3.1 串口物理冲刷标准
        uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);

        esp_light_sleep_start();

        // 判定唤醒源
        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
        if (cause == ESP_SLEEP_WAKEUP_GPIO) {
            ESP_LOGI(TAG, "Woke up by EXTERNAL BUTTON!");
        } else {
            ESP_LOGI(TAG, "Woke up by TIMER!");
        }

        oled_sleep_exit();

        ESP_LOGI(TAG, "Resuming SSD Task...");
        vTaskResume(ssd_task_handle);
    }
}
