#ifndef FAN_CONTROL_H
#define FAN_CONTROL_H

#include "driver/ledc.h"
#include "driver/gpio.h"

/**
 * @brief 初始化制冷片 PWM 控制
 * @param channel LEDC 通道，用于输出 PWM
 * @param pin GPIO 引脚，连接至MOS管
 */
void cooler_pwm_init(ledc_channel_t channel, gpio_num_t pin);

/**
 * @brief 设置制冷片功率
 * @param power 百分比 (0-100)
 */
void cooler_pwm_set_power(uint8_t power);

/**
 * @brief 初始化风扇 PWM 控制
 * @param channel LEDC 通道，用于输出 PWM
 * @param pin GPIO 引脚，连接至风扇的 PWM 输入
 */
void fan_pwm_init(ledc_channel_t channel, gpio_num_t pin);

/**
 * @brief 设置风扇转速
 * @param speed 转速百分比 (0-100)
 */
void fan_pwm_set_speed(uint8_t speed);

#endif // FAN_CONTROL_H
