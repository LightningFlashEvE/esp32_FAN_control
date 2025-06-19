#ifndef MQTT_COMM_H
#define MQTT_COMM_H

#include "mqtt_client.h"  // ESP MQTT 客户端类型
#include <stdbool.h>
#include <stdint.h>

// 命令结构体
typedef enum {
    MQTT_MODE_AUTO,
    MQTT_MODE_MANUAL
} mqtt_mode_t;

typedef struct {
    uint8_t speed;
    mqtt_mode_t mode;
    bool has_speed;
    bool has_mode;
} mqtt_command_t;

// 配置结构体
typedef struct {
    float temp_threshold;
    uint8_t max_speed;
    bool has_temp_threshold;
    bool has_max_speed;
} mqtt_config_t;

// 回调函数类型定义
typedef void (*mqtt_command_callback_t)(const mqtt_command_t* cmd);
typedef void (*mqtt_config_callback_t)(const mqtt_config_t* cfg);

/**
 * @brief 初始化 MQTT 客户端并返回句柄（不自动启动）
 * @return 已初始化但未启动的 MQTT 客户端句柄
 */
esp_mqtt_client_handle_t mqtt_comm_init(void);

/**
 * @brief 发布风扇状态到 MQTT 主题
 * @param client MQTT 客户端句柄
 * @param temperature 当前温度值，单位摄氏度
 * @param speed      当前风扇速度百分比 (0-100)
 * @param auto_mode  当前模式标志：true=自动模式，false=手动模式
 */
void mqtt_comm_publish(esp_mqtt_client_handle_t client, float temperature, uint8_t speed, bool auto_mode);

/**
 * @brief 发布设备信息到 MQTT 主题
 * @param client MQTT 客户端句柄
 * @param device_id 设备ID
 * @param firmware_version 固件版本
 */
void mqtt_comm_publish_device_info(esp_mqtt_client_handle_t client, const char* device_id, const char* firmware_version);

/**
 * @brief 设置命令回调函数
 * @param callback 命令回调函数指针
 */
void mqtt_comm_set_command_callback(mqtt_command_callback_t callback);

/**
 * @brief 设置配置回调函数
 * @param callback 配置回调函数指针
 */
void mqtt_comm_set_config_callback(mqtt_config_callback_t callback);

#endif // MQTT_COMM_H
