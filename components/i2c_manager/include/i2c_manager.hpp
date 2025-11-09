#pragma once
#include "driver/i2c.h"
#include "esp_err.h"

class I2CManager {
public:
    I2CManager(i2c_port_t port);
    ~I2CManager();

    esp_err_t initialize(gpio_num_t sda, gpio_num_t scl, uint32_t clk_speed = 100000);
    esp_err_t probe_device(uint8_t device_addr);
    esp_err_t write_register(uint8_t device_addr, uint8_t reg_addr, uint8_t data);
    esp_err_t read_register(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t len);
    
    i2c_port_t get_port() const { return port_; }
    bool is_initialized() const { return initialized_; }

private:
    i2c_port_t port_;
    bool initialized_;
};