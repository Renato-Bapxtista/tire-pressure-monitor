#pragma once
#include "esp_err.h"
#include "i2c_manager.hpp"

class SMP3011Driver {
public:
    SMP3011Driver(I2CManager* i2c_manager, uint8_t device_address);
    ~SMP3011Driver();

    esp_err_t initialize_sensor();
    esp_err_t read_pressure(float* pressure_kilopascal);
    esp_err_t set_pressure_range(float minimum_pressure_kpa, float maximum_pressure_kpa);
    bool is_sensor_initialized() const { return sensor_initialized_; }

private:
    I2CManager* i2c_manager_;
    uint8_t device_address_;
    bool sensor_initialized_;
    
    // Configurações de medição
    float minimum_measurement_pressure_;
    float maximum_measurement_pressure_;
    float pressure_scale_factor_;

    // Registros do sensor (valores hipotéticos - ajustar conforme datasheet)
    static constexpr uint8_t REGISTER_COMMAND = 0x08;
    static constexpr uint8_t REGISTER_DATA_MSB = 0x00;
    static constexpr uint8_t REGISTER_DATA_LSB = 0x01;
    static constexpr uint8_t REGISTER_DATA_XLSB = 0x02;
    static constexpr uint8_t REGISTER_CONTROL = 0x01;
    static constexpr uint8_t REGISTER_CONFIG = 0x02;
    
    static constexpr uint8_t COMMAND_START_MEASUREMENT = 0x01;
    static constexpr uint8_t CONTROL_REGISTER_DEFAULT = 0x14; // 12-bit resolution
    static constexpr uint8_t CONFIG_REGISTER_DEFAULT = 0x00;  // Normal mode

    esp_err_t configure_sensor_operation();
    esp_err_t verify_sensor_communication();
    float convert_raw_pressure_to_kilopascal(uint32_t raw_pressure_value);
    uint32_t combine_pressure_data_bytes(uint8_t msb_byte, uint8_t lsb_byte, uint8_t xlsb_byte);
};