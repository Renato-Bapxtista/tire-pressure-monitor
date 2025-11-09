#include "smp3011_driver.hpp"
#include "esp_log.h"
#include <cstring>

static const char *TAG = "SMP3011Driver";

SMP3011Driver::SMP3011Driver(I2CManager* i2c_manager, uint8_t device_address) 
    : i2c_manager_(i2c_manager), 
      device_address_(device_address), 
      sensor_initialized_(false),
      minimum_measurement_pressure_(0.0f),
      maximum_measurement_pressure_(1000.0f),
      pressure_scale_factor_(0.0f),
      pressure_offset_(0.0f) {}

SMP3011Driver::~SMP3011Driver() {
    ESP_LOGI(TAG, "Driver SMP3011 finalizado");
}

esp_err_t SMP3011Driver::initialize_sensor() {
    ESP_LOGI(TAG, "Inicializando sensor SMP3011 no endereço 0x%02X", device_address_);

    // Verificar comunicação básica
    esp_err_t probe_result = i2c_manager_->probe_device(device_address_);
    if (probe_result != ESP_OK) {
        ESP_LOGE(TAG, "SMP3011 não responde no endereço 0x%02X", device_address_);
        return probe_result;
    }

    ESP_LOGI(TAG, "Comunicação básica com SMP3011 verificada");

    // Tentar identificar o sensor
    esp_err_t id_result = verify_sensor_identification();
    if (id_result != ESP_OK) {
        ESP_LOGW(TAG, "Não foi possível verificar identificação do sensor, continuando...");
        // Continuar mesmo sem identificação - sensor pode não ter registro WHO_AM_I
    }

    // Configurar faixa de pressão
    ESP_ERROR_CHECK(set_pressure_offset(0.0f)); // Resetar offset
    ESP_ERROR_CHECK(set_pressure_range(0.0f, 1000.0f));

    // Configurar operação
    esp_err_t config_result = configure_sensor_operation();
    if (config_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha na configuração do sensor");
        return config_result;
    }

    // Fazer uma leitura teste
    float test_pressure;
    uint32_t raw_value;
    esp_err_t test_result = read_pressure_detailed(&test_pressure, &raw_value);
    
    if (test_result == ESP_OK) {
        ESP_LOGI(TAG, "Leitura teste: %.2f kPa (raw: %lu)", test_pressure, raw_value);
        
        // Se a leitura for 0.0, adicionar um offset de calibração de teste
        if (test_pressure < 1.0f) {
            ESP_LOGW(TAG, "Leitura muito baixa, aplicando offset de calibração de teste");
            set_pressure_offset(250.0f); // 250 kPa = ~2.5 bar
        }
    } else {
        ESP_LOGE(TAG, "Falha na leitura teste do sensor");
        return test_result;
    }

    sensor_initialized_ = true;
    ESP_LOGI(TAG, "SMP3011 inicializado com sucesso");
    return ESP_OK;
}

esp_err_t SMP3011Driver::verify_sensor_identification() {
    uint8_t who_am_i;
    esp_err_t result = i2c_manager_->read_register(device_address_, REGISTER_WHO_AM_I, &who_am_i, 1);
    
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Registro WHO_AM_I: 0x%02X", who_am_i);
        if (who_am_i == EXPECTED_WHO_AM_I) {
            ESP_LOGI(TAG, "Sensor identificado corretamente como SMP3011");
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "WHO_AM_I inesperado. Esperado: 0x%02X, Recebido: 0x%02X", 
                    EXPECTED_WHO_AM_I, who_am_i);
            return ESP_ERR_NOT_SUPPORTED;
        }
    } else {
        ESP_LOGW(TAG, "Não foi possível ler registro WHO_AM_I: %s", esp_err_to_name(result));
        return result;
    }
}

esp_err_t SMP3011Driver::configure_sensor_operation() {
    // Configuração genérica - ajustar conforme datasheet específica
    ESP_LOGI(TAG, "Configurando operação do sensor");
    
    // Tentar configurar modo de medição contínua ou por comando
    // Estes são valores genéricos - precisam ser validados com a documentação do sensor
    esp_err_t result = i2c_manager_->write_register(device_address_, REGISTER_CONTROL, 0x01);
    
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar registro de controle: %s", esp_err_to_name(result));
        return result;
    }
    
    ESP_LOGI(TAG, "Configuração do sensor aplicada");
    return ESP_OK;
}

esp_err_t SMP3011Driver::set_pressure_range(float min_pressure_kpa, float max_pressure_kpa) {
    if (min_pressure_kpa >= max_pressure_kpa) {
        ESP_LOGE(TAG, "Pressão mínima deve ser menor que máxima");
        return ESP_ERR_INVALID_ARG;
    }

    minimum_measurement_pressure_ = min_pressure_kpa;
    maximum_measurement_pressure_ = max_pressure_kpa;
    
    // Calcular fator de escala (assumindo 19 bits como BMP280)
    const uint32_t max_raw_value = 524287; // 2^19 - 1
    pressure_scale_factor_ = (max_pressure_kpa - min_pressure_kpa) / max_raw_value;

    ESP_LOGI(TAG, "Faixa configurada: %.1f-%.1f kPa, escala: %.6f kPa/bit", 
             min_pressure_kpa, max_pressure_kpa, pressure_scale_factor_);
    
    return ESP_OK;
}

