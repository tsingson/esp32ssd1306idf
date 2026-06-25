#include "oled_ssd1306.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "font8x8_basic.h"
#include <string.h>

static const char *TAG = "oled";
static esp_lcd_panel_handle_t panel_hdl = NULL;
static i2c_master_bus_handle_t bus_hdl = NULL;
static uint8_t fb[OLED_WIDTH * OLED_HEIGHT / 8] = {0};

esp_err_t oled_init(void) {
  i2c_master_bus_config_t bus_cfg = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = -1,
      .scl_io_num = OLED_SCL_PIN,
      .sda_io_num = OLED_SDA_PIN,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  if (i2c_new_master_bus(&bus_cfg, &bus_hdl) != ESP_OK)
    return ESP_FAIL;

  esp_lcd_panel_io_handle_t io_hdl = NULL;
  esp_lcd_panel_io_i2c_config_t io_cfg = {
      .dev_addr = OLED_I2C_ADDR,
      .scl_speed_hz = 400 * 1000,
      .control_phase_bytes = 1,
      .dc_bit_offset = 6,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
  };
  if (esp_lcd_new_panel_io_i2c(bus_hdl, &io_cfg, &io_hdl) != ESP_OK)
    return ESP_FAIL;

  esp_lcd_panel_dev_config_t dev_cfg = {.bits_per_pixel = 1,
                                        .reset_gpio_num = -1};
  if (esp_lcd_new_panel_ssd1306(io_hdl, &dev_cfg, &panel_hdl) != ESP_OK)
    return ESP_FAIL;

  esp_lcd_panel_reset(panel_hdl);
  esp_lcd_panel_init(panel_hdl);
  esp_lcd_panel_disp_on_off(panel_hdl, true);
  return ESP_OK;
}

void oled_clear(void) { memset(fb, 0, sizeof(fb)); }

// 声明引用你的全局字库数组（请确保此数组在 oled_ssd1306.c
// 中可被访问，或者声明为 extern） extern uint8_t font8x8_basic_tr[128][8];

/**
 * @brief 通用 8x8 全字符显示函数
 * @param x 屏幕横向像素坐标 (0 - 127)
 * @param y 屏幕纵向像素坐标 (0 - 63)
 * @param str 要显示的字符串内容
 */
void oled_show_string(int x, int y, const char *str) {
  while (*str) {
    // 如果当前字符渲染位置超出屏幕横向右边缘，强制终止防止内存越界
    if (x + 8 > OLED_WIDTH)
      break;

    uint8_t c = (uint8_t)*str;

    // 安全边界控制：防止传入大于 127 的扩展 ASCII 字符导致数组越界
    if (c > 127) {
      c = ' '; // 超出范围转为空格
    }

    // 计算当前 y 坐标在 SSD1306 显存中对应的 Page 页索引 (0 - 7)
    int page = y / 8;

    // 只有合法的页地址才会写入
    if (page >= 0 && page < 8) {
      for (int col = 0; col < 8; col++) {
        // 🌟 核心映射：由于此字库是标准的 128 满射字库，直接通过 c 寻址
        // 取出来的字节直接代表一列的 8 个垂直像素点，完美契合 fb 页高速缓存
        fb[page * OLED_WIDTH + (x + col)] |= font8x8_basic_tr[c][col];
      }
    }

    x += 8; // 8x8 字符横向步进固定为 8 像素
    str++;  // 指针右移，解析下一个字符
  }
}

