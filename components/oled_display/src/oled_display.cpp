#include "oled_display.hpp"
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "OLEDDisplay";

// Inicializar membro estático
OLEDDisplay* OLEDDisplay::instance_ = nullptr;

OLEDDisplay::OLEDDisplay(I2CManager* i2c_manager, uint8_t device_address) 
    : i2c_manager_(i2c_manager), device_address_(device_address), display_initialized_(false) {
    
    instance_ = this;
    
    // Configurar U8G2 para SSD1306 128x64 via I2C
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2_display_, U8G2_R0, i2c_byte_callback, gpio_delay_callback);
    
    
}

OLEDDisplay::~OLEDDisplay() {
    if (display_initialized_) {
        u8g2_SetPowerSave(&u8g2_display_, 1); // Power save mode
        ESP_LOGI(TAG, "Display OLED finalizado");
    }
    instance_ = nullptr;
}

esp_err_t OLEDDisplay::initialize_display() {
    ESP_LOGI(TAG, "Inicializando display OLED SSD1306 no endereço 0x%02X", device_address_);

    // Verificar se o dispositivo está presente
    esp_err_t probe_result = i2c_manager_->probe_device(device_address_);
    if (probe_result != ESP_OK) {
        ESP_LOGE(TAG, "Display OLED não encontrado no endereço 0x%02X", device_address_);
        return probe_result;
    }

    // Inicializar display usando U8G2
    u8g2_InitDisplay(&u8g2_display_);
    u8g2_SetPowerSave(&u8g2_display_, 0); // Desligar power save
    u8g2_ClearBuffer(&u8g2_display_);

    // Configurar parâmetros do display
    u8g2_SetContrast(&u8g2_display_, 200);
    u8g2_SetFontMode(&u8g2_display_, 1); // Transparente

    display_initialized_ = true;
    
    // Mostrar tela de inicialização
    display_welcome_screen();
    
    ESP_LOGI(TAG, "Display OLED inicializado com sucesso");
    return ESP_OK;
}

void OLEDDisplay::clear_display() {
    if (!display_initialized_) return;
    
    u8g2_ClearBuffer(&u8g2_display_);
    u8g2_SendBuffer(&u8g2_display_);
}

void OLEDDisplay::display_welcome_screen() {
    if (!display_initialized_) return;

    u8g2_ClearBuffer(&u8g2_display_);
    
    // Título principal
    u8g2_SetFont(&u8g2_display_, u8g2_font_7x14B_tf);
    u8g2_DrawStr(&u8g2_display_, 20, 8, "MEDIDOR");
    u8g2_DrawStr(&u8g2_display_, 25, 23, "PRESSAO");
    
    // Linha divisória
    u8g2_DrawHLine(&u8g2_display_, 10, 35, 108);
    
    // Informações do sistema
    u8g2_SetFont(&u8g2_display_, u8g2_font_5x8_tf);
    u8g2_DrawStr(&u8g2_display_, 15, 45, "ESP32 + BMP280");
    u8g2_DrawStr(&u8g2_display_, 25, 55, "SMP3011 + OLED");
    
    u8g2_SendBuffer(&u8g2_display_);
}

void OLEDDisplay::display_system_status(const char* status_message) {
    if (!display_initialized_) return;

    u8g2_ClearBuffer(&u8g2_display_);
    
    u8g2_SetFont(&u8g2_display_, u8g2_font_7x14B_tf);
    u8g2_DrawStr(&u8g2_display_, 35, 10, "STATUS");
    
    u8g2_SetFont(&u8g2_display_, u8g2_font_6x10_tf);
    u8g2_DrawStr(&u8g2_display_, 10, 35, status_message);
    
    u8g2_SendBuffer(&u8g2_display_);
}

void OLEDDisplay::display_sensor_readings(float temperature_celsius, float atmospheric_pressure_hpa, float tire_pressure_kpa) {
    if (!display_initialized_) return;

    char display_buffer[32];
    
    u8g2_ClearBuffer(&u8g2_display_);
    
    // Cabeçalho
    u8g2_SetFont(&u8g2_display_, u8g2_font_7x14B_tf);
    u8g2_DrawStr(&u8g2_display_, 35, 0, "LEITURAS");
    
    // Linha divisória
    u8g2_DrawHLine(&u8g2_display_, 5, 15, 118);
    
    u8g2_SetFont(&u8g2_display_, u8g2_font_6x10_tf);
    
    // Temperatura
    snprintf(display_buffer, sizeof(display_buffer), "Temp: %.1f C", temperature_celsius);
    u8g2_DrawStr(&u8g2_display_, 5, 20, display_buffer);
    
    // Pressão atmosférica
    snprintf(display_buffer, sizeof(display_buffer), "Atm: %.1f hPa", atmospheric_pressure_hpa);
    u8g2_DrawStr(&u8g2_display_, 5, 32, display_buffer);
    
    // Pressão do pneu em bar
    float tire_pressure_bar = tire_pressure_kpa / 100.0f;
    snprintf(display_buffer, sizeof(display_buffer), "Pneu: %.2f bar", tire_pressure_bar);
    u8g2_DrawStr(&u8g2_display_, 5, 44, display_buffer);
    
    // Pressão do pneu em PSI
    float tire_pressure_psi = tire_pressure_kpa * 0.145038f;
    snprintf(display_buffer, sizeof(display_buffer), "Pneu: %.1f PSI", tire_pressure_psi);
    u8g2_DrawStr(&u8g2_display_, 5, 56, display_buffer);
    
    u8g2_SendBuffer(&u8g2_display_);
}

