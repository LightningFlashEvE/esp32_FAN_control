#include "oled_display.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include <stdio.h>
#include <string.h>

// 抑制旧版I2C驱动警告
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// static const char* TAG = "OLED";  // 暂时未使用，注释掉避免警告

// 声明手动模式下制冷片功率变量（由main.c定义）
extern uint8_t manual_cooler_power;

#define SSD1306_I2C_ADDR 0x3C
#define SSD1306_WIDTH    128
#define SSD1306_HEIGHT   64
#define SSD1306_PAGE_COUNT (SSD1306_HEIGHT/8)

static i2c_port_t s_i2c_num;

// 5x7 ASCII 字体表（仅部分，完整可扩展）
static const uint8_t font5x7[][5] = {
    // ... 只示例部分 ...
    {0x00,0x00,0x00,0x00,0x00}, // 空格 0x20
    {0x00,0x00,0x5F,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00}, // "
    // ... 可补充完整 ...
};

// I2C 写命令
static esp_err_t ssd1306_write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return i2c_master_write_to_device(s_i2c_num, SSD1306_I2C_ADDR, buf, 2, 100 / portTICK_PERIOD_MS);
}
// I2C 写数据
static esp_err_t ssd1306_write_data(const uint8_t* data, size_t len) {
    uint8_t buf[SSD1306_WIDTH+1];
    buf[0] = 0x40;
    memcpy(&buf[1], data, len);
    return i2c_master_write_to_device(s_i2c_num, SSD1306_I2C_ADDR, buf, len+1, 100 / portTICK_PERIOD_MS);
}
// 初始化 SSD1306
void oled_init(i2c_port_t i2c_num, gpio_num_t sda_pin, gpio_num_t scl_pin) {
    s_i2c_num = i2c_num;
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl_pin,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    ESP_ERROR_CHECK(i2c_param_config(i2c_num, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(i2c_num, I2C_MODE_MASTER, 0, 0, 0));
    // SSD1306 初始化命令序列（简化版）
    ssd1306_write_cmd(0xAE); // 关闭显示
    ssd1306_write_cmd(0x20); ssd1306_write_cmd(0x00); // 水平寻址
    ssd1306_write_cmd(0xB0); // page0
    ssd1306_write_cmd(0xC8); // COM扫描方向
    ssd1306_write_cmd(0x00); // 低列地址
    ssd1306_write_cmd(0x10); // 高列地址
    ssd1306_write_cmd(0x40); // 起始行
    ssd1306_write_cmd(0x81); ssd1306_write_cmd(0x7F); // 对比度
    ssd1306_write_cmd(0xA1); // 段重定向
    ssd1306_write_cmd(0xA6); // 正常显示
    ssd1306_write_cmd(0xA8); ssd1306_write_cmd(0x3F); // 多路复用
    ssd1306_write_cmd(0xA4); // 全局显示开启
    ssd1306_write_cmd(0xD3); ssd1306_write_cmd(0x00); // 显示偏移
    ssd1306_write_cmd(0xD5); ssd1306_write_cmd(0x80); // 时钟分频
    ssd1306_write_cmd(0xD9); ssd1306_write_cmd(0xF1); // 预充电
    ssd1306_write_cmd(0xDA); ssd1306_write_cmd(0x12); // COM引脚
    ssd1306_write_cmd(0xDB); ssd1306_write_cmd(0x40); // VCOMH
    ssd1306_write_cmd(0x8D); ssd1306_write_cmd(0x14); // 电荷泵
    ssd1306_write_cmd(0xAF); // 开启显示
}
// 清屏
static void ssd1306_clear(void) {
    uint8_t zero[SSD1306_WIDTH] = {0};
    for (uint8_t page = 0; page < SSD1306_PAGE_COUNT; ++page) {
        ssd1306_write_cmd(0xB0 | page);
        ssd1306_write_cmd(0x00);
        ssd1306_write_cmd(0x10);
        ssd1306_write_data(zero, SSD1306_WIDTH);
    }
}
// 显示ASCII字符串（单行，x:0-127, y:0/8/16...63）
static void ssd1306_draw_str(uint8_t x, uint8_t page, const char* str) {
    ssd1306_write_cmd(0xB0 | page);
    ssd1306_write_cmd(0x00 | (x & 0x0F));
    ssd1306_write_cmd(0x10 | (x >> 4));
    while (*str && x < SSD1306_WIDTH-6) {
        char c = *str++;
        if (c < 32 || c > 126) c = '?';
        ssd1306_write_data(font5x7[c-32], 5);
        uint8_t space = 0x00;
        ssd1306_write_data(&space, 1); // 字符间隔
        x += 6;
    }
}
// 刷新显示内容（本实现为直接写入，无需额外刷新）

void oled_display_update(float temperature, uint8_t fan_speed, bool auto_mode) {
    char line1[22], line2[22], line3[22], line4[22];
    snprintf(line1, sizeof(line1), "Temp : %5.1f °C", temperature);
    snprintf(line2, sizeof(line2), "Fan  : %3d%% %s", fan_speed, auto_mode ? "(Auto) " : "      ");
    if (auto_mode) {
        snprintf(line3, sizeof(line3), "Cooler: %3d%% (Auto) ", fan_speed);
        snprintf(line4, sizeof(line4), "Mode : 自动       ");
    } else {
        snprintf(line3, sizeof(line3), "Cooler: %3d%% (Manual)", manual_cooler_power);
        snprintf(line4, sizeof(line4), "Mode : 手动       ");
    }
    ssd1306_clear();
    ssd1306_draw_str(0, 0, line1);
    ssd1306_draw_str(0, 1, line2);
    ssd1306_draw_str(0, 2, line3);
    ssd1306_draw_str(0, 3, line4);
}

#pragma GCC diagnostic pop  // 恢复警告设置
