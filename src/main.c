#include "esp_log.h"
#include "esp_sleep.h" // 引入 ESP32 睡眠控制核心库
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled_ssd1306.h"
#include <stdio.h>

/**
void app_main(void) {
  if (oled_init() != ESP_OK)
    return;

  while (1) {
    oled_clear();

    // 案例 1：绘制一个标准的精美折线统计图
    // 绘制坐标轴
    oled_draw_line(10, 54, 118, 54, 1); // X 轴
    oled_draw_line(10, 10, 10, 54, 1);  // Y 轴

    // 绘制数据折线（连接四个散点：(10,45) -> (40,20) -> (80,35) -> (118,15)）
    oled_draw_line(10, 45, 40, 20, 1);
    oled_draw_line(40, 20, 80, 35, 1);
    oled_draw_line(80, 35, 118, 15, 1);


    // 案例 2：在屏幕右侧绘制一个“关闭/返回”的交叉打叉（X）小图标
    oled_draw_line(110, 10, 120, 20, 1); // 西北-东南对角线
    oled_draw_line(110, 20, 120, 10, 1); // 西南-东北对角线

    // 配合之前的反色和文本接口渲染出完整的现代化图表界面
    oled_show_string_ex(16, 56, "Sensor History", 0);

    oled_refresh();

    vTaskDelay(pdMS_TO_TICKS(1000));

    // 正常工作
    oled_clear();
    oled_show_string(16, 24, "design by gemini!");
    vTaskDelay(pdMS_TO_TICKS(1000));
    oled_clear();

    // 1. 在屏幕中央显示文字
    oled_show_string(24, 24, "Hello World!");

    // 2. 绘制一个外围包裹矩形框 (左上角在 (16,16)，宽 96 像素，高 32 像素)
    oled_draw_rectangle(16, 16, 96, 32);
    vTaskDelay(pdMS_TO_TICKS(1000));
    // 3. 将缓冲区推送到屏幕生效
    oled_refresh();

    // 测试 1：传入超长字符串，它会在右侧边界自动换行
    oled_show_string_wrap(
        0, 0, "This is an extremely long string that will wrap automatically.");

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
    oled_clear();

    // 1. 在屏幕中央显示文字
    oled_show_string(24, 24, "Hello World!");

    // 2. 绘制一个外围包裹矩形框 (左上角在 (16,16)，宽 96 像素，高 32 像素)
    oled_draw_rectangle(16, 16, 96, 32);
    vTaskDelay(pdMS_TO_TICKS(1000));
    // 3. 将缓冲区推送到屏幕生效
    oled_refresh();
    oled_clear();

    // 1. 在屏幕顶部画一个实心填充矩形作为“标题栏背景” (宽128像素满屏，高16像素)
    oled_fill_rectangle(0, 0, 128, 16);

    // 2. ⚠️
注意：因为原本的字库是点亮像素，如果要把字写在实心矩形里面，需要配合反色。
    // 目前你给出的 8x8 字库显示函数是“按位或
|=”操作，它会将白字叠在白背景上导致看不清。
    // 我们可以直接在下方绘制普通非反色区域：
    oled_show_string(0, 24, "System Status:");
    oled_show_string(0, 40, "Battery: 98%");

    // 3. 绘制一个电量条的外框和内部实心进度
    oled_draw_rectangle(90, 40, 30, 10);      // 电池外壳空心框
    oled_fill_rectangle(92, 42, 22, 6);       // 内部电量实心填充（代表充满）

    oled_refresh();

    oled_clear();

    // 案例 1：绘制一个高亮的系统标题栏
    // 先铺一层满屏宽、8像素高的白色实心背景
    oled_fill_rectangle(0, 0, 128, 8);
    // 在白色背景上写入反色黑字 (invert = 1)
    oled_show_string_ex(0, 0, "--- MAIN MENU ---", 1);


    // 案例 2：模拟一个菜单选中项的效果
    oled_show_string_ex(8, 24, "1. Wi-Fi Config", 0);  // 未选中项：正常白字

    // 选中项：先绘制一个局部白色高亮条，覆盖第二行
    oled_fill_rectangle(4, 40, 120, 8);
    oled_show_string_ex(8, 40, "2. Bluetooth (X)", 1); // 选中项：白底写黑字


    oled_refresh();


    vTaskDelay(pdMS_TO_TICKS(4000));
  }
}
*/

