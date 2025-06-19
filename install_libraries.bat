@echo off
echo 🚀 ESP32智能风扇控制器 - 依赖库安装
echo ===================================

echo 📦 检查ESP-IDF环境...
if "%IDF_PATH%"=="" (
    echo ❌ 错误：未找到ESP-IDF环境
    echo 请先运行ESP-IDF命令行环境，然后重试
    pause
    exit /b 1
)
echo ✅ ESP-IDF环境正常

echo 📝 项目使用统一依赖管理 (idf_component.yml)
echo 包含的主要库：
echo   - ds18b20: DS18B20温度传感器驱动
echo   - ssd1306: OLED显示屏驱动  
echo   - cjson: JSON数据处理
echo   - rotary_encoder: 旋转编码器
echo   - button: 按钮处理

echo ⚙️ 清理缓存并重新配置...
if exist "managed_components" rd /s /q "managed_components"
idf.py fullclean

echo 🔄 重新配置项目 (会自动下载依赖)...
idf.py reconfigure

echo 🔨 编译项目...
idf.py build

if %errorlevel% equ 0 (
    echo.
    echo ✅ 项目编译成功！
    echo.
    echo 📋 下一步：
    echo 1. 连接硬件（参考README.md）
    echo 2. 烧录: idf.py flash
    echo 3. 监控: idf.py monitor
    echo.
    echo 💡 所有依赖库已自动安装到 managed_components/ 目录
) else (
    echo ❌ 编译失败，请检查错误信息
    echo 💡 提示：确保所有依赖库都可以正常下载
)

pause
