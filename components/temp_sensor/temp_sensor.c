#include "temp_sensor.h"
#include "ds18b20.h"
#include "onewire_bus.h"
#include "esp_log.h"

static const char* TAG = "TEMP_SENSOR";
static onewire_bus_handle_t bus_handle = NULL;
static ds18b20_device_handle_t device_handle = NULL;

/**
 * @brief 初始化DS18B20温度传感器 - 使用官方DS18B20库
 * @param gpio_pin 数据引脚GPIO
 */
void temp_sensor_init(gpio_num_t gpio_pin) {
    // 创建1-Wire总线
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = gpio_pin,
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10, // 10个字节足以接收64位ROM码 + 9字节暂存器
    };
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus_handle));
    ESP_LOGI(TAG, "1-Wire总线已在GPIO %d上创建", gpio_pin);

    // 创建DS18B20设备迭代器，搜索设备
    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t next_onewire_device;
    esp_err_t search_result = ESP_OK;

    ESP_ERROR_CHECK(onewire_new_device_iter(bus_handle, &iter));
    ESP_LOGI(TAG, "开始搜索DS18B20设备...");
    do {
        search_result = onewire_device_iter_get_next(iter, &next_onewire_device);
        if (search_result == ESP_OK) { // 找到设备，检查是否为DS18B20
            ESP_LOGI(TAG, "找到设备，地址: %016llX", next_onewire_device.address);
            // 创建DS18B20设备
            ESP_ERROR_CHECK(ds18b20_new_device(&next_onewire_device, &device_handle));
            ESP_LOGI(TAG, "DS18B20设备初始化成功");
            break;
        }
    } while (search_result != ESP_ERR_NOT_FOUND);

    onewire_del_device_iter(iter);
    if (!device_handle) {
        ESP_LOGE(TAG, "未找到DS18B20设备");
    }
}

/**
 * @brief 获取温度值 - 使用官方DS18B20库
 * @return 温度值（摄氏度），错误时返回-127.0
 */
float temp_sensor_get_temperature(void) {
    if (!device_handle) {
        ESP_LOGE(TAG, "DS18B20设备未初始化");
        return -127.0;
    }

    float temperature;
    esp_err_t ret = ds18b20_trigger_temperature_conversion(device_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "触发温度转换失败");
        return -127.0;
    }

    ret = ds18b20_get_temperature(device_handle, &temperature);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取温度失败");
        return -127.0;
    }

    ESP_LOGI(TAG, "温度: %.2f°C", temperature);
    return temperature;
}
