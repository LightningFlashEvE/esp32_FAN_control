#include "fan_control.h"
#include "esp_err.h"

// 用于保存配置的 LEDC 通道
static ledc_channel_t fan_channel;

/**
 * @brief 初始化风扇 PWM 控制
 * @param channel LEDC 通道，用于输出 PWM 信号
 * @param pin GPIO 引脚，连接至风扇的 PWM 输入管脚
 */
void fan_control_init(ledc_channel_t channel, gpio_num_t pin) {
    fan_channel = channel;
    // 配置 LEDC 定时器参数
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,  // 高速模式
        .timer_num        = LEDC_TIMER_0,          // 定时器 0
        .duty_resolution  = LEDC_TIMER_8_BIT,      // 8 位分辨率
        .freq_hz          = 25000,                 // 25 kHz PWM 频率
        .clk_cfg          = LEDC_AUTO_CLK,         // 自动时钟
    };
    ledc_timer_config(&ledc_timer);

    // 配置 LEDC 通道参数
    ledc_channel_config_t ledc_channel_conf = {
        .gpio_num       = pin,                     // PWM 输出管脚
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = channel,                 // 使用的通道
        .intr_type      = LEDC_INTR_DISABLE,       // 关闭中断
        .timer_sel      = LEDC_TIMER_0,            // 定时器 0
        .duty           = 0,                       // 初始占空比 0
    };
    ledc_channel_config(&ledc_channel_conf);
}

/**
 * @brief 设置风扇转速
 * @param speed 转速百分比 (0-100)
 */
void fan_control_set_speed(uint8_t speed) {
    // 限制范围
    if (speed > 100) speed = 100;
    // 将百分比映射到 8 位占空比值
    uint32_t duty = (speed * ((1 << 8) - 1)) / 100;
    // 设置占空比并更新
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, fan_channel, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, fan_channel);
}
