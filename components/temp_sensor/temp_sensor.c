#include "temp_sensor.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "TEMP_SENSOR";
static gpio_num_t sensor_pin; // DS18B20 数据引脚

// ---------- One-Wire 协议底层操作 ----------
/**
 * @brief 拉低总线电平，发送复位或写时脉冲
 */
static void ow_drive_low() {
    gpio_set_direction(sensor_pin, GPIO_MODE_OUTPUT_OD); // 开漏输出
    gpio_set_level(sensor_pin, 0);                      // 拉低电平
}

/**
 * @brief 释放总线，总线被外部上拉
 */
static void ow_release() {
    gpio_set_level(sensor_pin, 1);                     // 输出高电平
    gpio_set_direction(sensor_pin, GPIO_MODE_INPUT_OUTPUT_OD); // 切换为开漏输入
}

/**
 * @brief 发送复位脉冲并检测设备存在
 * @return true: 有 DS18B20 响应，false: 无响应
 */
static bool ow_reset() {
    ow_drive_low();        // 拉低总线 480us
    ets_delay_us(480);
    ow_release();          // 释放总线
    ets_delay_us(70);
    int presence = gpio_get_level(sensor_pin); // 读取存在脉冲
    ets_delay_us(410);
    return (presence == 0);
}

/**
 * @brief 往总线写入单个位数据
 * @param bit 位值 (0 或 1)
 */
static void ow_write_bit(uint8_t bit) {
    if (bit) {
        ow_drive_low();    // 写 '1' 时，先拉低 6us
        ets_delay_us(6);
        ow_release();      // 再释放，总线拉高
        ets_delay_us(64);
    } else {
        ow_drive_low();    // 写 '0' 时，保持拉低 60us
        ets_delay_us(60);
        ow_release();      // 再释放
        ets_delay_us(10);
    }
}

/**
 * @brief 从总线读取单个位数据
 * @return 读取到的位值 (0 或 1)
 */
static uint8_t ow_read_bit() {
    ow_drive_low();        // 主机主导读取时序
    ets_delay_us(6);
    ow_release();          // 释放总线
    ets_delay_us(9);
    int bit = gpio_get_level(sensor_pin); // 采样数据
    ets_delay_us(55);
    return bit;
}

/**
 * @brief 写入一个字节到总线
 * @param byte 要写入的字节
 */
static void ow_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        ow_write_bit(byte & 0x01);
        byte >>= 1;
    }
}

/**
 * @brief 从总线读取一个字节
 * @return 读取到的字节
 */
static uint8_t ow_read_byte() {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte |= (ow_read_bit() << i);
    }
    return byte;
}

// ---------- 对外接口实现 ----------
/**
 * @brief 初始化 DS18B20 传感器
 * @param pin 连接到 DS18B20 数据线的 GPIO 引脚
 */
void temp_sensor_init(gpio_num_t pin) {
    sensor_pin = pin;
    gpio_reset_pin(sensor_pin); // 重置并配置为普通 GPIO
    gpio_set_direction(sensor_pin, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(sensor_pin, GPIO_PULLUP_ONLY);
    ESP_LOGI(TAG, "DS18B20 已初始化，GPIO: %d", pin);
}

/**
 * @brief 获取当前温度值
 * @return 摄氏度温度值，读取失败返回 -127.0
 */
float temp_sensor_get_temperature(void) {
    if (!ow_reset()) {
        ESP_LOGE(TAG, "未检测到 DS18B20 响应");
        return -127.0;
    }
    ow_write_byte(0xCC); // 跳过 ROM 选择
    ow_write_byte(0x44); // 启动温度转换
    vTaskDelay(pdMS_TO_TICKS(750)); // 等待转换完成

    ow_reset();
    ow_write_byte(0xCC);
    ow_write_byte(0xBE); // 读取暂存器
    uint8_t lsb = ow_read_byte();
    uint8_t msb = ow_read_byte();
    int16_t raw = (msb << 8) | lsb;
    float temp = raw * 0.0625f; // 每位代表 0.0625°C
    ESP_LOGI(TAG, "读取温度: %.2f°C", temp);
    return temp;
}
