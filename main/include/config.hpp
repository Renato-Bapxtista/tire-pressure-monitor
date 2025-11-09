#pragma once
#include "driver/gpio.h"

// I2C0 - Display OLED
constexpr gpio_num_t I2C0_SDA_PIN = GPIO_NUM_5;
constexpr gpio_num_t I2C0_SCL_PIN = GPIO_NUM_4;
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;

// I2C1 - Sensores
constexpr gpio_num_t I2C1_SDA_PIN = GPIO_NUM_33;
constexpr gpio_num_t I2C1_SCL_PIN = GPIO_NUM_32;
constexpr uint8_t BMP280_I2C_ADDRESS = 0x76;
constexpr uint8_t SMP3011_I2C_ADDRESS = 0x78;

// Configurações do sistema
constexpr uint32_t SENSOR_READ_INTERVAL_MS = 2000;