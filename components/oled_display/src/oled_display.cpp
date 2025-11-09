#include "oled_display.hpp"
#include "esp_log.h"

static const char *TAG = "OLEDDisplay";

OLEDDisplay::OLEDDisplay(I2CManager* i2c_manager, uint8_t device_address) 
    : i2c_manager_(i2c_manager), device_address_(device_address), display_initialized_(false) {}

OLEDDisplay::~OLEDDisplay() {
    ESP_LOGI(TAG, "Display OLED finalizado");
}

esp_err_t OLEDDisplay::initialize_display() {
    ESP_LOGI(TAG, "Inicializando display OLED no endereço 0x%02X", device_address_);

    // Verificar se o dispositivo está presente
    esp_err_t communication_result = i2c_manager_->probe_device(device_address_);
    if (communication_result != ESP_OK) {
        ESP_LOGE(TAG, "Display OLED não encontrado no endereço 0x%02X", device_address_);
        return communication_result;
    }

    // Para simplificar, vamos usar uma implementação básica sem U8G2 por enquanto
    // Inicialização básica do SSD1306
    uint8_t init_sequence[] = {
        0x00, // Comando
        0xAE, // Display OFF
        0x20, 0x00, // Memory addressing mode = horizontal
        0x21, 0x00, 0x7F, // Column address range
        0x22, 0x00, 0x07, // Page address range
        0xA8, 0x3F, // Mux ratio
        0xD3, 0x00, // Display offset
        0x40, // Display start line
        0xA1, // Segment remap
        0xC8, // COM output scan direction
        0xDA, 0x12, // COM pins hardware configuration
        0x81, 0x7F, // Contrast control
        0xA4, // Entire display ON
        0xA6, // Normal display
        0xD5, 0x80, // Oscillator frequency
        0x8D, 0x14, // Enable charge pump
        0xAF  // Display ON
    };

    // Enviar sequência de inicialização
    for (size_t i = 0; i < sizeof(init_sequence); i++) {
        esp_err_t result = i2c_manager_->write_register(device_address_, 0x00, init_sequence[i]);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Falha ao enviar comando de inicialização: %s", esp_err_to_name(result));
            return result;
        }
    }

    display_initialized_ = true;
    ESP_LOGI(TAG, "Display OLED inicializado com sucesso");
    return ESP_OK;
}

void OLEDDisplay::clear_display() {
    if (!display_initialized_) return;
    
    // Limpar display - enviar zeros para toda a memória
    for (uint8_t page = 0; page < 8; page++) {
        i2c_manager_->write_register(device_address_, 0x00, 0x22); // Set page address
        i2c_manager_->write_register(device_address_, 0x00, page);
        i2c_manager_->write_register(device_address_, 0x00, page);
        
        i2c_manager_->write_register(device_address_, 0x00, 0x21); // Set column address
        i2c_manager_->write_register(device_address_, 0x00, 0);
        i2c_manager_->write_register(device_address_, 0x00, 127);
        
        // Enviar 128 bytes de zeros para esta página
        for (uint8_t col = 0; col < 128; col++) {
            i2c_manager_->write_register(device_address_, 0x40, 0x00);
        }
    }
}

void OLEDDisplay::display_welcome_screen() {
    if (!display_initialized_) return;

    clear_display();
    
    // Implementação básica de texto - vamos mostrar apenas via serial por enquanto
    ESP_LOGI("OLED", "Tela de boas-vindas exibida");
}

void OLEDDisplay::display_system_status(const char* status_message) {
    if (!display_initialized_) return;
    ESP_LOGI("OLED", "Status: %s", status_message);
}

void OLEDDisplay::display_sensor_readings(float temperature_celsius, float atmospheric_pressure_hpa, float tire_pressure_kpa) {
    if (!display_initialized_) return;
    
    ESP_LOGI("OLED", "Leituras - Temp: %.1fC, Atm: %.1fhPa, Pneu: %.1fkPa", 
             temperature_celsius, atmospheric_pressure_hpa, tire_pressure_kpa);
}

void OLEDDisplay::display_error_message(const char* error_message) {
    if (!display_initialized_) return;
    ESP_LOGE("OLED", "Erro: %s", error_message);
}

void OLEDDisplay::update_display_brightness(uint8_t brightness_level) {
    if (!display_initialized_) return;
    
    i2c_manager_->write_register(device_address_, 0x00, 0x81); // Contrast command
    i2c_manager_->write_register(device_address_, 0x00, brightness_level);
}