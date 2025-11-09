#include "smp3011_driver.hpp"
#include "esp_log.h"

static const char *TAG = "SMP3011Driver";

SMP3011Driver::SMP3011Driver(I2CManager* i2c_manager, uint8_t device_address) 
    : i2c_manager_(i2c_manager), 
      device_address_(device_address), 
      sensor_initialized_(false),
      minimum_measurement_pressure_(0.0f),
      maximum_measurement_pressure_(1000.0f), // 1000 kPa = ~145 PSI
      pressure_scale_factor_(0.0f) {}

SMP3011Driver::~SMP3011Driver() {
    ESP_LOGI(TAG, "Driver SMP3011 destruído");
}

esp_err_t SMP3011Driver::initialize_sensor() {
    ESP_LOGI(TAG, "Inicializando sensor SMP3011 no endereço 0x%02X", device_address_);

    // Verificar comunicação com o sensor
    esp_err_t communication_result = verify_sensor_communication();
    if (communication_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha na comunicação com SMP3011: %s", esp_err_to_name(communication_result));
        return communication_result;
    }

    // Configurar faixa de pressão padrão (0-1000 kPa)
    esp_err_t range_result = set_pressure_range(0.0f, 1000.0f);
    if (range_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar faixa de pressão: %s", esp_err_to_name(range_result));
        return range_result;
    }

    // Configurar operação do sensor
    esp_err_t config_result = configure_sensor_operation();
    if (config_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar operação do sensor: %s", esp_err_to_name(config_result));
        return config_result;
    }

    sensor_initialized_ = true;
    ESP_LOGI(TAG, "SMP3011 inicializado com sucesso");
    return ESP_OK;
}

esp_err_t SMP3011Driver::verify_sensor_communication() {
    // Tentar ler um registro do sensor para verificar comunicação
    uint8_t test_register_value;
    esp_err_t read_result = i2c_manager_->read_register(device_address_, REGISTER_CONTROL, &test_register_value, 1);
    
    if (read_result == ESP_OK) {
        ESP_LOGI(TAG, "Comunicação com SMP3011 verificada com sucesso");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Falha na comunicação I2C com SMP3011: %s", esp_err_to_name(read_result));
        return read_result;
    }
}

esp_err_t SMP3011Driver::configure_sensor_operation() {
    // Configurar registro de controle
    esp_err_t control_result = i2c_manager_->write_register(device_address_, REGISTER_CONTROL, CONTROL_REGISTER_DEFAULT);
    if (control_result != ESP_OK) {
        return control_result;
    }

    // Configurar registro de configuração
    esp_err_t config_result = i2c_manager_->write_register(device_address_, REGISTER_CONFIG, CONFIG_REGISTER_DEFAULT);
    if (config_result != ESP_OK) {
        return config_result;
    }

    ESP_LOGI(TAG, "Operação do sensor configurada");
    return ESP_OK;
}

esp_err_t SMP3011Driver::set_pressure_range(float minimum_pressure_kpa, float maximum_pressure_kpa) {
    if (minimum_pressure_kpa >= maximum_pressure_kpa) {
        ESP_LOGE(TAG, "Pressão mínima deve ser menor que pressão máxima");
        return ESP_ERR_INVALID_ARG;
    }

    minimum_measurement_pressure_ = minimum_pressure_kpa;
    maximum_measurement_pressure_ = maximum_pressure_kpa;
    
    // Calcular fator de escala (kPa por bit)
    // Assumindo resolução de 19 bits (como BMP280) - 524288 valores
    const uint32_t maximum_raw_value = 524287; // 2^19 - 1
    pressure_scale_factor_ = (maximum_pressure_kpa - minimum_pressure_kpa) / maximum_raw_value;

    ESP_LOGI(TAG, "Faixa de pressão configurada: %.1f-%.1f kPa, escala: %.6f kPa/bit", 
             minimum_pressure_kpa, maximum_pressure_kpa, pressure_scale_factor_);
    
    return ESP_OK;
}

esp_err_t SMP3011Driver::read_pressure(float* pressure_kilopascal) {
    if (!sensor_initialized_) {
        ESP_LOGE(TAG, "Sensor não inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    // Iniciar medição
    esp_err_t command_result = i2c_manager_->write_register(device_address_, REGISTER_COMMAND, COMMAND_START_MEASUREMENT);
    if (command_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao enviar comando de medição: %s", esp_err_to_name(command_result));
        return command_result;
    }

    // Aguardar conversão (ajustar conforme datasheet)
    vTaskDelay(pdMS_TO_TICKS(15));

    // Ler dados de pressão (3 bytes)
    uint8_t pressure_data_msb, pressure_data_lsb, pressure_data_xlsb;
    
    esp_err_t read_msb_result = i2c_manager_->read_register(device_address_, REGISTER_DATA_MSB, &pressure_data_msb, 1);
    esp_err_t read_lsb_result = i2c_manager_->read_register(device_address_, REGISTER_DATA_LSB, &pressure_data_lsb, 1);
    esp_err_t read_xlsb_result = i2c_manager_->read_register(device_address_, REGISTER_DATA_XLSB, &pressure_data_xlsb, 1);

    if (read_msb_result != ESP_OK || read_lsb_result != ESP_OK || read_xlsb_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao ler dados de pressão");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Combinar bytes para valor bruto
    uint32_t raw_pressure_value = combine_pressure_data_bytes(pressure_data_msb, pressure_data_lsb, pressure_data_xlsb);
    
    // Converter para kPa
    *pressure_kilopascal = convert_raw_pressure_to_kilopascal(raw_pressure_value);

    ESP_LOGD(TAG, "Pressão bruta: %lu, Convertida: %.2f kPa", raw_pressure_value, *pressure_kilopascal);
    
    return ESP_OK;
}

uint32_t SMP3011Driver::combine_pressure_data_bytes(uint8_t msb_byte, uint8_t lsb_byte, uint8_t xlsb_byte) {
    // Combinar bytes para valor de 19 bits (mesmo formato BMP280)
    uint32_t combined_value = ((uint32_t)msb_byte << 16) | ((uint32_t)lsb_byte << 8) | xlsb_byte;
    return combined_value >> 5; // Ajustar para 19 bits
}

float SMP3011Driver::convert_raw_pressure_to_kilopascal(uint32_t raw_pressure_value) {
    // Converter valor bruto para kPa usando escala linear
    float pressure_kilopascal = minimum_measurement_pressure_ + (raw_pressure_value * pressure_scale_factor_);
    
    // Garantir que esteja dentro da faixa configurada
    if (pressure_kilopascal < minimum_measurement_pressure_) {
        pressure_kilopascal = minimum_measurement_pressure_;
    } else if (pressure_kilopascal > maximum_measurement_pressure_) {
        pressure_kilopascal = maximum_measurement_pressure_;
    }
    
    return pressure_kilopascal;
}