#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "oled_ssd1306.h"

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
            // Block indefinitely until an item arrives from the queue
            if (xQueueReceive(data_queue, &received_val, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI(TAG, "SSD Task processed value: %d", received_val);

                // Format string output line matching the requirements
                snprintf(display_str, sizeof(display_str), "queue rcv: %d", received_val);

                // Render cleanly onto the canvas using our robust 8x8 font engine
                oled_clear();
                oled_show_string_ex(0, 0, "=== SYSTEM ===", 1); // Fixed header
                oled_show_string_ex(0, 24, display_str, 0);       // Changing dynamic data row
                oled_refresh();

                // Small human-readable delay between data print frames
                vTaskDelay(pdMS_TO_TICKS(400));
            }
        }

        // Step 2: Completed processing 10 values. Enforce required 2-second hold delay
        ESP_LOGI(TAG, "SSD Task finished 10 items. Holding view for 2s...");
        vTaskDelay(pdMS_TO_TICKS(2000));

        // Step 3: Flash a message stating rendering loop complete
        oled_clear();
        oled_show_string_ex(0, 0, "=== SYSTEM ===", 1);
        oled_show_string_ex(0, 24, "Batch Complete!", 0);
        oled_refresh();

        // Step 4: Fire signaling semaphore upstream to indicate task batch wrap up
        xSemaphoreGive(display_done_sem);

        // Step 5: Force task into suspension loop awaiting explicit external wakeup call
        ESP_LOGI(TAG, "SSD Task suspending self to wait for main thread...");
        vTaskSuspend(NULL);
    }
}

void app_main(void) {
    // Initialize our zero-RAM-pollution driver
    if (oled_init() != ESP_OK) {
        ESP_LOGE(TAG, "OLED Core Engine Init Failed!");
        return;
    }

    // Allocate FreeRTOS primitives
    data_queue = xQueueCreate(10, sizeof(int));
    display_done_sem = xSemaphoreCreateBinary();

    if (data_queue == NULL || display_done_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create OS primitives!");
        return;
    }

    // Spawn the background consumer processing task loop
    xTaskCreate(ssd_task, "ssd_task", 3072, NULL, 5, &ssd_task_handle);

    while (1) {
        ESP_LOGI(TAG, "===============================");
        ESP_LOGI(TAG, "Starting new processing cycle...");
        ESP_LOGI(TAG, "===============================");

        // Step 1: Consecutively write integer data 1 through 10 downstream into the queue pipeline
        for (int val = 1; val <= 10; val++) {
            ESP_LOGI(TAG, "Main writing to queue: %d", val);
            xQueueSend(data_queue, &val, portMAX_DELAY);
        }

        // Step 2: Block main thread execution context until SSD worker fires done semaphore
        ESP_LOGI(TAG, "Main thread waiting for display batch completion signal...");
        xSemaphoreTake(display_done_sem, portMAX_DELAY);

        // Step 3: Synchronously invoke low-power peripheral sleep profiles
        ESP_LOGI(TAG, "Signal received. Putting peripheral to hardware sleep mode...");
        oled_sleep_enter(); // Shuts down SSD1306 charge pump -> Microamps drop

        // Step 4: Keep main sleeping for exactly 30 seconds as explicitly requested
        ESP_LOGI(TAG, "Main entering a 30-second low-power sleep phase...");
        vTaskDelay(pdMS_TO_TICKS(30000));

        // Step 5: 30 seconds elapsed. Perform coordinated hardware wake sequences
        ESP_LOGI(TAG, "30-second timer elapsed! Re-awakening hardware components...");
        oled_sleep_exit();  // Recalibrates SSD1306 VCC charge pumps and updates active frame buffer

        // Step 6: Wake the consumer task from its suspended state to handle next iteration
        ESP_LOGI(TAG, "Resuming SSD Task context loop...");
        vTaskResume(ssd_task_handle);
    }
}
