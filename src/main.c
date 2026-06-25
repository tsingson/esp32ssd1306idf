#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled_ssd1306.h"

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

    // 2. ⚠️ 注意：因为原本的字库是点亮像素，如果要把字写在实心矩形里面，需要配合反色。
    // 目前你给出的 8x8 字库显示函数是“按位或 |=”操作，它会将白字叠在白背景上导致看不清。
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
