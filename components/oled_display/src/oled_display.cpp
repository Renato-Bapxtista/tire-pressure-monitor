#include "oled_display.hpp"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "OLEDDisplay";

// Sequência de inicialização do SSD1306
static const uint8_t INIT_COMMANDS[] = {
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

OLEDDisplay::OLEDDisplay(I2CManager* i2c_manager, uint8_t device_address) 
    : i2c_manager_(i2c_manager), device_address_(device_address), display_initialized_(false) {}

OLEDDisplay::~OLEDDisplay() {
    if (display_initialized_) {
        send_command(0xAE); // Display OFF
        ESP_LOGI(TAG, "Display OLED finalizado");
    }
}

esp_err_t OLEDDisplay::send_command(uint8_t command) {
    uint8_t buffer[2] = {0x00, command}; // 0x00 = command mode
    return i2c_manager_->write_register(device_address_, buffer[0], buffer[1]);
}

esp_err_t OLEDDisplay::send_data(const uint8_t* data, size_t length) {
    esp_err_t result = ESP_OK;
    for (size_t i = 0; i < length; i++) {
        result = i2c_manager_->write_register(device_address_, 0x40, data[i]); // 0x40 = data mode
        if (result != ESP_OK) break;
    }
    return result;
}

esp_err_t OLEDDisplay::send_command_sequence(const uint8_t* commands, size_t length) {
    esp_err_t result = ESP_OK;
    for (size_t i = 0; i < length; i++) {
        result = send_command(commands[i]);
        if (result != ESP_OK) break;
        vTaskDelay(pdMS_TO_TICKS(1)); // Pequeno delay entre comandos
    }
    return result;
}

esp_err_t OLEDDisplay::initialize_display() {
    ESP_LOGI(TAG, "Inicializando display OLED SSD1306 no endereço 0x%02X", device_address_);

    // Verificar se o dispositivo está presente
    esp_err_t probe_result = i2c_manager_->probe_device(device_address_);
    if (probe_result != ESP_OK) {
        ESP_LOGE(TAG, "Display OLED não encontrado no endereço 0x%02X", device_address_);
        return probe_result;
    }

    // Enviar sequência de inicialização
    esp_err_t init_result = send_command_sequence(INIT_COMMANDS, sizeof(INIT_COMMANDS));
    if (init_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha na inicialização do display OLED");
        return init_result;
    }

    // Limpar display
    clear_display();

    display_initialized_ = true;
    ESP_LOGI(TAG, "Display OLED inicializado com sucesso");
    return ESP_OK;
}

void OLEDDisplay::clear_display() {
    if (!display_initialized_) return;

    // Limpar toda a memória do display (128x64 pixels = 1024 bytes)
    uint8_t zero_buffer[128] = {0};
    
    for (uint8_t page = 0; page < 8; page++) {
        // Configurar endereço de página
        send_command(0xB0 + page); // Set page address
        send_command(0x00);        // Set lower column address
        send_command(0x10);        // Set higher column address
        
        // Enviar 128 bytes de zeros para esta página
        send_data(zero_buffer, 128);
    }
}

void OLEDDisplay::draw_text(uint8_t x, uint8_t y, const char* text) {
    if (!display_initialized_ || text == nullptr) return;

    // Implementação básica de texto - apenas para demonstração
    // Em uma implementação real, precisaríamos de uma fonte
    ESP_LOGI(TAG, "Texto no display [%d,%d]: %s", x, y, text);
}

void OLEDDisplay::draw_horizontal_line(uint8_t x, uint8_t y, uint8_t length) {
    if (!display_initialized_) return;

    // Implementação básica de linha horizontal
    // Configurar posição
    send_command(0xB0 + (y / 8)); // Page address
    send_command(0x00 + (x & 0x0F)); // Lower column address
    send_command(0x10 + ((x >> 4) & 0x0F)); // Higher column address
    
    // Desenhar linha (cada bit representa um pixel)
    uint8_t line_data[128];
    memset(line_data, 0xFF, length); // Todos os pixels ligados
    send_data(line_data, length);
}

void OLEDDisplay::display_welcome_screen() {
    if (!display_initialized_) return;

    clear_display();
    
    // Tela de boas-vindas simples
    // Usar comandos básicos para mostrar algo
    ESP_LOGI(TAG, "Exibindo tela de boas-vindas no OLED");
    
    // Desenhar borda
    draw_horizontal_line(0, 0, 128);
    draw_horizontal_line(0, 63, 128);
    
    // Mostrar via serial que o display está funcionando
    ESP_LOGI("OLED", "=== MEDIDOR DE PRESSAO ===");
    ESP_LOGI("OLED", "Sistema Inicializado");
    ESP_LOGI("OLED", "Aguardando sensores...");
}

void OLEDDisplay::display_system_status(const char* status_message) {
    if (!display_initialized_) return;
    
    ESP_LOGI("OLED", "Status: %s", status_message);
}

void OLEDDisplay::display_sensor_readings(float temperature_celsius, float atmospheric_pressure_hpa, float tire_pressure_kpa) {
    if (!display_initialized_) return;
    
    char buffer[64];
    
    // Limpar área de dados (mantendo bordas)
    clear_display();
    draw_horizontal_line(0, 0, 128);
    draw_horizontal_line(0, 63, 128);
    
    // Mostrar dados via serial (enquanto não implementamos texto gráfico)
    snprintf(buffer, sizeof(buffer), "Temp: %.1f C", temperature_celsius);
    ESP_LOGI("OLED", "%s", buffer);
    
    snprintf(buffer, sizeof(buffer), "Atm: %.1f hPa", atmospheric_pressure_hpa);
    ESP_LOGI("OLED", "%s", buffer);
    
    float tire_pressure_bar = tire_pressure_kpa / 100.0f;
    snprintf(buffer, sizeof(buffer), "Pneu: %.2f bar", tire_pressure_bar);
    ESP_LOGI("OLED", "%s", buffer);
    
    float tire_pressure_psi = tire_pressure_kpa * 0.145038f;
    snprintf(buffer, sizeof(buffer), "Pneu: %.1f PSI", tire_pressure_psi);
    ESP_LOGI("OLED", "%s", buffer);
}

void OLEDDisplay::display_error_message(const char* error_message) {
    if (!display_initialized_) return;
    
    ESP_LOGE("OLED", "ERRO: %s", error_message);
}