#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include "driver/gpio.h"

/**
 * @brief Initialize DS18B20 sensor on the specified GPIO pin
 */
void temp_sensor_init(gpio_num_t pin);

/**
 * @brief Read temperature from DS18B20
 * @return Temperature in Celsius
 */
float temp_sensor_get_temperature(void);

#endif // TEMP_SENSOR_H
