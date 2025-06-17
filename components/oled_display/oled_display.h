#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include "driver/i2c.h"
#include <stdbool.h>

/**
 * @brief Initialize OLED display (SSD1306) via I2C
 */
void oled_init(i2c_port_t i2c_num, gpio_num_t sda_pin, gpio_num_t scl_pin);

/**
 * @brief Update OLED display with temperature, speed, and mode
 */
void oled_display_update(float temperature, uint8_t speed, bool auto_mode);

#endif // OLED_DISPLAY_H