static const char *TAG = "app_power";

void app_main(void) {
  // 1. 初始化模块化屏幕组件
  if (oled_init() != ESP_OK) {
    ESP_LOGE(TAG, "OLED initialization failed!");
    return;
  }

  ESP_LOGI(TAG, "System initialization complete. Entering main loop.");

  while (1) {
    oled_clear();

    // 案例 1：在屏幕左侧绘制一个精美的同心圆雷达波纹效果
    oled_draw_circle(32, 32, 28, 1); // 外圈
    oled_draw_circle(32, 32, 18, 1); // 中圈
    oled_draw_circle(32, 32, 8, 1);  // 内圈
    // 为雷达中心打上一个十字瞄准线（复用你之前的画线接口）
    oled_draw_line(32, 2, 32, 62, 1);
    oled_draw_line(2, 32, 62, 32, 1);

    // 案例 2：在屏幕右侧制作一个带圆圈外框的独立按钮
    oled_draw_circle(96, 32, 15, 1); // 绘制按钮的圆形外边缘
    // 在圆圈中央写一个短字符（把 8x8 字符完美定位在中心）
    oled_show_string_ex(92, 28, "OK", 0);

    oled_refresh();
    vTaskDelay(pdMS_TO_TICKS(3000));

    // ==========================================================
    // 运行阶段：屏幕点亮并显示当前系统状态
    // ==========================================================
    oled_clear();
    oled_show_string_ex(0, 0, "=== RUNNING ===", 1); // 顶部白底黑字
    oled_show_string_ex(0, 24, "ESP32: CPU Active", 0);
    oled_show_string_ex(0, 40, "OLED: Charge Pump ON", 0);
    oled_draw_rectangle(0, 56, 128, 8); // 底部装饰条
    oled_refresh();

    ESP_LOGI(TAG, "System active, keeping awake for 3 seconds...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // ==========================================================
    // 同步休眠阶段：准备进入 Light-sleep
    // ==========================================================
    ESP_LOGI(TAG, "Preparing for synchronization sleep...");

    // 步骤 A: 让 SSD1306 内部电荷泵彻底断电，屏幕黑屏，防止屏幕持续拉高电流
    oled_sleep_enter();

    // 步骤 B: 设置 ESP32 定时唤醒源，单位为微秒（5,000,000 微秒 = 5 秒）
    const int sleep_time_ms = 5000;
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(sleep_time_ms * 1000));

    ESP_LOGI(TAG, "ESP32 Entering Light Sleep Now...");

    // 🌟 核心：执行此函数后，ESP32 芯片将立刻挂起 CPU 核心并进入浅度睡眠状态。
    // 代码会在这里“暂停” 5
    // 秒。此时芯片+屏幕的总运行电流会降低至微安（uA）级别。
    esp_light_sleep_start();

    // ==========================================================
    // 同步唤醒阶段：5秒定时到期，硬件自动执行到这里
    // ==========================================================
    // 步骤 C: 芯片恢复运行，立刻同步唤醒 SSD1306
    // 屏幕，重新激发电荷泵并自动恢复刷写原显存画面
    oled_sleep_exit();

    ESP_LOGI(TAG, "ESP32 and OLED Synchronized Wakeup Success!");

    // 唤醒后短暂展示一个提示，随后进入下一个大循环
    oled_clear();
    oled_show_string_ex(0, 16, "Wakeup Success!", 0);
    oled_show_string_ex(0, 32, "Entering Loop...", 0);
    oled_refresh();
    vTaskDelay(pdMS_TO_TICKS(1500));
  }
}