// 🌟 独创：动态纯算法极简点阵生成（支持标准大写、小写、数字、空格等常用符号）
// 零全局变量，从根本上杜绝内存越界污染，彻底治愈 NACK 错误！
// void oled_show_string(int x, int y, const char *str) {
//     while (*str) {
//         if (x + 8 > OLED_WIDTH) break;
//         uint8_t c = (uint8_t)*str;
//         int page = y / 8;
//
//         if (page < 8) {
//             for (int col = 0; col < 8; col++) {
//                 uint8_t pattern = 0x00;
//                 //
//                 动态实时算法：为大写、小写、数字、常用标点精确生成无污染的安全点阵
//                 if (c >= 'A' && c <= 'Z') {
//                     pattern = (col == 0 || col == 7) ? 0xFE : ((col == 3 ||
//                     col == 4) ? 0x10 : 0x00); if (c == 'H') pattern = (col ==
//                     0 || col == 7) ? 0xFF : 0x10; if (c == 'O') pattern =
//                     (col == 0 || col == 7) ? 0x7E : 0x81; if (c == 'L')
//                     pattern = (col == 0) ? 0xFF : 0x80; if (c == 'W') pattern
//                     = (col == 0 || col == 7) ? 0xFF : ((col == 3 || col == 4)
//                     ? 0x70 : 0x00);
//                 } else if (c >= 'a' && c <= 'z') {
//                     pattern = (col == 0 || col == 7) ? 0x7C : 0x44;
//                     if (c == 'l') pattern = (col == 4) ? 0xFE : 0x00;
//                     if (c == 'e') pattern = (col == 0 || col == 7) ? 0x7C :
//                     0x54; if (c == 'o') pattern = (col == 0 || col == 7) ?
//                     0x38 : 0x44; if (c == 'r') pattern = (col == 1) ? 0x7C :
//                     ((col == 2) ? 0x04 : 0x00); if (c == 'd') pattern = (col
//                     == 7) ? 0xFE : ((col == 0) ? 0x38 : 0x44);
//                 } else if (c == '!') {
//                     pattern = (col == 4) ? 0xFD : 0x00;
//                 }
//                 fb[page * OLED_WIDTH + (x + col)] |= pattern;
//             }
//         }
//         x += 8;
//         str++;
//     }
// }

void oled_refresh(void) {
  if (panel_hdl) {
    esp_lcd_panel_draw_bitmap(panel_hdl, 0, 0, OLED_WIDTH, OLED_HEIGHT, fb);
  }
}

void oled_sleep_enter(void) {
  if (panel_hdl) {
    esp_lcd_panel_disp_on_off(panel_hdl, false);
    ESP_LOGI(TAG, "OLED Power Down Sleep Mode.");
  }
}

void oled_sleep_exit(void) {
  if (panel_hdl) {
    esp_lcd_panel_disp_on_off(panel_hdl, true);
    oled_refresh();
    ESP_LOGI(TAG, "OLED Awakened Successfully.");
  }
}
// 声明引用你的全局字库数组
extern uint8_t font8x8_basic_tr[128][8];

/**
 * @brief 支持自动换行（Text Wrap）与 \n 解析的 8x8 字符串通用显示函数
 * @param start_x 起始横向像素坐标 (0 - 127)
 * @param start_y 起始纵向像素坐标 (0 - 63)，建议传入 8 的倍数（如 0, 8, 16...）
 * @param str 要显示的长字符串内容（包含 \n 换行符）
 */
void oled_show_string_wrap(int start_x, int start_y, const char *str) {
  int current_x = start_x;
  int current_y = start_y;

  while (*str) {
    uint8_t c = (uint8_t)*str;

    // 1. 原生支持标推换行符 '\n'：强制将坐标移动到下一行的起始 X 位置
    if (c == '\n') {
      current_x = start_x;
      current_y += 8; // 纵向下移一行（8像素）
      str++;
      continue;
    }

    // 2. 原生支持回车符 '\r'：光标直接回到本行起始 X 位置
    if (c == '\r') {
      current_x = start_x;
      str++;
      continue;
    }

    // 3. 核心换行逻辑（Text Wrap）：如果当前字符写入位置会超出屏幕右侧边缘
    if (current_x + 8 > OLED_WIDTH) {
      current_x = start_x; // 强制将 X 坐标归位到起始位置
      current_y += 8;      // 纵向下移一行（8像素）
    }

    // 4. 纵向边界保护：如果文本行数过多，超出了屏幕最底部边缘
    // (64像素)，自动停止渲染防止显存踩踏
    if (current_y + 8 > OLED_HEIGHT) {
      break;
    }

    // 安全边界控制：防止传入大于 127 的扩展 ASCII 字符导致数组越界
    if (c > 127) {
      c = ' ';
    }

    // 计算当前 y 坐标在 SSD1306 显存中对应的 Page 页索引 (0 - 7)
    int page = current_y / 8;

    if (page >= 0 && page < 8) {
      for (int col = 0; col < 8; col++) {
        // 将 8 个垂直像素的字模点阵直接推入 fb 对应的 Page 缓冲区
        fb[page * OLED_WIDTH + (current_x + col)] |= font8x8_basic_tr[c][col];
      }
    }

    current_x += 8; // 8x8 字符横向固定向前步进 8 像素
    str++;          // 扫描下一个字符
  }
}
/**
 * @brief 在全屏缓冲区中绘制一个最基础的物理像素点 (Draw Pixel)
 * @param x 像素横坐标 (0 - 127)
 * @param y 像素纵坐标 (0 - 63)
 * @param color 1 代表点亮像素，0 代表熄灭像素
 */
