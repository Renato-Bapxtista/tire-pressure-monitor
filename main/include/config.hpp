#pragma once
#include "driver/gpio.h"

// I2C0 - Display OLED
constexpr gpio_num_t I2C0_SDA_PIN = GPIO_NUM_5;
constexpr gpio_num_t I2C0_SCL_PIN = GPIO_NUM_4;
constexpr uint8_t    OLED_I2C_ADDRESS = 0x3C;


// I2C1 - Sensores
constexpr gpio_num_t I2C1_SDA_PIN = GPIO_NUM_33;
constexpr gpio_num_t I2C1_SCL_PIN = GPIO_NUM_32;
constexpr uint8_t    BMP280_I2C_ADDRESS = 0x76;
constexpr uint8_t    SMP3011_I2C_ADDRESS = 0x78;
#define I2C_CLOCK_SPEED 400000 
// Configurações do sistema
constexpr uint32_t SENSOR_READ_INTERVAL_MS = 2000;

// ADD THESE BUTTON PIN DEFINITIONS:
#define BUTTON_UP_PIN   GPIO_NUM_32
#define BUTTON_DOWN_PIN GPIO_NUM_33  
#define BUTTON_MODE_PIN GPIO_NUM_25

// Task stack sizes
#define SENSOR_TASK_STACK_SIZE  4096
#define BUTTON_TASK_STACK_SIZE  2048
#define DISPLAY_TASK_STACK_SIZE 4096
#define SYSTEM_TASK_STACK_SIZE  4096
#define POWER_TASK_STACK_SIZE   2048

// Task priorities
#define SENSOR_TASK_PRIORITY  5
#define BUTTON_TASK_PRIORITY  4
#define DISPLAY_TASK_PRIORITY 3
#define SYSTEM_TASK_PRIORITY  6
#define POWER_TASK_PRIORITY   2

// Queue sizes
#define QUEUE_SIZE 10