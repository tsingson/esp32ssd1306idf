#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "driver/gpio.h"    // 🌟 引入 GPIO 驱动，用于配置按键管脚
#include "esp_sleep.h"      // 引入 SoC 休眠控制库
#include "esp_log.h"
#include "oled_ssd1306.h"

static const char *TAG = "main";

// 硬件唤醒按键管脚定义（推荐使用 GPIO 4，外接一个按键到地 GND）
#define WAKEUP_BUTTON_GPIO   0// boot // 4

// FreeRTOS 句柄
static QueueHandle_t data_queue = NULL;
static SemaphoreHandle_t display_done_sem = NULL;
static TaskHandle_t ssd_task_handle = NULL;

/**
 * @brief 配置唤醒按键的 GPIO 硬件属性
 */
static void configure_wakeup_button(void) {
    gpio_config_t btn_config = {
        .pin_bit_mask = (1ULL << WAKEUP_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,      // 启用内部上拉电阻，默认高电平
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE          // 休眠唤醒不需要开启普通 GPIO 中断
    };
    ESP_ERROR_CHECK(gpio_config(&btn_config));

    // 🌟 极为关键的一步：使能该 GPIO 的低功耗微安级硬件唤醒功能
    ESP_ERROR_CHECK(gpio_wakeup_enable(WAKEUP_BUTTON_GPIO, GPIO_INTR_LOW_LEVEL));
}


 /**
 * @brief SSD 屏幕数据处理与渲染任务
 * 调整后逻辑：进度条先加载至 100%，加载结束时彻底隐藏进度条，之后上方再顺次轮播显示 1 到 10 的数字
 */
void ssd_task(void *pvParameters) {
    int received_val = 0;
    int data_buffer[10]; // 本地临时缓冲区，用于暂存从队列取出的 10 个数字
    char display_str[32];

    while (1) {
        // ==========================================================
        // 阶段一：进度条加载阶段 (此时数字还未显示)
        // ==========================================================
        // 从队列中顺次取出 10 个数，每取出一个数，进度条递增 10%
        for (int i = 1; i <= 10; i++) {
            if (xQueueReceive(data_queue, &received_val, portMAX_DELAY) == pdTRUE) {
                // 将取出的数据暂时存入本地数组，留到阶段二使用
                data_buffer[i - 1] = received_val;
                ESP_LOGI(TAG, "Queue buffering data index %d: %d", i, received_val);

                oled_clear();
                // 顶部固定标题
                oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);
                // 提示当前处于加载接收阶段
                oled_show_string_ex(0, 20, "Buffering data...", 0);

                // 🌟 绘制进度条：此时进度条顺次递增
                oled_draw_progress_bar(0, 44, 128, 12, i, 10);

                oled_refresh();

                // 进度条每走一格，留出 200ms 的顺滑加载动画时间
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        }

        // ==========================================================
        // 阶段二：数字轮播显示阶段 (🌟 此时进度条已完全隐藏)
        // ==========================================================
        // 进度条圆满完成任务，在此阶段不再调用 oled_draw_progress_bar，使其在屏幕上彻底隐形
        for (int j = 0; j < 10; j++) {
            // 从刚才的本地缓冲区读出数字进行格式化
            snprintf(display_str, sizeof(display_str), "queue rcv: %d", data_buffer[j]);
            ESP_LOGI(TAG, "OLED Sequential Print: %d", data_buffer[j]);

            oled_clear();
            oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);

            // 🌟 在干净的屏幕中央依次显示 1 到 10，下方没有任何进度条干扰
            oled_show_string_ex(0, 28, display_str, 0);

            oled_refresh();

            // 每个数字在屏幕上强制停留 400ms 以便人类阅读
            vTaskDelay(pdMS_TO_TICKS(400));
        }

        // ==========================================================
        // 结束收尾：全部完成后延时 2 秒，随后准备休眠
        // ==========================================================
        ESP_LOGI(TAG, "All sequences finished. Holding final view for 2s...");
        vTaskDelay(pdMS_TO_TICKS(2000));

        oled_clear();
        oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);
        oled_show_string_ex(0, 28, "Batch Complete!", 0); // 居中显示完成提示
        oled_refresh();

        // 释放二值信号量，通知 main 线程我们干完活了
        xSemaphoreGive(display_done_sem);

        // 挂起自身任务，彻底交出 CPU 控制权，静等 main 下个周期唤醒
        ESP_LOGI(TAG, "SSD Task entering suspension state...");
        vTaskSuspend(NULL);
    }
}




void app_main(void) {
    // 1. 初始化稳固的模块化 OLED 引擎
    if (oled_init() != ESP_OK) {
        ESP_LOGE(TAG, "OLED Core Engine Init Failed!");
        return;
    }

    // 2. 初始化配置我们的外部唤醒物理按键
    configure_wakeup_button();

    // 3. 构建 OS 级同步原语
    data_queue = xQueueCreate(10, sizeof(int));
    display_done_sem = xSemaphoreCreateBinary();

    if (data_queue == NULL || display_done_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create OS primitives!");
        return;
    }

    xTaskCreate(ssd_task, "ssd_task", 3072, NULL, 5, &ssd_task_handle);

    while (1) {
        ESP_LOGI(TAG, "===============================");
        ESP_LOGI(TAG, "Starting new processing cycle...");
        ESP_LOGI(TAG, "===============================");

        // 顺次写入 1 到 10
        for (int val = 1; val <= 10; val++) {
            xQueueSend(data_queue, &val, portMAX_DELAY);
        }

        // 等待显示完成信号
        xSemaphoreTake(display_done_sem, portMAX_DELAY);

        // 关闭屏幕内置电荷泵，使其彻底断电进入微安级深度休眠
        ESP_LOGI(TAG, "Signal received. Putting OLED to hardware sleep mode...");
        oled_sleep_enter();

        // 🌟 唤醒源配置链路 1：开启 30 秒的 RTC 定时唤醒 (30,000,000 微秒)
        uint64_t sleep_time_us = 30ULL * 1000ULL * 1000ULL;
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(sleep_time_us));

        // 🌟 唤醒源配置链路 2：开启特定的 GPIO 外部硬件唤醒逻辑功能
        ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());

        ESP_LOGI(TAG, "ESP32 Entering Light Sleep. Wait 30s OR press Button on GPIO 4...");

        // 等待串口缓冲区清空（完美适配你的单参数 v5.3.1 固件原生标准）
        uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);

        // 🌟 执行浅度休眠。此时定时器开始倒计时，同时硬件不断侦测 GPIO 4。
        // 一旦按键按下（低电平），或者 30 秒时间到，芯片立刻在此处复苏恢复时钟。
        esp_light_sleep_start();

        // 判定唤醒原因（非必须，但方便你调试打印查看是定时器叫醒的还是按键叫醒的）
        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
        if (cause == ESP_SLEEP_WAKEUP_GPIO) {
            ESP_LOGI(TAG, "ESP32 SoC woke up by EXTERNAL BUTTON PRESSED!");
        } else {
            ESP_LOGI(TAG, "ESP32 SoC woke up by TIMER EXPIRATION!");
        }

        // 重新启动 SSD1306 的电荷泵并刷写恢复原显存
        oled_sleep_exit();

        // 激活被挂起的线程
        ESP_LOGI(TAG, "Resuming SSD Task context loop...");
        vTaskResume(ssd_task_handle);
    }
}