void OLEDDisplay::display_error_message(const char* error_message) {
    if (!display_initialized_) return;

    u8g2_ClearBuffer(&u8g2_display_);
    
    u8g2_SetFont(&u8g2_display_, u8g2_font_7x14B_tf);
    u8g2_DrawStr(&u8g2_display_, 40, 5, "ERRO");
    
    u8g2_SetFont(&u8g2_display_, u8g2_font_5x8_tf);
    
    // Quebrar mensagem longa em múltiplas linhas
    char buffer[64];
    strncpy(buffer, error_message, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* token = strtok(buffer, " ");
    uint8_t line_position = 25;
    char line_buffer[21] = ""; // 20 caracteres por linha
    
    while (token != NULL && line_position < 60) {
        if (strlen(line_buffer) + strlen(token) + 1 < sizeof(line_buffer)) {
            if (line_buffer[0] != '\0') strcat(line_buffer, " ");
            strcat(line_buffer, token);
        } else {
            u8g2_DrawStr(&u8g2_display_, 5, line_position, line_buffer);
            line_position += 10;
            strcpy(line_buffer, token);
        }
        token = strtok(NULL, " ");
    }
    
    if (line_buffer[0] != '\0') {
        u8g2_DrawStr(&u8g2_display_, 5, line_position, line_buffer);
    }
    
    u8g2_SendBuffer(&u8g2_display_);
}

void OLEDDisplay::update_display_brightness(uint8_t brightness_level) {
    if (!display_initialized_) return;
    
    if (brightness_level > 255) brightness_level = 255;
    u8g2_SetContrast(&u8g2_display_, brightness_level);
}

// Callbacks para U8G2
uint8_t OLEDDisplay::i2c_byte_callback(u8x8_t* u8x8_handle, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    //OLEDDisplay* display_instance = (OLEDDisplay*)u8x8_GetUserPtr(u8x8_handle);
    OLEDDisplay* display_instance = (OLEDDisplay*)arg_ptr;
    
    switch (msg) {
        case U8X8_MSG_BYTE_SEND: {
            // Enviar múltiplos bytes de dados
            uint8_t* data = (uint8_t*)arg_ptr;
            for (uint8_t i = 0; i < arg_int; i++) {
                // Para SSD1306, enviamos dados para registro 0x40
                display_instance->i2c_manager_->write_register(
                    display_instance->device_address_, 0x40, data[i]);
            }
            break;
        }
            
        case U8X8_MSG_BYTE_INIT:
            // Inicialização - já feita no I2CManager
            break;
            
        case U8X8_MSG_BYTE_SET_DC:
            // Para I2C, o bit DC é enviado como parte do endereço
            // 0x00 = comando, 0x40 = dados
            break;
            
        case U8X8_MSG_BYTE_START_TRANSFER:
            // Início da transferência - enviar byte de controle
            // arg_int contém o byte de controle (0x00 para comando, 0x40 para dados)
            display_instance->i2c_manager_->write_register(
                display_instance->device_address_, 0x00, arg_int);
            break;
            
        case U8X8_MSG_BYTE_END_TRANSFER:
            // Fim da transferência - nada a fazer
            break;
    }
    
    return 1;
}

uint8_t OLEDDisplay::gpio_delay_callback(u8x8_t* u8x8_handle, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            // Inicialização - não necessária para I2C puro
            break;
            
        case U8X8_MSG_DELAY_MILLI:
            // Delay em milissegundos
            vTaskDelay(pdMS_TO_TICKS(arg_int));
            break;
            
        case U8X8_MSG_DELAY_10MICRO:
            // Delay em microssegundos (aproximado)
            esp_rom_delay_us(arg_int * 10);
            break;
            
        case U8X8_MSG_DELAY_100NANO:
            // Delay em nanossegundos (mínimo)
            esp_rom_delay_us(1);
            break;
    }
    
    return 1;
}