#include "oled_display.h"
#include "ssd1306.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char* TAG = "OLED";
static ssd1306_handle_t ssd1306_dev = NULL;

/**
 * @brief 初始化 OLED 显示屏 - 使用SSD1306专用库
 * @param i2c_num I2C 端口号
 * @param sda_pin SDA 引脚 GPIO
 * @param scl_pin SCL 引脚 GPIO
 */
void oled_init(i2c_port_t i2c_num, gpio_num_t sda_pin, gpio_num_t scl_pin) {
    // I2C配置
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

    // SSD1306设备配置
    ssd1306_config_t ssd1306_conf = {
        .i2c_port = i2c_num,
        .i2c_addr = 0x3C,
        .width = 128,
        .height = 64,
    };

    // 创建SSD1306设备
    ssd1306_dev = ssd1306_create(&ssd1306_conf);
    if (ssd1306_dev == NULL) {
        ESP_LOGE(TAG, "SSD1306设备创建失败");
        return;
    }

    // 初始化显示
    ESP_ERROR_CHECK(ssd1306_init(ssd1306_dev));
    ESP_ERROR_CHECK(ssd1306_clear_screen(ssd1306_dev, 0x00));
    ESP_ERROR_CHECK(ssd1306_refresh_gram(ssd1306_dev));
    
    ESP_LOGI(TAG, "SSD1306 OLED初始化完成");
}

/**
 * @brief 更新 OLED 显示 - 使用SSD1306库的高级功能
 * @param temperature 当前温度值
 * @param speed 当前风扇速度百分比 (0-100)
 * @param auto_mode 当前模式：true=自动，false=手动
 */
void oled_display_update(float temperature, uint8_t speed, bool auto_mode) {
    if (!ssd1306_dev) {
        ESP_LOGE(TAG, "SSD1306设备未初始化");
        return;
    }

    // 清除屏幕
    ssd1306_clear_screen(ssd1306_dev, 0x00);

    // 第一行：温度和模式
    char line1[32];
    snprintf(line1, sizeof(line1), "温度:%.1f°C", temperature);
    ssd1306_draw_string(ssd1306_dev, 0, 0, (const uint8_t*)line1, 12, 1);
    
    const char* mode_str = auto_mode ? "自动" : "手动";
    ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t*)mode_str, 12, 1);

    // 第二行：速度和进度条
    char speed_text[16];
    snprintf(speed_text, sizeof(speed_text), "速度:%d%%", speed);
    ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t*)speed_text, 12, 1);

    // 绘制进度条
    int bar_width = (speed * 100) / 100;  // 100像素宽度
    ssd1306_draw_rectangle(ssd1306_dev, 0, 48, 100, 16, 1);  // 外框
    if (bar_width > 0) {
        ssd1306_fill_rectangle(ssd1306_dev, 2, 50, bar_width-4, 12, 1);  // 填充
    }

    // 刷新显示
    ESP_ERROR_CHECK(ssd1306_refresh_gram(ssd1306_dev));
    
    ESP_LOGI(TAG, "OLED更新: 温度=%.1f°C, 速度=%d%%, 模式=%s", 
             temperature, speed, mode_str);
}
