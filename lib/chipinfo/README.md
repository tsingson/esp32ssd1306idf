
usage

``` 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h" // 确保引入了堆内存管理头文件

// 声明您之前定义的函数
char *get_chip_info_string_heap(void);

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Application started. Fetching chip info...");

    // 1. 调用函数，获取动态分配的芯片信息字符串
    char *chip_info_str = get_chip_info_string_heap();

    // 2. 必须检查返回值是否为 NULL，防止内存不足导致空指针崩溃
    if (chip_info_str != NULL) {
        
        // 示例 A: 使用 ESP_LOG 打印（推荐的 ESP-IDF 打印方式）
        ESP_LOGI(TAG, "Successfully retrieved info:\n%s", chip_info_str);

        // 示例 B: 进行其他操作，例如判断字符串里是否包含 "ESP32S3"
        // if (strstr(chip_info_str, "ESP32S3") != NULL) { ... }

        // 3. 【极重要】使用完毕后，必须释放内存
        heap_caps_free(chip_info_str);
        
        // 4. 将指针置空，防止变成野指针
        chip_info_str = NULL;
        
        ESP_LOGI(TAG, "Memory has been freed successfully.");
        
    } else {
        // 内存分配失败时的处理逻辑
        ESP_LOGE(TAG, "Failed to allocate memory for chip info string!");
    }

    // 主循环
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

```
