#pragma once
#include "esp_err.h"
#include "i2c_manager.hpp"
#include "u8g2.h"

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
    void update_display_brightness(uint8_t brightness_level);
    bool is_display_initialized() const { return display_initialized_; }

private:
    I2CManager* i2c_manager_;
    uint8_t device_address_;
    bool display_initialized_;
    u8g2_t u8g2_display_;

    // Callbacks estáticas para U8G2
    static uint8_t i2c_byte_callback(u8x8_t* u8x8_handle, uint8_t msg, uint8_t arg_int, void* arg_ptr);
    static uint8_t gpio_delay_callback(u8x8_t* u8x8_handle, uint8_t msg, uint8_t arg_int, void* arg_ptr);
    
    // Instância para acessar membros não-estáticos
    static OLEDDisplay* instance_;
};