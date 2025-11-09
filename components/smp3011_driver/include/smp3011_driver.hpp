#pragma once
#include "esp_err.h"
#include "i2c_manager.hpp"

class SMP3011Driver {
public:
    SMP3011Driver(I2CManager* i2c_manager, uint8_t device_address);
    ~SMP3011Driver();

    esp_err_t initialize_sensor();
    esp_err_t read_pressure(float* pressure_kilopascal);
    esp_err_t read_pressure_detailed(float* pressure_kilopascal, uint32_t* raw_value);
    esp_err_t set_pressure_offset(float offset_kpa);
    esp_err_t set_pressure_range(float min_pressure_kpa, float max_pressure_kpa); // ADD THIS LINE
    esp_err_t scan_sensor_registers();
    bool is_sensor_initialized() const { return sensor_initialized_; }

private:
    I2CManager* i2c_manager_;
    uint8_t device_address_;
    bool sensor_initialized_;
    
    // Configurações de medição - MAKE SURE THESE ARE DECLARED
    float minimum_measurement_pressure_;
    float maximum_measurement_pressure_;
    float pressure_scale_factor_;
    float pressure_offset_;

    // Registros do sensor
    static constexpr uint8_t REGISTER_WHO_AM_I = 0x0F;
    static constexpr uint8_t REGISTER_STATUS = 0x07;
    static constexpr uint8_t REGISTER_DATA_MSB = 0x00;
    static constexpr uint8_t REGISTER_DATA_LSB = 0x01;
    static constexpr uint8_t REGISTER_DATA_XLSB = 0x02;
    static constexpr uint8_t REGISTER_CONTROL = 0x08;
    
    static constexpr uint8_t COMMAND_START_MEASUREMENT = 0x01;
    static constexpr uint8_t EXPECTED_WHO_AM_I = 0x30;

    esp_err_t configure_sensor_operation();
    esp_err_t verify_sensor_identification();
    esp_err_t read_raw_pressure_data(uint32_t* raw_pressure);
    float convert_raw_to_pressure(uint32_t raw_data);
    uint32_t combine_pressure_data_bytes(uint8_t msb_byte, uint8_t lsb_byte, uint8_t xlsb_byte);
};