#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "wifi_provision.h"

#include "temp_sensor.h"
#include "fan_control.h"
#include "user_input.h"
#include "oled_display.h"
#include "mqtt_comm.h"
#include <mqtt_client.h> // 添加 MQTT 客户端类型定义

static const char *TAG = "MAIN";

// 定义引脚和通道
#define DS18B20_GPIO       GPIO_NUM_4
#define ENCODER_A_GPIO     GPIO_NUM_15
#define ENCODER_B_GPIO     GPIO_NUM_2
#define ENCODER_BTN_GPIO   GPIO_NUM_0
#define FAN_PWM_GPIO       GPIO_NUM_18
#define I2C_SDA_GPIO       GPIO_NUM_21
#define I2C_SCL_GPIO       GPIO_NUM_22
#define LEDC_CHANNEL       LEDC_CHANNEL_0

// 模式和速度状态
static bool auto_mode = true;
static uint8_t manual_speed = 0;
static esp_mqtt_client_handle_t mqtt_client = NULL;  // MQTT 客户端句柄

/**
 * @brief 回调：模式切换
 */
static void on_mode_change(bool mode) {
    auto_mode = mode;
    // 更新屏幕和发布MQTT
    oled_display_update(0, manual_speed, auto_mode);
    mqtt_comm_publish(mqtt_client, 0, manual_speed, auto_mode);
}

/**
 * @brief 回调：手动调速
 */
static void on_speed_change(uint8_t speed) {
    manual_speed = speed;
    if (!auto_mode) {
        fan_control_set_speed(manual_speed);
    }
    oled_display_update(0, manual_speed, auto_mode);
    mqtt_comm_publish(mqtt_client, 0, manual_speed, auto_mode);
}

/**
 * @brief 温度映射到风扇转速
 */
static uint8_t map_temp_to_speed(float temp) {
    if (temp <= 25.0) return 0;
    if (temp <= 30.0) return 50;
    return 100;
}

/**
 * @brief 自动模式控制任务
 */
static void auto_task(void *arg) {
    while (1) {
        if (auto_mode) {
            float temp = temp_sensor_get_temperature();
            uint8_t spd = map_temp_to_speed(temp);
            fan_control_set_speed(spd);
            oled_display_update(temp, spd, auto_mode);
            mqtt_comm_publish(mqtt_client, temp, spd, auto_mode);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief IP 获取回调：在获取 IP 后启动 MQTT
 */
static void on_got_ip(void* arg, esp_event_base_t event_base,
                      int32_t event_id, void* event_data) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "获取到 IP: " IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGI(TAG, "启动 MQTT 客户端");
    if (mqtt_client) {
        esp_mqtt_client_start(mqtt_client);
    } else {
        ESP_LOGE(TAG, "MQTT 客户端句柄为空！");
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32 Fan Control Project Start");
    
    // 1. 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    // 2. 初始化 TCP/IP 和事件循环
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // 3. 检查WiFi配置是否存在
    nvs_handle_t nvs_handle;
    ret = nvs_open("storage", NVS_READONLY, &nvs_handle);
    
    size_t ssid_len = 0;
    bool wifi_configured = false;
    
    if (ret == ESP_OK) {
        esp_err_t err = nvs_get_str(nvs_handle, "wifi_ssid", NULL, &ssid_len);
        if (err == ESP_OK && ssid_len > 0) {
            wifi_configured = true;
        }
        nvs_close(nvs_handle);
    }
    
    if (!wifi_configured) {
        // WiFi未配置，启动配置模式
        ESP_LOGI(TAG, "WiFi未配置，启动配置模式");
        wifi_prov_start();
        return; // 配置完成后会重启
    }    // 4. WiFi已配置，启动正常模式
    ESP_LOGI(TAG, "WiFi已配置，启动正常模式");
    
    // 注册 IP 事件（在启动WiFi之前注册）
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               on_got_ip, NULL));
    
    // 启动WiFi Station模式（使用wifi_provision模块统一管理）
    wifi_prov_connect_from_nvs();

    // 5. 初始化各组件
    temp_sensor_init(DS18B20_GPIO);
    fan_control_init(LEDC_CHANNEL, FAN_PWM_GPIO);
    oled_init(I2C_NUM_0, I2C_SDA_GPIO, I2C_SCL_GPIO);
    mqtt_client = mqtt_comm_init();  // 初始化MQTT客户端
    user_input_init(ENCODER_A_GPIO, ENCODER_B_GPIO, ENCODER_BTN_GPIO,
                    on_mode_change, on_speed_change);
    
    // 6. 启动自动模式任务
    xTaskCreate(auto_task, "auto_task", 4096, NULL, 5, NULL);
    
    // 7. 主循环
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
