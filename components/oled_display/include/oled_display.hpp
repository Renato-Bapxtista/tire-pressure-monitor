#pragma once
#include "esp_err.h"
#include "i2c_manager.hpp"

class OLEDDisplay {
public:
    OLEDDisplay(I2CManager* i2c_manager, uint8_t device_address);
    ~OLEDDisplay();

    esp_err_t initialize_display();
    void clear_display();
    void display_welcome_screen();
    void display_system_status(const char* status_message);
    void display_sensor_readings(float temperature_celsius, float atmospheric_pressure_hpa, float tire_pressure_kpa);
    void display_error_message(const char* error_message);
    bool is_display_initialized() const { return display_initialized_; }

private:
    I2CManager* i2c_manager_;
    uint8_t device_address_;
    bool display_initialized_;

    esp_err_t send_command(uint8_t command);
    esp_err_t send_data(const uint8_t* data, size_t length);
    esp_err_t send_command_sequence(const uint8_t* commands, size_t length);
    void draw_text(uint8_t x, uint8_t y, const char* text);
    void draw_horizontal_line(uint8_t x, uint8_t y, uint8_t length);
};