#include "mqtt_comm.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cjson.h"
#include <string.h>
#include <stdio.h>

static const char* TAG = "MQTT";

// MQTT主题定义
#define MQTT_TOPIC_STATUS    "esp32/fan_control/status"
#define MQTT_TOPIC_COMMAND   "esp32/fan_control/command"
#define MQTT_TOPIC_CONFIG    "esp32/fan_control/config"

// 回调函数指针
static mqtt_command_callback_t command_callback = NULL;
static mqtt_config_callback_t config_callback = NULL;

// 前向声明
static void mqtt_handle_command(const char* data, int data_len);
static void mqtt_handle_config(const char* data, int data_len);

/**
 * @brief 处理MQTT命令消息 - 使用cJSON解析
 */
static void mqtt_handle_command(const char* data, int data_len) {
    char* json_string = malloc(data_len + 1);
    if (!json_string) {
        ESP_LOGE(TAG, "内存分配失败");
        return;
    }
    
    memcpy(json_string, data, data_len);
    json_string[data_len] = '\0';
    
    // 使用cJSON解析
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        ESP_LOGE(TAG, "JSON解析失败: %s", json_string);
        free(json_string);
        return;
    }
    
    // 解析命令参数
    cJSON *speed_item = cJSON_GetObjectItem(json, "speed");
    cJSON *mode_item = cJSON_GetObjectItem(json, "mode");
    
    mqtt_command_t cmd = {0};
    
    if (cJSON_IsNumber(speed_item)) {
        cmd.speed = speed_item->valueint;
        cmd.has_speed = true;
        ESP_LOGI(TAG, "解析到速度命令: %d%%", cmd.speed);
    }
    
    if (cJSON_IsString(mode_item)) {
        if (strcmp(mode_item->valuestring, "auto") == 0) {
            cmd.mode = MQTT_MODE_AUTO;
            cmd.has_mode = true;
        } else if (strcmp(mode_item->valuestring, "manual") == 0) {
            cmd.mode = MQTT_MODE_MANUAL;
            cmd.has_mode = true;
        }
        ESP_LOGI(TAG, "解析到模式命令: %s", mode_item->valuestring);
    }
    
    // 调用回调函数处理命令
    if (command_callback) {
        command_callback(&cmd);
    }
    
    cJSON_Delete(json);
    free(json_string);
}

/**
 * @brief 处理MQTT配置消息 - 使用cJSON解析
 */
static void mqtt_handle_config(const char* data, int data_len) {
    char* json_string = malloc(data_len + 1);
    if (!json_string) {
        ESP_LOGE(TAG, "内存分配失败");
        return;
    }
    
    memcpy(json_string, data, data_len);
    json_string[data_len] = '\0';
    
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        ESP_LOGE(TAG, "JSON解析失败: %s", json_string);
        free(json_string);
        return;
    }
    
    mqtt_config_t cfg = {0};
    
    cJSON *temp_threshold = cJSON_GetObjectItem(json, "temp_threshold");
    if (cJSON_IsNumber(temp_threshold)) {
        cfg.temp_threshold = temp_threshold->valuedouble;
        cfg.has_temp_threshold = true;
    }
    
    cJSON *max_speed = cJSON_GetObjectItem(json, "max_speed");
    if (cJSON_IsNumber(max_speed)) {
        cfg.max_speed = max_speed->valueint;
        cfg.has_max_speed = true;
    }
    
    if (config_callback) {
        config_callback(&cfg);
    }
    
    cJSON_Delete(json);
    free(json_string);
}

// MQTT事件处理器
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT已连接");
            esp_mqtt_client_subscribe(client, MQTT_TOPIC_COMMAND, 1);
            esp_mqtt_client_subscribe(client, MQTT_TOPIC_CONFIG, 1);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT已断开连接");
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "收到MQTT消息: %.*s", event->topic_len, event->topic);
            
            if (strncmp(event->topic, MQTT_TOPIC_COMMAND, event->topic_len) == 0) {
                mqtt_handle_command(event->data, event->data_len);
            } else if (strncmp(event->topic, MQTT_TOPIC_CONFIG, event->topic_len) == 0) {
                mqtt_handle_config(event->data, event->data_len);
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief 初始化MQTT客户端
 */
esp_mqtt_client_handle_t mqtt_comm_init(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://nas.phenosolar.com:1883",
        .credentials.username = "admin",
        .credentials.authentication.password = "admin",
        .session.keepalive = 60,
        .session.disable_clean_session = false,
        .network.refresh_connection_after_lost = true,
        .network.reconnect_timeout_ms = 5000,
        .network.timeout_ms = 10000,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "MQTT客户端初始化失败");
        return NULL;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_LOGI(TAG, "MQTT客户端初始化完成");
    
    return client;
}

/**
 * @brief 发布状态信息 - 使用cJSON生成格式化JSON
 */
void mqtt_comm_publish(esp_mqtt_client_handle_t client, float temperature, uint8_t speed, bool auto_mode) {
    if (!client) return;
    
    // 使用cJSON构建JSON消息
    cJSON *json = cJSON_CreateObject();
    cJSON *temp_item = cJSON_CreateNumber(temperature);
    cJSON *speed_item = cJSON_CreateNumber(speed);
    cJSON *mode_item = cJSON_CreateString(auto_mode ? "auto" : "manual");
    
    cJSON_AddItemToObject(json, "temp", temp_item);
    cJSON_AddItemToObject(json, "speed", speed_item);
    cJSON_AddItemToObject(json, "mode", mode_item);
    
    char *json_string = cJSON_Print(json);
    if (json_string) {
        esp_mqtt_client_publish(client, MQTT_TOPIC_STATUS, json_string, 0, 1, 0);
        ESP_LOGI(TAG, "发布状态: %s", json_string);
        free(json_string);
    }
    
    cJSON_Delete(json);
}

/**
 * @brief 设置命令回调函数
 */
void mqtt_comm_set_command_callback(mqtt_command_callback_t callback) {
    command_callback = callback;
}

/**
 * @brief 设置配置回调函数
 */
void mqtt_comm_set_config_callback(mqtt_config_callback_t callback) {
    config_callback = callback;
}
