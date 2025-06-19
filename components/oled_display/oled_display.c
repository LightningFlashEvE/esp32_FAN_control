#include "oled_display.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char* TAG = "OLED";

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

    // TODO: 需用官方 driver/i2c.h 自己实现SSD1306驱动，移除所有 ssd1306 相关API和变量。
}

/**
 * @brief 更新 OLED 显示 - 使用SSD1306库的高级功能
 * @param temperature 当前温度值
 * @param speed 当前风扇速度百分比 (0-100)
 * @param auto_mode 当前模式：true=自动，false=手动
 */
void oled_display_update(float temperature, uint8_t speed, bool auto_mode) {
    // TODO: 需用官方 driver/i2c.h 自己实现SSD1306驱动，移除所有 ssd1306 相关API和变量。
}
