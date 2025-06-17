#include "mqtt_comm.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>
#include <stdio.h>

static const char* TAG = "MQTT";

// MQTT主题定义
#define MQTT_TOPIC_STATUS    "esp32/fan_control/status"      // 状态发布主题
#define MQTT_TOPIC_COMMAND   "esp32/fan_control/command"     // 命令订阅主题
#define MQTT_TOPIC_CONFIG    "esp32/fan_control/config"      // 配置订阅主题

// 前向声明
static void mqtt_handle_command(const char* data, int data_len);
static void mqtt_handle_config(const char* data, int data_len);

// MQTT事件处理器
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT已连接到服务器");
            // 订阅控制命令主题
            esp_mqtt_client_subscribe(client, MQTT_TOPIC_COMMAND, 1);
            esp_mqtt_client_subscribe(client, MQTT_TOPIC_CONFIG, 1);
            ESP_LOGI(TAG, "已订阅主题: %s 和 %s", MQTT_TOPIC_COMMAND, MQTT_TOPIC_CONFIG);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT已断开连接");
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT订阅成功, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT取消订阅, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT发布成功, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "收到MQTT消息:");
            ESP_LOGI(TAG, "主题: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "数据: %.*s", event->data_len, event->data);
            
            // 处理命令消息
            if (strncmp(event->topic, MQTT_TOPIC_COMMAND, event->topic_len) == 0) {
                mqtt_handle_command(event->data, event->data_len);
            }
            // 处理配置消息
            else if (strncmp(event->topic, MQTT_TOPIC_CONFIG, event->topic_len) == 0) {
                mqtt_handle_config(event->data, event->data_len);
            }
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT错误事件");
            break;
              default:
            ESP_LOGI(TAG, "其他MQTT事件, 事件ID: %ld", (long)event_id);
            break;
    }
}

/**
 * @brief 处理MQTT命令消息
 * @param data 命令数据
 * @param data_len 数据长度
 */
static void mqtt_handle_command(const char* data, int data_len) {
    // 简单的命令解析，查找关键字
    char command_str[256];
    if (data_len >= sizeof(command_str)) {
        ESP_LOGW(TAG, "命令数据过长");
        return;
    }
    
    memcpy(command_str, data, data_len);
    command_str[data_len] = '\0';
    
    ESP_LOGI(TAG, "处理命令: %s", command_str);
    
    // 检查设置速度命令
    if (strstr(command_str, "set_speed") != NULL) {
        // 简单解析速度值
        char *speed_str = strstr(command_str, "speed");
        if (speed_str != NULL) {
            int speed = 0;
            if (sscanf(speed_str, "speed\":%d", &speed) == 1) {
                ESP_LOGI(TAG, "收到设置速度命令: %d%%", speed);
                // TODO: 调用风扇控制函数设置速度
            }
        }
    }
    // 检查设置模式命令
    else if (strstr(command_str, "set_mode") != NULL) {
        if (strstr(command_str, "auto") != NULL) {
            ESP_LOGI(TAG, "收到设置模式命令: 自动模式");
            // TODO: 调用风扇控制函数设置自动模式
        } else if (strstr(command_str, "manual") != NULL) {
            ESP_LOGI(TAG, "收到设置模式命令: 手动模式");
            // TODO: 调用风扇控制函数设置手动模式
        }
    }
}

/**
 * @brief 处理MQTT配置消息
 * @param data 配置数据
 * @param data_len 数据长度
 */
static void mqtt_handle_config(const char* data, int data_len) {
    char config_str[256];
    if (data_len >= sizeof(config_str)) {
        ESP_LOGW(TAG, "配置数据过长");
        return;
    }
    
    memcpy(config_str, data, data_len);
    config_str[data_len] = '\0';
    
    ESP_LOGI(TAG, "处理配置: %s", config_str);
    
    // 解析温度阈值
    char *temp_str = strstr(config_str, "temp_threshold");
    if (temp_str != NULL) {
        float temp_threshold = 0.0;
        if (sscanf(temp_str, "temp_threshold\":%f", &temp_threshold) == 1) {
            ESP_LOGI(TAG, "收到温度阈值配置: %.1f°C", temp_threshold);
            // TODO: 保存温度阈值配置
        }
    }
    
    // 解析发布间隔
    char *interval_str = strstr(config_str, "publish_interval");
    if (interval_str != NULL) {
        int publish_interval = 0;
        if (sscanf(interval_str, "publish_interval\":%d", &publish_interval) == 1) {
            ESP_LOGI(TAG, "收到发布间隔配置: %d秒", publish_interval);
            // TODO: 保存发布间隔配置
        }
    }
}

/**
 * @brief 初始化 MQTT 客户端并返回句柄（不自动启动）
 * @return 已初始化的 MQTT 客户端句柄
 */
esp_mqtt_client_handle_t mqtt_comm_init(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = "mqtt://nas.phenosolar.com:1883",
            },
        },
        .credentials = {
            .username = "admin",
            .authentication = {
                .password = "admin",
            },
        },
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    
    // 注册MQTT事件处理器
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    
    ESP_LOGI(TAG, "MQTT 客户端已初始化");
    return client;
}

/**
 * @brief 发布风扇状态到 MQTT 主题
 * @param client MQTT 客户端句柄
 * @param temperature 当前温度 (°C)
 * @param speed 风扇速度百分比 (0-100)
 * @param auto_mode 当前模式：true=自动，false=手动
 */
void mqtt_comm_publish(esp_mqtt_client_handle_t client, float temperature, uint8_t speed, bool auto_mode) {
    if (!client) {
        ESP_LOGW(TAG, "MQTT 客户端句柄无效");
        return;
    }
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"temp\":%.1f,\"speed\":%d,\"mode\":\"%s\"}",
             temperature, speed, auto_mode ? "auto" : "manual");
    esp_mqtt_client_publish(client, MQTT_TOPIC_STATUS, payload, 0, 1, 0);
    ESP_LOGI(TAG, "已发布 MQTT: %s", payload);
}

/**
 * @brief 发布设备信息到 MQTT 主题
 * @param client MQTT 客户端句柄
 * @param device_id 设备ID
 * @param firmware_version 固件版本
 */
void mqtt_comm_publish_device_info(esp_mqtt_client_handle_t client, const char* device_id, const char* firmware_version) {
    if (!client) {
        ESP_LOGW(TAG, "MQTT 客户端句柄无效");
        return;
    }
      char payload[256];
    snprintf(payload, sizeof(payload), 
             "{\"device_id\":\"%s\",\"firmware\":\"%s\",\"status\":\"online\"}", 
             device_id, firmware_version);
    
    esp_mqtt_client_publish(client, "esp32/fan_control/device_info", payload, 0, 1, 0);
    ESP_LOGI(TAG, "已发布设备信息: %s", payload);
}
