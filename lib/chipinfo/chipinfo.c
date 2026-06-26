#include "chipinfo.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h" // 新增：提供 heap_caps_malloc 和 heap_caps_free
#include "freertos/FreeRTOS.h"
#include <stdio.h>

static const char *model_info(esp_chip_model_t model) {
  switch (model) {
  case CHIP_ESP32:
    return "ESP32";
  case CHIP_ESP32S2:
    return "ESP32S2";
  case CHIP_ESP32S3:
    return "ESP32S3";
  case CHIP_ESP32C3:
    return "ESP32C3";
  case CHIP_ESP32H2:
    return "ESP32H2";
  case CHIP_ESP32C2:
    return "ESP32C2";
  default:
    return "Unknown";
  }
}

/* 新增函数：使用 heap_caps_malloc 动态分配内存并返回芯片信息字符串指针
   注意：使用完该字符串后，必须手动调用 heap_caps_free() 释放内存 */
char *get_chip_info_string_heap(void) {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  unsigned major_rev = chip_info.revision / 100;
  unsigned minor_rev = chip_info.revision % 100;

  // 1. 第一次调用 snprintf，计算所需的精确缓冲区大小
  int len = snprintf(
      NULL, 0,
      "Chip model %s with %d CPU core(s), WiFi%s%s, silicon revision v%d.%d",
      model_info(chip_info.model), chip_info.cores,
      (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
      (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "", major_rev,
      minor_rev);

  if (len < 0) {
    return NULL;
  }

  // 2. 使用 ESP-IDF 堆管理函数分配内存
  // MALLOC_CAP_8BIT 保证了分配的内存支持字节操作，非常适合字符串
  char *buf = (char *)heap_caps_malloc(len + 1, MALLOC_CAP_8BIT);
  if (buf == NULL) {
    return NULL; // 堆内存不足，分配失败
  }

  // 3. 第二次调用 snprintf，将内容真正写入分配置的内存中
  snprintf(
      buf, len + 1,
      "Chip model %s with %d CPU core(s), WiFi%s%s, silicon revision v%d.%d",
      model_info(chip_info.model), chip_info.cores,
      (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
      (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "", major_rev,
      minor_rev);

  return buf;
}

/* 原始函数：调用堆内存分配函数并打印，随后立即释放内存 */
void print_chip_info() {
  char *info_str = get_chip_info_string_heap();

  if (info_str != NULL) {
    printf("\n\n%s\n\n", info_str);

    // 推荐使用 heap_caps_free 释放对应的堆内存（也可以直接使用标准 free）
    heap_caps_free(info_str);
  } else {
    printf("\n\nFailed to allocate heap memory for chip info.\n\n");
  }
}

// void app_main()
// {
//     for (;;)
//     {
//         print_chip_info();
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     };
// }
