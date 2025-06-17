# ESP32 智能风扇控制器

##  项目概述

基于ESP32的智能风扇控制系统，支持温度监测、自动调速、手动控制、OLED显示和MQTT远程控制。

##  主要功能

- **温度监测**：DS18B20数字温度传感器
- **智能调速**：根据温度自动调节风扇速度
- **手动控制**：旋转编码器手动设置风扇速度
- **OLED显示**：实时显示温度、速度和运行状态
- **WiFi配置**：网页配置WiFi连接信息
- **MQTT通信**：远程监控和控制
- **NVS存储**：配置信息持久化保存

##  硬件连接

### 必需硬件
- ESP32开发板
- DS18B20温度传感器
- PWM风扇（12V/5V）
- 旋转编码器（带按钮）
- OLED显示屏（SSD1306, I2C）
- 电源适配器

### 引脚定义
`
DS18B20温度传感器    -> GPIO 4
PWM风扇控制         -> GPIO 18
旋转编码器 A        -> GPIO 15
旋转编码器 B        -> GPIO 2
旋转编码器按钮      -> GPIO 0
OLED显示屏 SDA      -> GPIO 21
OLED显示屏 SCL      -> GPIO 22
`

##  快速开始

### 1. 环境准备
`ash
# 安装ESP-IDF v5.4+
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.4.1
./install.sh
source export.sh
`

### 2. 编译烧录
`ash
# 克隆项目
git clone https://github.com/LightningFlashEvE/esp32_FAN_control.git
cd esp32_FAN_control

# 配置项目
idf.py menuconfig

# 编译和烧录
idf.py build
idf.py flash monitor
`

### 3. WiFi配置
1. 首次启动时，ESP32会创建热点 ESP32_Config
2. 连接此热点（开放网络，无密码）
3. 浏览器访问 192.168.4.1
4. 输入WiFi名称和密码
5. 设备自动重启并连接WiFi

##  MQTT通信

### 连接配置
- **服务器**：nas.phenosolar.com:1883
- **用户名**：admin
- **密码**：admin

### 主题定义

####  状态上报（每5秒）
**主题**：esp32/fan_control/status
**格式**：
`json
{
  "temp": 25.5,
  "speed": 60,
  "mode": "auto"
}
`

####  控制命令
**主题**：esp32/fan_control/command
**格式**：
`json
{
  "speed": 80,
  "mode": "manual"
}
`

---

** 如果这个项目对你有帮助，请给个Star！**
