# ESP32 智能风扇控制器

## 📋 项目概述

基于ESP32的智能风扇控制系统，支持温度监测、自动调速、手动控制、OLED显示和MQTT远程控制。

## 🎯 主要功能

- **温度监测**：DS18B20数字温度传感器
- **智能调速**：根据温度自动调节风扇速度
- **手动控制**：旋转编码器手动设置风扇速度
- **OLED显示**：实时显示温度、速度和运行状态
- **WiFi配置**：网页配置WiFi连接信息
- **MQTT通信**：远程监控和控制
- **NVS存储**：配置信息持久化保存

## 🔧 硬件连接

### 必需硬件
- ESP32开发板
- DS18B20温度传感器
- PWM风扇（12V/5V）
- 旋转编码器（带按钮）
- OLED显示屏（SSD1306, I2C）
- 电源适配器

### 引脚定义
```
DS18B20温度传感器    -> GPIO 4
PWM风扇控制         -> GPIO 18
旋转编码器 A        -> GPIO 15
旋转编码器 B        -> GPIO 2
旋转编码器按钮      -> GPIO 0
OLED显示屏 SDA      -> GPIO 21
OLED显示屏 SCL      -> GPIO 22
```

## 🚀 快速开始

### 1. 环境准备
```bash
# 安装ESP-IDF v5.4+
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.4.1
./install.sh
source export.sh
```

### 2. 编译烧录
```bash
# 克隆项目
git clone https://github.com/LightningFlashEvE/esp32_FAN_control.git
cd esp32_FAN_control

# 配置项目
idf.py menuconfig

# 编译和烧录
idf.py build
idf.py flash monitor
```

### 3. WiFi配置
1. 首次启动时，ESP32会创建热点 `ESP32_Config`
2. 连接此热点（开放网络，无密码）
3. 浏览器访问 `192.168.4.1`
4. 输入WiFi名称和密码
5. 设备自动重启并连接WiFi

## 📡 MQTT通信

### 连接配置
- **服务器**：`nas.phenosolar.com:1883`
- **用户名**：`admin`
- **密码**：`admin`

### 主题定义

#### 📤 状态上报（每5秒）
**主题**：`esp32/fan_control/status`
**格式**：
```json
{
  "temp": 25.5,
  "speed": 60,
  "mode": "auto"
}
```

#### 📥 控制命令
**主题**：`esp32/fan_control/command`
**格式**：
```json
{
  "speed": 80,
  "mode": "manual"
}
```

#### ⚙️ 配置参数
**主题**：`esp32/fan_control/config`
**格式**：
```json
{
  "temp_threshold": 30,
  "max_speed": 100
}
```

## 🎮 操作说明

### 旋转编码器操作
- **旋转**：调节风扇速度（手动模式下）
- **短按**：切换自动/手动模式
- **长按**：进入配置菜单

### OLED显示信息
```
温度:25.5°C 模式:自动
速度: 60% [######----]
```

### 运行模式
- **自动模式**：根据温度自动调节风扇速度
- **手动模式**：固定风扇速度，可手动调节

## 📊 温度控制算法

```
温度 ≤ 25°C  -> 风扇速度 0%
温度 25-30°C -> 风扇速度 50%
温度 > 30°C  -> 风扇速度 100%
```

## 📁 项目结构

```
esp32_FAN_control/
├── main/
│   └── main.c                    # 主程序
├── components/
│   ├── wifi_provision/           # WiFi配置组件
│   ├── temp_sensor/             # 温度传感器组件
│   ├── fan_control/             # 风扇控制组件
│   ├── user_input/              # 用户输入组件
│   ├── oled_display/            # OLED显示组件
│   └── mqtt_comm/               # MQTT通信组件
├── CMakeLists.txt
└── README.md
```

## 🔧 技术栈

- **主控**：ESP32 (ESP-IDF v5.4.1)
- **网络**：WiFi + MQTT
- **传感器**：DS18B20 (1-Wire)
- **显示**：SSD1306 OLED (I2C)
- **存储**：NVS (Non-Volatile Storage)
- **控制**：PWM + 旋转编码器

## 🛠️ 故障排除

### 常见问题

1. **WiFi连接失败**
   - 检查WiFi名称和密码
   - 确认信号强度
   - 重新进入配置模式

2. **温度传感器错误**
   - 检查DS18B20接线
   - 确认4.7kΩ上拉电阻
   - 温度显示-127.0°C表示传感器未连接

3. **MQTT连接失败**
   - 检查服务器地址和端口
   - 确认网络连通性
   - 查看串口日志

4. **OLED显示异常**
   - 检查I2C接线
   - 确认OLED地址（通常0x3C）

### 调试方法
```bash
# 查看详细日志
idf.py monitor

# 清除配置重新开始
idf.py erase-flash
idf.py flash
```

## 📈 性能指标

- **温度精度**：±0.5°C
- **PWM频率**：1kHz
- **MQTT上报间隔**：5秒
- **OLED刷新率**：实时更新
- **功耗**：< 500mA @ 5V

## 🔮 未来扩展

- [ ] Web管理界面
- [ ] 多传感器支持
- [ ] 定时控制功能
- [ ] 历史数据记录
- [ ] 手机App控制
- [ ] 语音控制集成

## 🎯 测试示例

### MQTT控制测试
```bash
# 设置手动模式，风扇速度70%
主题: esp32/fan_control/command
消息: {"speed": 70, "mode": "manual"}

# 切换到自动模式
主题: esp32/fan_control/command
消息: {"mode": "auto"}

# 设置温度阈值
主题: esp32/fan_control/config
消息: {"temp_threshold": 28, "max_speed": 90}
```

## 📄 许可证

MIT License

## 👥 贡献

欢迎提交Issue和Pull Request！

## 📝 更新历史

- **2025-06-17**：完成项目重构，增加模块化设计
- **2025-06-17**：添加MQTT JSON格式通信
- **2025-06-17**：完善WiFi配置网页功能
- **2025-06-17**：添加OLED显示和旋转编码器支持

---

**⭐ 如果这个项目对你有帮助，请给个Star！**
