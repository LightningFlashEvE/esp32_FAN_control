#include "oled_display.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <stdio.h>

static const char* TAG = "OLED";
#define OLED_ADDRESS 0x3C

/**
 * @brief 初始化 OLED 显示屏 (SSD1306)，通过 I2C 接口
 * @param i2c_num I2C 端口号
 * @param sda_pin SDA 引脚 GPIO
 * @param scl_pin SCL 引脚 GPIO
 */
void oled_init(i2c_port_t i2c_num, gpio_num_t sda_pin, gpio_num_t scl_pin) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl_pin,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(i2c_num, &conf);
    i2c_driver_install(i2c_num, I2C_MODE_MASTER, 0, 0, 0);

    // 初始化命令序列
    uint8_t init_cmds[] = {
        0xAE, // 关闭显示
        0x20, 0x00, // 设置内存寻址模式
        0xB0, // 设置页起始地址
        0xC8, // COM 输出扫描方向
        0x00, // 低列地址
        0x10, // 高列地址
        0x40, // 起始行地址
        0x81, 0xFF, // 对比度设置
        0xA1, // 段重映射
        0xA6, // 正常显示
        0xA8, 0x3F, // 多路复用比率
        0xA4, // 显示跟随 RAM 内容
        0xD3, 0x00, // 显示偏移
        0xD5, 0xF0, // 显示时钟分频比率
        0xD9, 0x22, // 预充电周期
        0xDA, 0x12, // COM 引脚配置
        0xDB, 0x20, // VCOMH 撤销电平
        0x8D, 0x14, // 电荷泵设置
        0xAF // 开启显示
    };
    for (int i = 0; i < sizeof(init_cmds); i++) {
        uint8_t cmd[2] = {0x00, init_cmds[i]};
        i2c_master_write_to_device(i2c_num, OLED_ADDRESS, cmd, 2, pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "OLED initialized");
}

/**
 * @brief 更新 OLED 显示：显示温度、模式和风扇转速主题
 * @param temperature 当前温度值，单位摄氏度
 * @param speed 当前风扇速度百分比 (0-100)
 * @param auto_mode 当前模式：true=自动，false=手动
 *
 * 主题设计：
 *  第一行显示 "温度:xx.x°C 模式:自动/手动"
 *  第二行展示文本化进度条，表示风扇转速
 */
void oled_display_update(float temperature, uint8_t speed, bool auto_mode) {
    char line1[32];
    char line2[32];
    // 构建第一行字符串：温度和模式
    snprintf(line1, sizeof(line1), "温度:%.1f°C 模式:%s", temperature, auto_mode ? "自动" : "手动");
    
    // 构建第二行字符串：速度进度条，共10格
    char bar[12];
    int filled = speed / 10; // 每格代表10%
    for (int i = 0; i < 10; i++) {
        bar[i] = (i < filled) ? '#' : '-';
    }
    bar[10] = '\0';
    snprintf(line2, sizeof(line2), "速度:%3d%% [%s]", speed, bar);

    // TODO: 调用 OLED 驱动函数清屏与绘制文本，以下为示例日志输出
    ESP_LOGI(TAG, "OLED 第一行: %s", line1);
    ESP_LOGI(TAG, "OLED 第二行: %s", line2);
    // 注意：实际显示需要按 SSD1306 协议写入指令和数据
}
