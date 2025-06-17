#ifndef WIFI_PROVISION_H
#define WIFI_PROVISION_H

/**
 * @brief 启动 Wi-Fi 配置模式（SoftAP + Web 表单）
 *        设备将创建开放热点并启动 HTTP 服务器，
 *        接收手机通过网页提交的 SSID 和密码，
 *        配置完成后自动重启。
 */
void wifi_prov_start(void);

/**
 * @brief 从 NVS 读取 WiFi 配置并连接
 *        统一的 WiFi Station 模式初始化和连接函数
 */
void wifi_prov_connect_from_nvs(void);

#endif // WIFI_PROVISION_H
