
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


内存清理示例
```aiignore
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char *TAG = "err_handling";

// 模拟一个复杂的处理函数：获取芯片信息，并对其进行加密或进一步处理
esp_err_t process_chip_info_securely(void)
{
    // 初始化所有指针为 NULL，这是安全释放的关键
    char *chip_info = NULL;
    char *processed_data = NULL;
    esp_err_t ret = ESP_OK;

    // 步骤 1: 分配第一个内存块（获取芯片信息）
    chip_info = get_chip_info_string_heap();
    if (chip_info == NULL) {
        ESP_LOGE(TAG, "Failed to get chip info");
        ret = ESP_ERR_NO_MEM;
        goto cleanup; // 直接跳到末尾清理
    }

    // 步骤 2: 模拟分配第二个内存块（例如用于存放处理后的数据）
    size_t out_len = strlen(chip_info) + 32;
    processed_data = (char *)heap_caps_malloc(out_len, MALLOC_CAP_8BIT);
    if (processed_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate process buffer");
        ret = ESP_ERR_NO_MEM;
        goto cleanup; // 跳到末尾，此时 chip_info 会被安全释放
    }

    // 步骤 3: 模拟某种可能会失败的业务操作（例如硬件加密、校验等）
    bool hardware_busy = true; // 模拟错误场景
    if (hardware_busy) {
        ESP_LOGE(TAG, "Crypto hardware timeout");
        ret = ESP_ERR_TIMEOUT;
        goto cleanup; // 跳到末尾，chip_info 和 processed_data 都会被释放
    }

    // 如果一切顺利，执行正常业务
    ESP_LOGI(TAG, "Data processed successfully");

cleanup:
    // 集中销毁阶段：free 安全地允许传入 NULL 指针
    // 如果某个指针没有被成功分配（仍为 NULL），heap_caps_free 会直接忽略它，不会崩溃
    if (chip_info) {
        heap_caps_free(chip_info);
        chip_info = NULL;
    }
    if (processed_data) {
        heap_caps_free(processed_data);
        processed_data = NULL;
    }

    return ret;
}

```

内清清理示范2
``` 
esp_err_t process_chip_info_alt(void)
{
    char *chip_info = NULL;
    esp_err_t ret = ESP_OK;

    do {
        // 步骤 1: 分配内存
        chip_info = get_chip_info_string_heap();
        if (chip_info == NULL) {
            ret = ESP_ERR_NO_MEM;
            break; // 跳出 do-while 块，进入下方的清理代码
        }

        // 步骤 2: 模拟其他可能失败的校验
        if (strlen(chip_info) == 0) {
            ret = ESP_ERR_INVALID_SIZE;
            break; // 跳出 do-while 块
        }

        // 正常业务逻辑...

    } while (0); // 保证只执行一次

    // 统一出口：无论上面哪里 break 出来的，都会执行到这里
    if (chip_info) {
        heap_caps_free(chip_info);
    }

    return ret;
}

```
