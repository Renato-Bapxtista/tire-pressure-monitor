#include "i2c_manager.hpp"
#include "esp_log.h"

static const char *TAG = "I2CManager";

I2CManager::I2CManager(i2c_port_t port) : port_(port), initialized_(false) {}

I2CManager::~I2CManager() {
    if (initialized_) {
        i2c_driver_delete(port_);
    }
}

esp_err_t I2CManager::initialize(gpio_num_t sda, gpio_num_t scl, uint32_t clk_speed) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = clk_speed,
        },
        .clk_flags = 0,
    };

    esp_err_t ret = i2c_param_config(port_, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C parameter config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(port_, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "I2C port %d initialized", port_);
    return ESP_OK;
}

esp_err_t I2CManager::probe_device(uint8_t device_addr) {
    if (!initialized_) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(port_, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "I2C device found at address 0x%02X", device_addr);
    }

    return ret;
}

esp_err_t I2CManager::write_register(uint8_t device_addr, uint8_t reg_addr, uint8_t data) {
    if (!initialized_) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(port_, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

esp_err_t I2CManager::read_register(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t len) {
    if (!initialized_) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(port_, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}