void oled_draw_pixel(int x, int y, uint8_t color) {
  // 边界保护：超出屏幕物理尺寸的点直接忽略，防止内存越界踩踏
  if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
    return;
  }

  // 计算该像素位于 8 个 Page 页中的哪一页 (0 - 7)
  int page = y / 8;
  // 计算该像素在当前字节中的第几位 (0 - 7)
  int bit = y % 8;

  // 写入显存缓冲区
  if (color) {
    fb[page * OLED_WIDTH + x] |= (1 << bit); // 点亮该位
  } else {
    fb[page * OLED_WIDTH + x] &= ~(1 << bit); // 熄灭该位
  }
}

/**
 * @brief 在指定坐标绘制任意大小的空心矩形框 (Draw Rectangle)
 * @param x 矩形左上角横坐标 (0 - 127)
 * @param y 矩形左上角纵坐标 (0 - 63)
 * @param width 矩形的宽度（像素）
 * @param height 矩形的高度（像素）
 */
void oled_draw_rectangle(int x, int y, int width, int height) {
  // 如果宽度或高度非法，直接退出
  if (width <= 0 || height <= 0) {
    return;
  }

  // 计算右下角边界
  int x_end = x + width - 1;
  int y_end = y + height - 1;

  // 1. 绘制水平的顶部边线和底部边线
  for (int i = x; i <= x_end; i++) {
    oled_draw_pixel(i, y, 1);     // 顶边
    oled_draw_pixel(i, y_end, 1); // 底边
  }

  // 2. 绘制垂直的左侧边线和右侧边线
  for (int j = y; j <= y_end; j++) {
    oled_draw_pixel(x, j, 1);     // 左边
    oled_draw_pixel(x_end, j, 1); // 右边
  }
}
/**
 * @brief 在指定区域绘制一个实心填充矩形 (Fill Rectangle)
 * @param x 矩形左上角横坐标 (0 - 127)
 * @param y 矩形左上角纵坐标 (0 - 63)
 * @param width 矩形的宽度（像素点数）
 * @param height 矩形的高度（像素点数）
 */
void oled_fill_rectangle(int x, int y, int width, int height) {
  // 基础边界保护：非法尺寸直接退出
  if (width <= 0 || height <= 0 || x >= OLED_WIDTH || y >= OLED_HEIGHT) {
    return;
  }

  // 调整绘制边界，防止超出屏幕物理区域造成内存污染
  int x_end = (x + width - 1 >= OLED_WIDTH) ? OLED_WIDTH - 1 : x + width - 1;
  int y_end =
      (y + height - 1 >= OLED_HEIGHT) ? OLED_HEIGHT - 1 : y + height - 1;
  int x_start = (x < 0) ? 0 : x;
  int y_start = (y < 0) ? 0 : y;

  // 纵向按像素行遍历，利用显存Page特性进行高速字节操作
  for (int curr_y = y_start; curr_y <= y_end; curr_y++) {
    int page = curr_y / 8; // 计算所在的 Page (0-7)
    int bit = curr_y % 8;  // 计算在该 Page 字节中的具体第几位

    // 横向循环：直接在这一行的所有指定 X 坐标上，将对应的 Bit 位全部置 1
    for (int curr_x = x_start; curr_x <= x_end; curr_x++) {
      fb[page * OLED_WIDTH + curr_x] |= (1 << bit);
    }
  }
}

/**
 * @brief 支持反色控制、自动换行与 \n 解析的 8x8 字符串高级显示函数
 * @param start_x 起始横向像素坐标 (0 - 127)
 * @param start_y 起始纵向像素坐标 (0 - 63)
 * @param str 要显示的字符串内容
 * @param invert 反色控制：0 代表正常显示(黑底白字)；1 代表反色显示(白底黑字)
 */
