#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

// 使用优化版的组件
#include "temp_sensor.h"     // 使用DS18B20库版本
#include "fan_control.h"
#include "user_input.h"      // 使用编码器和按钮库版本
#include "oled_display.h"    // 使用SSD1306库版本
#include "mqtt_comm.h"       // 使用cJSON版本
#include "wifi_provision.h"  // 自定义WiFi配网模块

// WiFi Provisioning - 使用ESP-IDF官方库
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"

static const char *TAG = "MAIN";

// GPIO定义
#define DS18B20_GPIO       GPIO_NUM_4
#define ENCODER_A_GPIO     GPIO_NUM_15
#define ENCODER_B_GPIO     GPIO_NUM_2
#define ENCODER_BTN_GPIO   GPIO_NUM_0
#define FAN_PWM_GPIO       GPIO_NUM_18
#define I2C_SDA_GPIO       GPIO_NUM_21
#define I2C_SCL_GPIO       GPIO_NUM_22
#define LEDC_CHANNEL       LEDC_CHANNEL_0

// 系统状态
typedef struct {
    bool auto_mode;
    uint8_t manual_speed;
    float temp_threshold;
    uint8_t max_speed;
    esp_mqtt_client_handle_t mqtt_client;
} system_state_t;

static system_state_t g_system = {
    .auto_mode = true,
    .manual_speed = 0,
    .temp_threshold = 30.0f,
    .max_speed = 100,
    .mqtt_client = NULL
};

/**
 * @brief 温度映射到风扇转速 - 可配置版本
 */
static uint8_t map_temp_to_speed(float temp) {
    if (temp <= 25.0) return 0;
    if (temp <= g_system.temp_threshold) return 50;
    return g_system.max_speed;
}

/**
 * @brief 模式切换回调
 */
static void on_mode_change(bool auto_mode) {
    g_system.auto_mode = auto_mode;
    ESP_LOGI(TAG, "模式切换为: %s", auto_mode ? "自动" : "手动");
    
    if (!auto_mode) {
        // 切换到手动模式时，设置为当前手动速度
        fan_control_set_speed(g_system.manual_speed);
    }
    
    // 立即更新显示和MQTT
    float temp = temp_sensor_get_temperature();
    uint8_t display_speed = auto_mode ? map_temp_to_speed(temp) : g_system.manual_speed;
    oled_display_update(temp, display_speed, auto_mode);
    mqtt_comm_publish(g_system.mqtt_client, temp, display_speed, auto_mode);
}

/**
 * @brief 速度调节回调
 */
static void on_speed_change(uint8_t speed) {
    g_system.manual_speed = speed;
    ESP_LOGI(TAG, "手动速度设置为: %d%%", speed);
    
    if (!g_system.auto_mode) {
        fan_control_set_speed(speed);
        // 更新显示
        float temp = temp_sensor_get_temperature();
        oled_display_update(temp, speed, false);
        mqtt_comm_publish(g_system.mqtt_client, temp, speed, false);
    }
}

/**
 * @brief MQTT命令处理回调
 */
static void on_mqtt_command(const mqtt_command_t* cmd) {
    ESP_LOGI(TAG, "收到MQTT命令");
    
    if (cmd->has_mode) {
        bool new_auto_mode = (cmd->mode == MQTT_MODE_AUTO);
        if (new_auto_mode != g_system.auto_mode) {
            on_mode_change(new_auto_mode);
        }
    }
    
    if (cmd->has_speed && !g_system.auto_mode) {
        if (cmd->speed <= 100) {
            on_speed_change(cmd->speed);
        }
    }
}

/**
 * @brief MQTT配置处理回调
 */
static void on_mqtt_config(const mqtt_config_t* cfg) {
    ESP_LOGI(TAG, "收到MQTT配置");
    
    if (cfg->has_temp_threshold) {
        g_system.temp_threshold = cfg->temp_threshold;
        ESP_LOGI(TAG, "温度阈值设置为: %.1f°C", g_system.temp_threshold);
    }
    
    if (cfg->has_max_speed) {
        g_system.max_speed = cfg->max_speed;
        ESP_LOGI(TAG, "最大速度设置为: %d%%", g_system.max_speed);
    }
}

/**
 * @brief 自动模式控制任务
 */
static void auto_control_task(void *arg) {
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        if (g_system.auto_mode) {
            float temp = temp_sensor_get_temperature();
            
            if (temp != -127.0) {  // 温度读取成功
                uint8_t target_speed = map_temp_to_speed(temp);
                fan_control_set_speed(target_speed);
                oled_display_update(temp, target_speed, true);
                mqtt_comm_publish(g_system.mqtt_client, temp, target_speed, true);
            } else {
                ESP_LOGW(TAG, "温度传感器读取失败");
                oled_display_update(-127.0, 0, true);
            }
        }
        
        // 精确5秒间隔
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief IP 获取回调：在获取 IP 后启动 MQTT
 */
static void on_got_ip(void* arg, esp_event_base_t event_base,
                      int32_t event_id, void* event_data) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;    ESP_LOGI(TAG, "获取到 IP: " IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGI(TAG, "启动 MQTT 客户端");
    if (g_system.mqtt_client) {
        esp_mqtt_client_start(g_system.mqtt_client);
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
    
    if (!wifi_configured) {        // WiFi未配置，启动配置模式
        ESP_LOGI(TAG, "WiFi未配置，启动配置模式");
        wifi_prov_start();
        return; // 配置完成后会重启
    }
    
    // 4. WiFi已配置，启动正常模式
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
    
    // 初始化 MQTT 客户端并设置回调，将句柄存储到全局结构体
    g_system.mqtt_client = mqtt_comm_init();
    mqtt_comm_set_command_callback(on_mqtt_command);
    mqtt_comm_set_config_callback(on_mqtt_config);
    
    user_input_init(ENCODER_A_GPIO, ENCODER_B_GPIO, ENCODER_BTN_GPIO,
                    on_mode_change, on_speed_change);
    
    // 6. 启动自动模式任务
    xTaskCreate(auto_control_task, "auto_control_task", 4096, NULL, 5, NULL);
    
    // 7. 主循环
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
