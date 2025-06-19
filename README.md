# 🌪️ ESP32 智能风扇控制器

## 📋 项目概述

基于ESP32的智能风扇控制系统，集成温度监测、自动调速、OLED显示、旋转编码器控制和MQTT远程管理功能。使用ESP-IDF官方组件构建，代码结构清晰，易于维护和扩展。

## ✨ 核心特性

- 🌡️ **DS18B20温度监测** - 精确温度检测，支持多传感器
- 🔄 **智能温控算法** - 根据温度自动调节风扇转速
- 🎮 **旋转编码器控制** - 本地手动调速和模式切换
- 📺 **SSD1306 OLED显示** - 实时显示温度、转速、运行模式（基于ESP-IDF官方I2C驱动实现，无第三方库）
- 📡 **MQTT远程控制** - 支持远程监控和参数配置
- 📶 **WiFi智能配网** - 网页配置WiFi，用户友好
- 💾 **NVS持久化存储** - 配置参数断电保存

## 🔧 硬件要求

### 核心组件
| 组件 | 型号 | 连接引脚 | 说明 |
|------|------|----------|------|
| 主控 | ESP32开发板 | - | 使用ESP32-WROOM-32 |
| 温度传感器 | DS18B20 | GPIO 4 | 需要4.7kΩ上拉电阻 |  
| 显示屏 | SSD1306 OLED (128x64) | SDA: GPIO 21<br>SCL: GPIO 22 | I2C接口，官方I2C驱动 |
| 编码器 | EC11旋转编码器 | A: GPIO 15<br>B: GPIO 2<br>BTN: GPIO 0 | 带按钮功能 |
| 制冷片 | MOS管+TEC | GPIO 18 | PWM控制制冷片功率 |
| 风扇 | PWM风扇 | GPIO 19 | 温度自动调速 |
| 电源 | 5V/12V适配器 | - | 根据风扇规格选择 |

### 接线图
```
ESP32 Development Board
┌─────────────────────────┐
│  ┌─────┐               │
│  │ USB │  ┌──────────── │ ── GPIO 4  ──── DS18B20 (Data)
│  └─────┘  │             │ ── GPIO 18 ──── Cooler (MOS/TEC)
│           │             │ ── GPIO 19 ──── Fan PWM
│           │             │ ── GPIO 21 ──── OLED SDA  
│           │    ESP32    │ ── GPIO 22 ──── OLED SCL
│           │             │ ── GPIO 15 ──── Encoder A
│           │             │ ── GPIO 2  ──── Encoder B  
│           │             │ ── GPIO 0  ──── Encoder BTN
│           └──────────── │
└─────────────────────────┘
```

## 🚀 快速开始

### 1. 环境搭建
```bash
# 安装ESP-IDF开发环境 (需要v5.4.0+)
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && git checkout v5.4.1
./install.sh && source export.sh
```

### 2. 项目构建
```bash
# 获取项目代码
git clone <repository-url>
cd esp32_FAN_control

# 安装依赖库 (自动安装ESP组件)
idf.py reconfigure

# 编译固件
idf.py build

# 烧录到设备
idf.py flash monitor
```

### 3. 设备配置
1. **首次启动**: 设备自动创建WiFi热点 `ESP32_Config`
2. **连接配网**: 手机连接热点，浏览器访问 `http://192.168.4.1`
3. **WiFi设置**: 输入目标WiFi的SSID和密码
4. **完成配置**: 设备重启后自动连接WiFi并启用MQTT

## 🎮 使用说明
### 操作控制

#### 模式说明
- **自动模式**  
  - 风扇和制冷片均根据温度自动调节，无需人工干预。
  - 旋转编码器旋转无效，短按按钮可切换至手动模式。
  - OLED屏显示温度、风扇转速、制冷片功率及“自动”模式标识。

- **手动模式**  
  - 风扇依然根据温度自动调速，用户无法直接调节风扇转速。
  - 旋转编码器调节制冷片功率，实现精细手动控制。
  - OLED屏显示温度、风扇转速、制冷片功率及“手动”模式标识。

#### 控制方式
- **旋转编码器**  
  - 自动模式下：旋转无效，仅用于查看信息。
  - 手动模式下：旋转调节制冷片功率，步进可自定义。
  - 短按：切换自动/手动模式。
- **OLED显示**  
  - 实时显示温度、风扇转速、制冷片功率和当前运行模式。

### 显示界面
新版OLED界面分行显示如下：
```
┌────────────────────────────┐
│ Temp : 26.5 °C             │
│ Fan  : 65% (Auto)          │
│ Cooler: 80% (Auto)         │
│ Mode : 自动                │
└────────────────────────────┘
```
手动模式示例：
```
┌────────────────────────────┐
│ Temp : 26.5 °C             │
│ Fan  : 65%                 │
│ Cooler: 40% (Manual)        │
│ Mode : 手动                │
└────────────────────────────┘
```
> OLED显示底层已用ESP-IDF官方I2C API实现SSD1306驱动，支持英文字符分行显示。如需自定义显示内容，可扩展ASCII字体表或绘图函数。