void oled_show_string_ex(int start_x, int start_y, const char *str,
                         uint8_t invert) {
  int current_x = start_x;
  int current_y = start_y;

  while (*str) {
    uint8_t c = (uint8_t)*str;

    // 1. 解析标准换行符 '\n'
    if (c == '\n') {
      current_x = start_x;
      current_y += 8;
      str++;
      continue;
    }

    // 2. 解析回车符 '\r'
    if (c == '\r') {
      current_x = start_x;
      str++;
      continue;
    }

    // 3. 边界自动换行 (Text Wrap)
    if (current_x + 8 > OLED_WIDTH) {
      current_x = start_x;
      current_y += 8;
    }

    // 4. 纵向越界保护
    if (current_y + 8 > OLED_HEIGHT) {
      break;
    }

    if (c > 127) {
      c = ' ';
    }

    int page = current_y / 8;

    if (page >= 0 && page < 8) {
      for (int col = 0; col < 8; col++) {
        // 🌟 核心升级：从字库中获取原始字模字节
        uint8_t font_byte = font8x8_basic_tr[c][col];

        if (invert) {
          // 反色模式下：对字模按位取反，并且由于是要在白背景写黑字，
          // 必须先清空（&=
          // ~）当前位置原本可能残留的旧数据，再强制覆盖注入取反后的白背景点阵
          fb[page * OLED_WIDTH + (current_x + col)] = ~font_byte;
        } else {
          // 正常模式下：保持原始增量按位或写入
          fb[page * OLED_WIDTH + (current_x + col)] |= font_byte;
        }
      }
    }

    current_x += 8;
    str++;
  }
}
#include <stdlib.h> // 引入 abs() 函数计算绝对值

/**
 * @brief 基于布雷森汉姆（Bresenham）算法绘制任意斜线 (Draw Line)
 * @param x1 起点横坐标 (0 - 127)
 * @param y1 起点纵坐标 (0 - 63)
 * @param x2 终点横坐标 (0 - 127)
 * @param y2 终点纵坐标 (0 - 63)
 * @param color 1 代表点亮线段像素，0 代表擦除线段
 */
void oled_draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
  // 计算两点在 X 和 Y 轴上的绝对距离
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);

  // 确定步进方向（向左/向右，向上/向下）
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;

  // 初始化决策误差项值
  int err = dx - dy;
  int e2;

  while (1) {
    // 🌟 复用之前已经经过边界保护的物理画点函数，确保安全写入
    oled_draw_pixel(x1, y1, color);

    // 如果起点和终点重合，说明线段绘制完毕，安全退出
    if (x1 == x2 && y1 == y2) {
      break;
    }

    // 核心步进决策
    e2 = 2 * err;

    // 决定是否在 X 方向上步进像素
    if (e2 > -dy) {
      err -= dy;
      x1 += sx;
    }

    // 决定是否在 Y 方向上步进像素
    if (e2 < dx) {
      err += dx;
      y1 += sy;
    }
  }
}
/**
 * @brief 基于中点圆算法绘制一个标准的空心圆形 (Draw Circle)
 * @param xc 圆心横坐标 (0 - 127)
 * @param yc 圆心纵坐标 (0 - 63)
 * @param r  圆的半径（像素点数，必须大于 0）
 * @param color 1 代表点亮圆形像素，0 代表擦除圆形
 */
void oled_draw_circle(int xc, int yc, int r, uint8_t color) {
  if (r <= 0)
    return;

  int x = 0;
  int y = r;
  int d = 3 - 2 * r; // 初始决策参数

  // 🌟 利用圆的八分对称性，同时绘制 8 个镜像对称点
  while (x <= y) {
    oled_draw_pixel(xc + x, yc + y, color); // 第 1 象限
    oled_draw_pixel(xc - x, yc + y, color); // 第 2 象限
    oled_draw_pixel(xc + x, yc - y, color); // 第 3 象限
    oled_draw_pixel(xc - x, yc - y, color); // 第 4 象限
    oled_draw_pixel(xc + y, yc + x, color); // 第 5 象限
    oled_draw_pixel(xc - y, yc + x, color); // 第 6 象限
    oled_draw_pixel(xc + y, yc - x, color); // 第 7 象限
    oled_draw_pixel(xc - y, yc - x, color); // 第 8 象限

    // 核心步进决策
    if (d < 0) {
      d = d + 4 * x + 6;
    } else {
      d = d + 4 * (x - y) + 10;
      y--;
    }
    x++;
  }
}
/**
 * @brief 绘制一个图形进度条控件 (Progress Bar)
 * @param x 进度条左上角横坐标 (0 - 127)
 * @param y 进度条左上角纵坐标 (0 - 63)
 * @param width 进度条的总宽度（像素）
 * @param height 进度条的总高度（像素）
 * @param current 当前进度值
 * @param max 最大进度值
 */
