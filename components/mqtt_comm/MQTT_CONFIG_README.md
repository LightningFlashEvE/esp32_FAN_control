# MQTT 通信配置说明

## 服务器配置

你的MQTT服务器已配置为：
- 服务器地址：`nas.phenosolar.com:1883`
- 用户名：`admin`
- 密码：`admin`

## MQTT 主题说明

### 发布主题（设备发送数据）
- `esp32/fan_control/status` - 风扇状态信息
  - 消息格式：`{"temp":25.5,"speed":75,"mode":"auto"}`
- `esp32/fan_control/device_info` - 设备信息
  - 消息格式：`{"device_id":"ESP32_FAN_001","firmware":"v1.0.0","status":"online"}`

### 订阅主题（设备接收命令）
- `esp32/fan_control/command` - 控制命令
  - 设置速度：`{"command":"set_speed","speed":80}`
  - 设置模式：`{"command":"set_mode","mode":"auto"}` 或 `{"command":"set_mode","mode":"manual"}`
- `esp32/fan_control/config` - 配置参数
  - 温度阈值：`{"temp_threshold":30.0}`
  - 发布间隔：`{"publish_interval":60}`

## 使用示例

### 1. 启动MQTT客户端
```c
esp_mqtt_client_handle_t mqtt_client = mqtt_comm_init();
esp_mqtt_client_start(mqtt_client);
```

### 2. 发布风扇状态
```c
mqtt_comm_publish(mqtt_client, 25.5, 75, true);
```

### 3. 发布设备信息
```c
mqtt_comm_publish_device_info(mqtt_client, "ESP32_FAN_001", "v1.0.0");
```

## 测试方法

你可以使用MQTT客户端工具（如MQTTX、mosquitto客户端）连接到你的EMQX服务器进行测试：

1. 连接到 `nas.phenosolar.com:1883`，用户名密码都是 `admin`
2. 订阅 `esp32/fan_control/status` 和 `esp32/fan_control/device_info` 查看设备发送的数据
3. 向 `esp32/fan_control/command` 发送控制命令
4. 向 `esp32/fan_control/config` 发送配置参数

## 注意事项

- 确保ESP32已连接到WiFi网络
- 确保网络可以访问EMQX服务器
- MQTT客户端在连接成功后会自动订阅命令和配置主题
- 所有消息都使用JSON格式传输
