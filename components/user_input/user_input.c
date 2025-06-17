#include "user_input.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char* TAG = "USER_INPUT"; // 日志标签
static mode_change_cb_t mode_cb;         // 模式切换回调函数指针
static speed_change_cb_t speed_cb;       // 速度调整回调函数指针
static bool last_a_level;                // 上次 A 相信号电平
static uint8_t speed = 0;                // 当前风扇速度百分比
static bool auto_mode = true;            // 当前模式：true=自动，false=手动

// 保存引脚配置
static gpio_num_t gpio_pin_a;
static gpio_num_t gpio_pin_b;
static gpio_num_t gpio_pin_btn;

/**
 * @brief GPIO 中断处理函数
 *        处理 EC11 编码器 A/B 相和按钮按下事件
 * @param arg 触发中断的 GPIO 引脚编号
 */
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    if (gpio_num == gpio_pin_btn) {
        // 按键按下，切换自动/手动模式
        auto_mode = !auto_mode;
        mode_cb(auto_mode); // 调用模式切换回调
    } else if (gpio_num == gpio_pin_a) {
        // 编码器 A 相触发
        bool level = gpio_get_level(gpio_pin_a);
        // 上升沿触发有效
        if (last_a_level == 0 && level == 1) {
            if (gpio_get_level(gpio_pin_b)) {
                // 顺时针旋转，速度增加
                if (speed < 100) speed++;
            } else {
                // 逆时针旋转，速度减少
                if (speed > 0) speed--;
            }
            speed_cb(speed); // 调用速度调整回调
        }
        last_a_level = level;
    }
}

/**
 * @brief 初始化 EC11 编码器和按钮
 * @param pin_a 编码器 A 相 GPIO
 * @param pin_b 编码器 B 相 GPIO
 * @param pin_btn 编码器按键 GPIO
 * @param m_cb 模式切换回调
 * @param s_cb 速度调整回调
 */
void user_input_init(gpio_num_t pin_a, gpio_num_t pin_b, gpio_num_t pin_btn,
                     mode_change_cb_t m_cb, speed_change_cb_t s_cb) {
    // 保存引脚
    gpio_pin_a = pin_a;
    gpio_pin_b = pin_b;
    gpio_pin_btn = pin_btn;
    
    mode_cb = m_cb;   // 保存回调
    speed_cb = s_cb;

    // 配置 A/B 相为输入并启用上拉
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,     // 任意边沿触发中断
        .mode = GPIO_MODE_INPUT,            // 输入模式
        .pin_bit_mask = (1ULL<<pin_a) | (1ULL<<pin_b),
        .pull_up_en = GPIO_PULLUP_ENABLE,   // 启用上拉
    };
    gpio_config(&io_conf);

    // 配置按钮按键为下降沿触发输入
    gpio_config_t btn_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,     // 下降沿触发
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL<<pin_btn),
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&btn_conf);

    // 安装并启动 GPIO 中断服务
    gpio_install_isr_service(0);
    gpio_isr_handler_add(pin_a, gpio_isr_handler, (void*) pin_a);
    gpio_isr_handler_add(pin_b, gpio_isr_handler, (void*) pin_b);
    gpio_isr_handler_add(pin_btn, gpio_isr_handler, (void*) pin_btn);

    // 读取初始 A 相电平
    last_a_level = gpio_get_level(pin_a);
    ESP_LOGI(TAG, "User input initialized: A=%d B=%d BTN=%d", pin_a, pin_b, pin_btn);
}