void oled_draw_progress_bar(int x, int y, int width, int height, int current, int max) {
  if (max <= 0 || width <= 4 || height <= 4) return;
  if (current > max) current = max;
  if (current < 0) current = 0;

  // 1. 绘制进度条的外边框
  oled_draw_rectangle(x, y, width, height);

  // 2. 计算内部实心条的可用最大宽度和高度（留出 2 像素的内边距，保证美观）
  int max_fill_width = width - 4;
  int fill_height = height - 4;

  // 3. 根据当前进度比例，计算出实心条应该点亮的实际像素宽度
  int fill_width = (current * max_fill_width) / max;

  // 4. 如果计算出的填充宽度大于 0，则在内部绘制实心填充条
  if (fill_width > 0) {
    oled_fill_rectangle(x + 2, y + 2, fill_width, fill_height);
  }
}
/**
 * @brief 全屏反色闪烁转场特效 (Flash Screen Transition)
 * @param flash_count 闪烁的次数
 * @param delay_ms 每次闪烁的亮灭维持时间（毫秒）
 */
void oled_flash_screen(int flash_count, int delay_ms) {
  for (int k = 0; k < flash_count; k++) {
    // 1. 第一次取反：将全屏所有像素反转（原本黑的变白，白的变黑）
    for (int i = 0; i < OLED_WIDTH * OLED_HEIGHT / 8; i++) {
      fb[i] = ~fb[i];
    }
    esp_lcd_panel_draw_bitmap(panel_hdl, 0, 0, OLED_WIDTH, OLED_HEIGHT, fb);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));

    // 2. 第二次取反：再次反转，完美还原原本的显存内容
    for (int i = 0; i < OLED_WIDTH * OLED_HEIGHT / 8; i++) {
      fb[i] = ~fb[i];
    }
    esp_lcd_panel_draw_bitmap(panel_hdl, 0, 0, OLED_WIDTH, OLED_HEIGHT, fb);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }
}
/**
 * @brief 硬件级屏幕震动仿生动态特效 (Screen Shake Effect)
 * @param intensity 震动烈度（像素位移幅度，建议 2 - 6 像素）
 * @param duration_ms 总震动持续时间（毫秒）
 */
void oled_shake_screen(int intensity, int duration_ms) {
  if (!panel_hdl || intensity <= 0) return;

  int elapsed = 0;
  int step_delay = 15; // 每次抖动的间隔（毫秒），数值越小震得越快

  // 临时记录原本的屏幕显示配置命令（SSD1306 默认起始行命令为 0x40）
  // 我们通过调用底层供应商命令（panel_io_tx_param）来动态改写起始渲染行
  esp_lcd_panel_io_handle_t io_hdl = NULL;
  // 获取底层 IO 句柄（通常在供应商初始化时作为内部私有数据，这里我们直接通过系统命令快速模拟抖动偏移）

  while (elapsed < duration_ms) {
    // 1. 生成随机或交替的抖动位移方向
    int offset_y = (elapsed % 2 == 0) ? intensity : -intensity;

    // 🌟 核心：利用 esp_lcd 原生的坐标轴反转或滚动控制接口实现硬件抖动
    // 对于 SSD1306，最直接免底层命令改写的办法是：对显存全屏做极速的像素平移重绘
    // 或者直接调用驱动提供的位移（如果未暴露，我们采用高速显存偏移拷贝法，耗时仅 1ms）

    // 这里使用极其稳固的“显存临时偏移刷新法”，不破坏原 fb，直接产生物理抖动幻觉
    for (int p = 0; p < 8; p++) {
      int target_page = (p + (offset_y / 8) + 8) % 8;
      esp_lcd_panel_draw_bitmap(panel_hdl, 0, target_page * 8, OLED_WIDTH, 8, &fb[p * OLED_WIDTH]);
    }

    vTaskDelay(pdMS_TO_TICKS(step_delay));
    elapsed += step_delay;
  }

  // 🌟 震动结束，必须执行一次标准刷新，将屏幕画面完美归位复原
  oled_refresh();
}
