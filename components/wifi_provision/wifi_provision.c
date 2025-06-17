#include "wifi_provision.h"
#include <string.h>
#include <ctype.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_http_server.h"

static const char *TAG = "WIFI_PROV";
static bool prov_done = false;

/**
 * @brief 处理根路径，显示WiFi配置页面
 */
static esp_err_t root_get_handler(httpd_req_t *req) {
    const char *html_page = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset='UTF-8'>"
        "<title>ESP32 WiFi配置</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; background-color: #f0f0f0; }"
        ".container { max-width: 400px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }"
        "h1 { color: #333; text-align: center; }"
        "form { margin-top: 20px; }"
        "label { display: block; margin-top: 15px; font-weight: bold; color: #555; }"
        "input[type=text], input[type=password] { width: 100%; padding: 10px; margin-top: 5px; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }"
        "input[type=submit] { width: 100%; padding: 12px; margin-top: 20px; background: #007cba; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }"
        "input[type=submit]:hover { background: #005a85; }"
        ".info { background: #e7f3ff; padding: 15px; border-radius: 5px; margin-bottom: 20px; border-left: 4px solid #007cba; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h1>ESP32 风扇控制器</h1>"
        "<div class='info'>"
        "<strong>WiFi配置</strong><br>"
        "请输入您的WiFi网络信息，设备将连接到指定网络。"
        "</div>"
        "<form action='/configure' method='post'>"
        "<label for='ssid'>WiFi名称 (SSID):</label>"
        "<input type='text' id='ssid' name='ssid' required maxlength='31' placeholder='请输入WiFi名称'>"
        "<label for='password'>WiFi密码:</label>"
        "<input type='password' id='password' name='password' maxlength='63' placeholder='请输入WiFi密码'>"
        "<input type='submit' value='保存配置'>"
        "</form>"
        "</div>"
        "</body>"
        "</html>";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page, strlen(html_page));
    return ESP_OK;
}

/**
 * @brief URL解码函数
 */
static void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

/**
 * @brief 处理配置页面表单提交，接收 SSID 和密码
 */
static esp_err_t config_post_handler(httpd_req_t *req) {
    char buf[256];
    int len = httpd_req_recv(req, buf, req->content_len);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid Request");
        return ESP_FAIL;
    }
    buf[len] = '\0';
    
    char ssid[32] = {0}, password[64] = {0};
    char ssid_encoded[64] = {0}, password_encoded[128] = {0};
    
    // 解析表单数据
    char *ssid_start = strstr(buf, "ssid=");
    char *password_start = strstr(buf, "password=");
    
    if (ssid_start) {
        ssid_start += 5; // 跳过"ssid="
        char *ssid_end = strchr(ssid_start, '&');
        if (ssid_end) {
            strncpy(ssid_encoded, ssid_start, ssid_end - ssid_start);
        } else {
            strncpy(ssid_encoded, ssid_start, sizeof(ssid_encoded)-1);
        }
        url_decode(ssid, ssid_encoded);
    }
    
    if (password_start) {
        password_start += 9; // 跳过"password="
        strncpy(password_encoded, password_start, sizeof(password_encoded)-1);
        url_decode(password, password_encoded);
    }
    
    ESP_LOGI(TAG, "收到配置：SSID=%s, PASS=%s", ssid, password);    // 保存WiFi配置到NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        nvs_set_str(nvs_handle, "wifi_ssid", ssid);
        nvs_set_str(nvs_handle, "wifi_password", password);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "WiFi配置已保存到NVS");
    } else {
        ESP_LOGE(TAG, "打开NVS失败");
    }

    const char *resp = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset='UTF-8'>"
        "<title>配置完成</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; background-color: #f0f0f0; text-align: center; }"
        ".container { max-width: 400px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }"
        "h1 { color: #28a745; }"
        ".info { background: #d4edda; padding: 15px; border-radius: 5px; border-left: 4px solid #28a745; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h1>✓ 配置完成</h1>"
        "<div class='info'>"
        "WiFi配置已保存，设备正在重启...<br>"
        "请稍候片刻，设备将连接到您的WiFi网络。"
        "</div>"
        "</div>"
        "</body>"
        "</html>";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp, strlen(resp));

    prov_done = true;
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

/**
 * @brief 启动 HTTP 服务并注册 URI
 */
static void start_http_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    httpd_handle_t server = NULL;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // 注册根路径，显示配置页面
        httpd_uri_t root_uri = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = root_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root_uri);
        
        // 注册配置提交路径
        httpd_uri_t config_uri = {
            .uri      = "/configure",
            .method   = HTTP_POST,
            .handler  = config_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &config_uri);
        
        ESP_LOGI(TAG, "HTTP 服务启动，端口 80");
        ESP_LOGI(TAG, "配置页面：http://192.168.4.1/");
    } else {
        ESP_LOGE(TAG, "HTTP 服务启动失败");
    }
}

void wifi_prov_start(void) {
    if (prov_done) return;
    ESP_LOGI(TAG, "开始 Wi-Fi 配置模式");
    
    // 初始化并启动 SoftAP
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
    
    wifi_config_t ap_cfg = {
        .ap = {
            .ssid = "ESP32_Config",
            .channel = 1,
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 4
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "SoftAP 已启动，SSID: ESP32_Config");
    
    // 启动 WEB 配置
    start_http_server();
}

/**
 * @brief 从 NVS 读取 WiFi 配置并连接 Station 模式
 */
void wifi_prov_connect_from_nvs(void) {
    ESP_LOGI(TAG, "从 NVS 读取 WiFi 配置并连接");
    
    // 读取WiFi配置
    nvs_handle_t nvs_handle;
    char ssid[32] = {0};
    char password[64] = {0};
    
    esp_err_t ret = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (ret == ESP_OK) {
        size_t ssid_len = sizeof(ssid);
        size_t password_len = sizeof(password);
        nvs_get_str(nvs_handle, "wifi_ssid", ssid, &ssid_len);
        nvs_get_str(nvs_handle, "wifi_password", password, &password_len);
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "无法打开 NVS 存储");
        return;
    }
    
    // 初始化WiFi Station模式
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // 配置WiFi连接参数
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid)-1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password)-1);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    ESP_LOGI(TAG, "WiFi Station 模式已启动，正在连接到: %s", ssid);
}