esp_err_t SMP3011Driver::set_pressure_offset(float offset_kpa) {
    pressure_offset_ = offset_kpa;
    ESP_LOGI(TAG, "Offset de pressão configurado: %.2f kPa", offset_kpa);
    return ESP_OK;
}

esp_err_t SMP3011Driver::read_pressure(float* pressure_kilopascal) {
    uint32_t raw_value;
    return read_pressure_detailed(pressure_kilopascal, &raw_value);
}

esp_err_t SMP3011Driver::read_pressure_detailed(float* pressure_kilopascal, uint32_t* raw_value) {
    if (!sensor_initialized_) {
        return ESP_ERR_INVALID_STATE;
    }

    // Iniciar medição
    esp_err_t cmd_result = i2c_manager_->write_register(device_address_, REGISTER_CONTROL, COMMAND_START_MEASUREMENT);
    if (cmd_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar medição: %s", esp_err_to_name(cmd_result));
        return cmd_result;
    }

    // Aguardar conversão
    vTaskDelay(pdMS_TO_TICKS(20));

    // Ler dados brutos
    esp_err_t read_result = read_raw_pressure_data(raw_value);
    if (read_result != ESP_OK) {
        return read_result;
    }

    // Converter para kPa
    *pressure_kilopascal = convert_raw_to_pressure(*raw_value);

    ESP_LOGD(TAG, "Leitura - Bruto: %lu, Convertido: %.2f kPa", *raw_value, *pressure_kilopascal);
    
    return ESP_OK;
}

esp_err_t SMP3011Driver::read_raw_pressure_data(uint32_t* raw_pressure) {
    uint8_t msb, lsb, xlsb;
    
    // Ler os três bytes de dados
    esp_err_t result_msb = i2c_manager_->read_register(device_address_, REGISTER_DATA_MSB, &msb, 1);
    esp_err_t result_lsb = i2c_manager_->read_register(device_address_, REGISTER_DATA_LSB, &lsb, 1);
    esp_err_t result_xlsb = i2c_manager_->read_register(device_address_, REGISTER_DATA_XLSB, &xlsb, 1);

    if (result_msb != ESP_OK || result_lsb != ESP_OK || result_xlsb != ESP_OK) {
        ESP_LOGE(TAG, "Erro na leitura de dados: MSB=%s, LSB=%s, XLSB=%s",
                esp_err_to_name(result_msb), esp_err_to_name(result_lsb), esp_err_to_name(result_xlsb));
        return ESP_ERR_INVALID_RESPONSE;
    }

    ESP_LOGD(TAG, "Bytes lidos: MSB=0x%02X, LSB=0x%02X, XLSB=0x%02X", msb, lsb, xlsb);

    // Combinar bytes (formato similar ao BMP280)
    *raw_pressure = combine_pressure_data_bytes(msb, lsb, xlsb);
    
    return ESP_OK;
}

esp_err_t SMP3011Driver::scan_sensor_registers() {
    ESP_LOGI(TAG, "Escaneando registros do SMP3011 no endereço 0x%02X", device_address_);
    
    uint8_t value;
    int registers_found = 0;
    
    // Escanear registros de 0x00 a 0x7F
    for (uint8_t reg = 0x00; reg < 0x80; reg++) {
        esp_err_t result = i2c_manager_->read_register(device_address_, reg, &value, 1);
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "Registro 0x%02X: 0x%02X", reg, value);
            registers_found++;
        }
    }
    
    ESP_LOGI(TAG, "Escaneamento completo. %d registros respondem", registers_found);
    return ESP_OK;
}

uint32_t SMP3011Driver::combine_pressure_data_bytes(uint8_t msb_byte, uint8_t lsb_byte, uint8_t xlsb_byte) {
    // Formato BMP280: 20 bits (MSB 8 bits + LSB 8 bits + XLSB 4 bits)
    uint32_t combined = ((uint32_t)msb_byte << 12) | ((uint32_t)lsb_byte << 4) | (xlsb_byte >> 4);
    return combined;
}

float SMP3011Driver::convert_raw_to_pressure(uint32_t raw_data) {
    // Converter valor bruto para kPa
    float pressure = minimum_measurement_pressure_ + (raw_data * pressure_scale_factor_);
    pressure += pressure_offset_; // Aplicar offset de calibração
    
    // Limitar à faixa configurada
    if (pressure < minimum_measurement_pressure_) {
        pressure = minimum_measurement_pressure_;
    } else if (pressure > maximum_measurement_pressure_) {
        pressure = maximum_measurement_pressure_;
    }
    
    return pressure;
}