### 控制逻辑
- **制冷片**: 支持自动/手动两种功率控制（MQTT/本地均可）
- **风扇**: 始终自动运行，温度越高转速越高
- **温控算法**: 
  - ≤25°C → 0%转速
  - 25-30°C → 50%转速  
  - >30°C → 100%转速

## 📡 MQTT通信协议

### 连接参数
```yaml
MQTT服务器: nas.phenosolar.com
端口: 1883
用户名: admin
密码: admin
```

### 消息格式

#### 📤 状态上报 (每5秒)
```bash
主题: esp32/fan_control/status
格式: {"temp": 25.5, "speed": 60, "mode": "auto"}
```

#### 📥 远程控制
```bash
# 控制命令
主题: esp32/fan_control/command  
格式: {"speed": 80, "mode": "manual"}

# 参数配置
主题: esp32/fan_control/config
格式: {"temp_threshold": 30, "max_speed": 100}
```

## 🏗️ 项目架构

### 目录结构
```
esp32_FAN_control/
├── main/
│   └── main.c                    # 主程序入口
├── components/                   # 功能组件
│   ├── temp_sensor/             # DS18B20温度传感器
│   ├── fan_control/             # PWM风扇控制
│   ├── oled_display/            # SSD1306显示
│   ├── user_input/              # 旋转编码器输入
│   ├── mqtt_comm/               # MQTT通信
│   └── wifi_provision/          # WiFi配网
├── idf_component.yml            # 依赖管理
├── CMakeLists.txt               # 构建配置
└── README.md                    # 项目文档
```

### 技术栈
- **开发框架**: ESP-IDF v5.4.1
- **硬件平台**: ESP32 (Xtensa LX6双核)
- **网络协议**: WiFi 802.11 b/g/n + MQTT 3.1.1
- **外设驱动**: 1-Wire, I2C, PWM, GPIO
- **依赖库**: 全部使用ESP-IDF官方组件（OLED显示仅依赖driver子系统，无第三方库）

### 核心依赖
```yaml
# 项目依赖 (idf_component.yml)
dependencies:
  idf: ">=5.4.0"
  # 仅使用ESP-IDF官方组件和本地自定义组件，已移除所有第三方库依赖
```

## 🛠️ 故障排除

### 常见问题及解决方案

| 问题现象 | 可能原因 | 解决方法 |
|----------|----------|----------|
| 温度显示-127°C | DS18B20未连接或损坏 | 检查接线和上拉电阻 |
| WiFi连接失败 | 信号弱或密码错误 | 重新配网或检查路由器 |
| OLED无显示 | I2C接线错误 | 检查SDA/SCL连接 |
| 风扇不转 | PWM信号异常 | 检查GPIO18连接 |
| MQTT断开 | 网络不稳定 | 检查网络连通性 |

### 调试命令
```bash
# 查看运行日志
idf.py monitor

# 完全重置设备
idf.py erase-flash && idf.py flash

# 仅查看ESP32日志输出
idf.py monitor --print_filter="*:I"
```

## 📊 性能指标

- **温度精度**: ±0.5°C (DS18B20)
- **PWM频率**: 25kHz (无噪音)
- **响应延迟**: <100ms (本地控制)
- **MQTT延迟**: <500ms (网络正常)
- **功耗**: 典型值150mA@5V
- **工作温度**: -10°C ~ +85°C

## 🎯 开发路线

### 已完成功能 ✅
- [x] 基础温控功能
- [x] OLED实时显示  
- [x] 旋转编码器控制
- [x] WiFi智能配网
- [x] MQTT远程监控
- [x] NVS配置存储
- [x] 模块化架构重构

### 计划中功能 📋
- [ ] Web管理界面
- [ ] 定时控制策略
- [ ] 多区域温控
- [ ] 历史数据图表
- [ ] 移动App控制
- [ ] Home Assistant集成

## 🤝 参与贡献

欢迎提交Issue和Pull Request来改进项目！

### 贡献指南
1. Fork本项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交修改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送分支 (`git push origin feature/AmazingFeature`)
5. 创建Pull Request

## 📄 开源许可

本项目采用 [MIT License](LICENSE) 开源协议

## 📞 技术支持

- 📧 Email: support@example.com
- 💬 Issues: [GitHub Issues](https://github.com/username/esp32_FAN_control/issues)
- 📖 Wiki: [项目Wiki](https://github.com/username/esp32_FAN_control/wiki)

---

### 🌟 如果此项目对您有帮助，请点个Star支持我们！

**最后更新**: 2025年6月19日  
**项目版本**: v2.0  
**ESP-IDF版本**: v5.4.1


