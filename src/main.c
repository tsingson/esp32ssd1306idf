#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_sleep.h"      // 🌟 Crucial: Added for ESP32 SoC low-power sleep registers
#include "esp_log.h"
#include "oled_ssd1306.h"
#include "driver/uart.h" // 🌟 显式引入 UART 驱动头文件


static const char *TAG = "main";

// FreeRTOS Handles
static QueueHandle_t data_queue = NULL;
static SemaphoreHandle_t display_done_sem = NULL;
static TaskHandle_t ssd_task_handle = NULL;

/**
 * @brief SSD Display Processing Task
 */
void ssd_task(void *pvParameters) {
    int received_val = 0;
    char display_str[32];

    while (1) {
        // Step 1: Wait and pull exactly 10 sequential elements out of the queue
        for (int i = 0; i < 10; i++) {
            if (xQueueReceive(data_queue, &received_val, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI(TAG, "SSD Task processed value: %d", received_val);

                snprintf(display_str, sizeof(display_str), "queue rcv: %d", received_val);

                oled_clear();
                oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);
                oled_show_string_ex(0, 24, display_str, 0);
                oled_refresh();

                vTaskDelay(pdMS_TO_TICKS(400));
            }
        }

        // Step 2: Completed processing 10 values. Enforce 2-second hold delay
        ESP_LOGI(TAG, "SSD Task finished 10 items. Holding view for 2s...");
        vTaskDelay(pdMS_TO_TICKS(2000));

        oled_clear();
        oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);
        oled_show_string_ex(0, 24, "Batch Complete!", 0);
        oled_refresh();

        // Step 3: Fire signaling semaphore upstream to indicate task batch wrap up
        xSemaphoreGive(display_done_sem);

        // Step 4: Force task into suspension loop awaiting explicit external wakeup call
        ESP_LOGI(TAG, "SSD Task suspending self to wait for main thread...");
        vTaskSuspend(NULL);
    }
}

void app_main(void) {
    if (oled_init() != ESP_OK) {
        ESP_LOGE(TAG, "OLED Core Engine Init Failed!");
        return;
    }

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

        // Step 1: Consecutively write integer data 1 through 10 downstream into the queue
        for (int val = 1; val <= 10; val++) {
            xQueueSend(data_queue, &val, portMAX_DELAY);
        }

        // Step 2: Block main thread until SSD worker fires done semaphore
        xSemaphoreTake(display_done_sem, portMAX_DELAY);

        // Step 3: Put the peripheral display into physical sleep mode (turns off charge pump)
        ESP_LOGI(TAG, "Signal received. Putting OLED to hardware sleep mode...");
        oled_sleep_enter();

        // Step 4: Configure the ESP32 SoC RTC Timer to wake up after 30 seconds
        // 30 seconds = 30,000,000 microseconds
        uint64_t sleep_time_us = 30ULL * 1000ULL * 1000ULL;
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(sleep_time_us));

        // Step 5: Enforce SoC Light Sleep
        ESP_LOGI(TAG, "ESP32 Entering SoC Light Sleep Now (RAM preserved)...");
        // 🌟 完美适配 ESP-IDF 5.x 的新版串口冲刷等待函数
        //  正确单参数写法，完美适配你的 ESP-IDF 5.3.1 串口驱动
        uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);



        // 🌟 Execution FREEZES here. CPU clocks stop, RTC timer ticks down 30s in the background.
        esp_light_sleep_start();

        // Step 6: 30 seconds elapsed! The SoC hardware automatic wake-up sequence resumes here seamlessly.
        ESP_LOGI(TAG, "ESP32 SoC woke up from Light Sleep!");

        // Step 7: Wake up the OLED display peripheral (Re-ignites I2C and VCC charge pump)
        oled_sleep_exit();

        // Step 8: Resume the suspended background consumer processing task loop
        ESP_LOGI(TAG, "Resuming SSD Task context loop...");
        vTaskResume(ssd_task_handle);
    }
}
