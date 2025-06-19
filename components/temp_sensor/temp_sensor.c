#include "temp_sensor.h"
#include "esp_log.h"

// static const char* TAG = "TEMP_SENSOR";  // 暂时未使用，注释掉避免警告

/**
 * @brief 初始化DS18B20温度传感器 - 使用官方DS18B20库
 * @param gpio_pin 数据引脚GPIO
 */
void temp_sensor_init(gpio_num_t gpio_pin) {
    // TODO: 需用官方 driver/gpio.h 自己实现1-Wire协议和DS18B20温度读取逻辑，移除所有 onewire_bus/ds18b20 相关API和变量。
}

/**
 * @brief 获取温度值 - 使用官方DS18B20库
 * @return 温度值（摄氏度），错误时返回-127.0
 */
float temp_sensor_get_temperature(void) {
    // TODO: 需用官方 driver/gpio.h 自己实现1-Wire协议和DS18B20温度读取逻辑，移除所有 onewire_bus/ds18b20 相关API和变量。
    return 0.0;
}
