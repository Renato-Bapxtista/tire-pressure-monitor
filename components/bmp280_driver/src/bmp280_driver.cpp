#include "bmp280_driver.hpp"
#include "esp_log.h"

static const char *TAG = "BMP280Driver";

BMP280Driver::BMP280Driver(I2CManager* i2c_manager, uint8_t device_address) 
    : i2c_manager_(i2c_manager), device_address_(device_address), sensor_initialized_(false) {
    
    // Inicializar estrutura de calibração com zeros
    calibration_data_ = {};
}

BMP280Driver::~BMP280Driver() {
    ESP_LOGI(TAG, "BMP280 driver destruído");
}

esp_err_t BMP280Driver::initialize_sensor() {
    ESP_LOGI(TAG, "Inicializando sensor BMP280 no endereço 0x%02X", device_address_);

    // Resetar o dispositivo
    esp_err_t operation_result = i2c_manager_->write_register(device_address_, REGISTER_RESET, RESET_COMMAND);
    if (operation_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao resetar BMP280: %s", esp_err_to_name(operation_result));
        return operation_result;
    }

    // Aguardar reset completar
    vTaskDelay(pdMS_TO_TICKS(10));

    // Verificar ID do chip
    uint8_t chip_identification;
    operation_result = i2c_manager_->read_register(device_address_, REGISTER_CHIP_ID, &chip_identification, 1);
    if (operation_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao ler ID do chip: %s", esp_err_to_name(operation_result));
        return operation_result;
    }

    if (chip_identification != CHIP_ID_EXPECTED) {
        ESP_LOGE(TAG, "ID do chip BMP280 incorreto: esperado 0x%02X, recebido 0x%02X", 
                CHIP_ID_EXPECTED, chip_identification);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Chip BMP280 identificado corretamente: 0x%02X", chip_identification);

    // Ler dados de calibração
    operation_result = read_calibration_data();
    if (operation_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao ler dados de calibração: %s", esp_err_to_name(operation_result));
        return operation_result;
    }

    // Configurar operação do sensor
    operation_result = configure_sensor_operation();
    if (operation_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar operação do sensor: %s", esp_err_to_name(operation_result));
        return operation_result;
    }

    sensor_initialized_ = true;
    ESP_LOGI(TAG, "BMP280 inicializado com sucesso");
    return ESP_OK;
}

esp_err_t BMP280Driver::read_calibration_data() {
    uint8_t calibration_buffer[24];
    esp_err_t operation_result = i2c_manager_->read_register(device_address_, REGISTER_CALIBRATION_START, 
                                                           calibration_buffer, sizeof(calibration_buffer));
    if (operation_result != ESP_OK) {
        return operation_result;
    }

    // Extrair coeficientes de temperatura
    calibration_data_.temperature_coefficient_1 = (calibration_buffer[1] << 8) | calibration_buffer[0];
    calibration_data_.temperature_coefficient_2 = (calibration_buffer[3] << 8) | calibration_buffer[2];
    calibration_data_.temperature_coefficient_3 = (calibration_buffer[5] << 8) | calibration_buffer[4];
    
    // Extrair coeficientes de pressão
    calibration_data_.pressure_coefficient_1 = (calibration_buffer[7] << 8) | calibration_buffer[6];
    calibration_data_.pressure_coefficient_2 = (calibration_buffer[9] << 8) | calibration_buffer[8];
    calibration_data_.pressure_coefficient_3 = (calibration_buffer[11] << 8) | calibration_buffer[10];
    calibration_data_.pressure_coefficient_4 = (calibration_buffer[13] << 8) | calibration_buffer[12];
    calibration_data_.pressure_coefficient_5 = (calibration_buffer[15] << 8) | calibration_buffer[14];
    calibration_data_.pressure_coefficient_6 = (calibration_buffer[17] << 8) | calibration_buffer[16];
    calibration_data_.pressure_coefficient_7 = (calibration_buffer[19] << 8) | calibration_buffer[18];
    calibration_data_.pressure_coefficient_8 = (calibration_buffer[21] << 8) | calibration_buffer[20];
    calibration_data_.pressure_coefficient_9 = (calibration_buffer[23] << 8) | calibration_buffer[22];

    ESP_LOGI(TAG, "Dados de calibração lidos com sucesso");
    return ESP_OK;
}

esp_err_t BMP280Driver::configure_sensor_operation() {
    // Configurar: oversampling temperatura x2, pressão x16, modo normal
    uint8_t control_configuration = (0x02 << 5) | (0x05 << 2) | 0x03;
    return i2c_manager_->write_register(device_address_, REGISTER_CONTROL_MEASUREMENT, control_configuration);
}

esp_err_t BMP280Driver::read_temperature_and_pressure(float* temperature_celsius, float* pressure_hectopascal) {
    if (!sensor_initialized_) {
        ESP_LOGE(TAG, "Sensor não inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t sensor_readings[6];
    esp_err_t operation_result = i2c_manager_->read_register(device_address_, REGISTER_DATA_START, 
                                                           sensor_readings, sizeof(sensor_readings));
    if (operation_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao ler dados do sensor: %s", esp_err_to_name(operation_result));
        return operation_result;
    }

    // Combinar bytes para valores brutos
    int32_t uncompensated_pressure = (sensor_readings[0] << 12) | (sensor_readings[1] << 4) | (sensor_readings[2] >> 4);
    int32_t uncompensated_temperature = (sensor_readings[3] << 12) | (sensor_readings[4] << 4) | (sensor_readings[5] >> 4);

    // Compensação
    int32_t fine_temperature;
    int32_t compensated_temperature = compensate_temperature_reading(uncompensated_temperature, &fine_temperature);
    uint32_t compensated_pressure = compensate_pressure_reading(uncompensated_pressure, fine_temperature);

    // Converter para unidades padrão
    *temperature_celsius = compensated_temperature / 100.0f;
    *pressure_hectopascal = compensated_pressure / 25600.0f;

    ESP_LOGD(TAG, "Leitura: %.1f°C, %.1f hPa", *temperature_celsius, *pressure_hectopascal);
    return ESP_OK;
}

int32_t BMP280Driver::compensate_temperature_reading(int32_t uncompensated_temperature, int32_t* fine_temperature_output) {
    int32_t variable_1 = ((((uncompensated_temperature >> 3) - 
                          ((int32_t)calibration_data_.temperature_coefficient_1 << 1))) * 
                         ((int32_t)calibration_data_.temperature_coefficient_2)) >> 11;

    int32_t variable_2 = (((((uncompensated_temperature >> 4) - 
                           (int32_t)calibration_data_.temperature_coefficient_1) * 
                          ((uncompensated_temperature >> 4) - 
                           (int32_t)calibration_data_.temperature_coefficient_1)) >> 12) * 
                         ((int32_t)calibration_data_.temperature_coefficient_3)) >> 14;

    *fine_temperature_output = variable_1 + variable_2;
    
    int32_t temperature = (*fine_temperature_output * 5 + 128) >> 8;
    return temperature;
}

uint32_t BMP280Driver::compensate_pressure_reading(int32_t uncompensated_pressure, int32_t fine_temperature) {
    int64_t variable_1 = ((int64_t)fine_temperature) - 128000;
    int64_t variable_2 = variable_1 * variable_1 * (int64_t)calibration_data_.pressure_coefficient_6;
    variable_2 = variable_2 + ((variable_1 * (int64_t)calibration_data_.pressure_coefficient_5) << 17);
    variable_2 = variable_2 + (((int64_t)calibration_data_.pressure_coefficient_4) << 35);
    
    variable_1 = ((variable_1 * variable_1 * (int64_t)calibration_data_.pressure_coefficient_3) >> 8) + 
                 ((variable_1 * (int64_t)calibration_data_.pressure_coefficient_2) << 12);
    variable_1 = ((((int64_t)1 << 47) + variable_1)) * ((int64_t)calibration_data_.pressure_coefficient_1) >> 33;

    if (variable_1 == 0) {
        return 0;
    }

    int64_t pressure = 1048576 - uncompensated_pressure;
    pressure = (((pressure << 31) - variable_2) * 3125) / variable_1;
    
    variable_1 = (((int64_t)calibration_data_.pressure_coefficient_9) * (pressure >> 13) * (pressure >> 13)) >> 25;
    variable_2 = (((int64_t)calibration_data_.pressure_coefficient_8) * pressure) >> 19;
    
    pressure = ((pressure + variable_1 + variable_2) >> 8) + (((int64_t)calibration_data_.pressure_coefficient_7) << 4);
    
    return (uint32_t)pressure;
}