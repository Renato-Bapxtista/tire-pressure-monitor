#pragma once
#include "esp_err.h"
#include "i2c_manager.hpp"

class BMP280Driver {
public:
    BMP280Driver(I2CManager* i2c_manager, uint8_t device_address);
    ~BMP280Driver();

    esp_err_t initialize_sensor();
    esp_err_t read_temperature_and_pressure(float* temperature_celsius, float* pressure_hectopascal);
    bool is_sensor_initialized() const { return sensor_initialized_; }

private:
    I2CManager* i2c_manager_;
    uint8_t device_address_;
    bool sensor_initialized_;

    // Dados de calibração com nomes descritivos
    struct CalibrationData {
        uint16_t temperature_coefficient_1;
        int16_t temperature_coefficient_2;
        int16_t temperature_coefficient_3;
        uint16_t pressure_coefficient_1;
        int16_t pressure_coefficient_2;
        int16_t pressure_coefficient_3;
        int16_t pressure_coefficient_4;
        int16_t pressure_coefficient_5;
        int16_t pressure_coefficient_6;
        int16_t pressure_coefficient_7;
        int16_t pressure_coefficient_8;
        int16_t pressure_coefficient_9;
    } calibration_data_;

    // Registros do BMP280
    static constexpr uint8_t REGISTER_CHIP_ID = 0xD0;
    static constexpr uint8_t REGISTER_RESET = 0xE0;
    static constexpr uint8_t REGISTER_CALIBRATION_START = 0x88;
    static constexpr uint8_t REGISTER_CONTROL_MEASUREMENT = 0xF4;
    static constexpr uint8_t REGISTER_DATA_START = 0xF7;
    
    static constexpr uint8_t CHIP_ID_EXPECTED = 0x58;
    static constexpr uint8_t RESET_COMMAND = 0xB6;

    esp_err_t read_calibration_data();
    esp_err_t configure_sensor_operation();
    int32_t compensate_temperature_reading(int32_t uncompensated_temperature, int32_t* fine_temperature_output);
    uint32_t compensate_pressure_reading(int32_t uncompensated_pressure, int32_t fine_temperature);
};