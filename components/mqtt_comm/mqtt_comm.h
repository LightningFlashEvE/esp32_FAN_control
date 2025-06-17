#ifndef MQTT_COMM_H
#define MQTT_COMM_H

#include "mqtt_client.h"  // ESP MQTT 客户端类型
#include <stdbool.h>
#include <stdint.h>

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

#endif // MQTT_COMM_H
