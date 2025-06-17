#ifndef USER_INPUT_H
#define USER_INPUT_H

#include "driver/gpio.h"
#include <stdint.h>
#include <stdbool.h>

// 模式切换回调类型：当切换自动/手动模式时调用
typedef void (*mode_change_cb_t)(bool auto_mode);
// 速度改变回调类型：当手动模式下旋转编码器调速时调用
typedef void (*speed_change_cb_t)(uint8_t new_speed);

/**
 * @brief 初始化 EC11 旋转编码器输入
 * @param pin_a GPIO 引脚，连接编码器 A 相
 * @param pin_b GPIO 引脚，连接编码器 B 相
 * @param pin_btn GPIO 引脚，连接编码器按键
 * @param mode_cb 模式切换回调函数
 * @param speed_cb 速度调整回调函数
 */
void user_input_init(gpio_num_t pin_a, gpio_num_t pin_b, gpio_num_t pin_btn,
                     mode_change_cb_t mode_cb, speed_change_cb_t speed_cb);

#endif // USER_INPUT_H
