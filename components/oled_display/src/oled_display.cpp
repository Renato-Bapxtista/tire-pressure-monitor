#include "oled_display.hpp"
#include "esp_log.h"
#include "u8g2.h"
#include "esp_rom_sys.h"

static const char *TAG = "OLEDDisplay";

OLEDDisplay::OLEDDisplay(I2CManager* i2c_manager, uint8_t device_address) 
    : i2c_manager_(i2c_manager), device_address_(device_address), display_initialized_(false) {
    
    // Inicializar estrutura u8g2 com a API mais antiga
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2_display_, U8G2_R0, i2c_byte_callback, gpio_delay_callback);
    
    // Para versões mais antigas do u8g2, usar user_ptr diretamente
    //u8g2_display_.user_ptr = this;
}

OLEDDisplay::~OLEDDisplay() {
    if (display_initialized_) {
        u8g2_SetPowerSave(&u8g2_display_, 1);
        ESP_LOGI(TAG, "Display OLED finalizado");
    }
}

esp_err_t OLEDDisplay::initialize_display() {
    ESP_LOGI(TAG, "Inicializando display OLED no endereço 0x%02X", device_address_);

    // Verificar se o dispositivo está presente
    esp_err_t communication_result = i2c_manager_->probe_device(device_address_);
    if (communication_result != ESP_OK) {
        ESP_LOGE(TAG, "Display OLED não encontrado no endereço 0x%02X", device_address_);
        return communication_result;
    }

    // Inicializar display
    u8g2_InitDisplay(&u8g2_display_);
    u8g2_SetPowerSave(&u8g2_display_, 0);
    u8g2_ClearBuffer(&u8g2_display_);

    // Configurar parâmetros do display
    esp_err_t config_result = configure_display_parameters();
    if (config_result != ESP_OK) {
        ESP_LOGE(TAG, "Falha na configuração do display");
        return config_result;
    }

    display_initialized_ = true;
    
    // Mostrar tela de boas-vindas
    display_welcome_screen();
    
    ESP_LOGI(TAG, "Display OLED inicializado com sucesso");
    return ESP_OK;
}

esp_err_t OLEDDisplay::configure_display_parameters() {
    // Configurar contraste (brilho)
    u8g2_SetContrast(&u8g2_display_, 128);
    
    // Configurar fonte
    u8g2_SetFont(&u8g2_display_, u8g2_font_6x10_tf);
    u8g2_SetFontRefHeightExtendedText(&u8g2_display_);
    u8g2_SetDrawColor(&u8g2_display_, 1);
    u8g2_SetFontPosTop(&u8g2_display_);

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
    
    // Título
    u8g2_SetFont(&u8g2_display_, u8g2_font_7x14B_tf);
    u8g2_DrawStr(&u8g2_display_, 15, 5, "PRESSURE");
    u8g2_DrawStr(&u8g2_display_, 25, 20, "MONITOR");
    
    // Linha divisória
    u8g2_DrawHLine(&u8g2_display_, 10, 35, 108);
    
    // Informações do sistema
    u8g2_SetFont(&u8g2_display_, u8g2_font_5x8_tf);
    u8g2_DrawStr(&u8g2_display_, 20, 45, "ESP32 + BMP280 + SMP3011");
    u8g2_DrawStr(&u8g2_display_, 40, 55, "Inicializando...");
    
    u8g2_SendBuffer(&u8g2_display_);
}

void OLEDDisplay::display_system_status(const char* status_message) {
    if (!display_initialized_) return;

    u8g2_ClearBuffer(&u8g2_display_);
    
    u8g2_SetFont(&u8g2_display_, u8g2_font_7x14B_tf);
    u8g2_DrawStr(&u8g2_display_, 25, 10, "STATUS");
    
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
    
    // Pressão do pneu
    float tire_pressure_bar = tire_pressure_kpa / 100.0f;
    snprintf(display_buffer, sizeof(display_buffer), "Pneu: %.2f bar", tire_pressure_bar);
    draw_bordered_text(5, 44, display_buffer);
    
    // Pressão em PSI também
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
    
    char line_buffer[21] = "";
    
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
    
    // Clamp brightness to valid range (0-100%)
    uint8_t level = brightness_level;
    if (level > 100) level = 100;
    
    // Convert percentage to contrast value (0-255)
    uint8_t contrast = (level * 255) / 100;
    u8g2_SetContrast(&u8g2_display_, contrast);
}

void OLEDDisplay::draw_bordered_text(uint8_t x_position, uint8_t y_position, const char* text_content) {
    u8g2_SetDrawColor(&u8g2_display_, 0);
    u8g2_DrawStr(&u8g2_display_, x_position-1, y_position-1, text_content);
    u8g2_DrawStr(&u8g2_display_, x_position+1, y_position+1, text_content);
    u8g2_SetDrawColor(&u8g2_display_, 1);
    u8g2_DrawStr(&u8g2_display_, x_position, y_position, text_content);
}

void OLEDDisplay::draw_progress_bar(uint8_t x_position, uint8_t y_position, uint8_t width, uint8_t height, uint8_t progress_percentage) {
    if (progress_percentage > 100) progress_percentage = 100;
    
    uint8_t progress_width = (width * progress_percentage) / 100;
    
    u8g2_DrawFrame(&u8g2_display_, x_position, y_position, width, height);
    u8g2_DrawBox(&u8g2_display_, x_position + 1, y_position + 1, progress_width - 2, height - 2);
}

// Callbacks para U8G2 - versão compatível
uint8_t OLEDDisplay::i2c_byte_callback(u8x8_t* u8x8_handle, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    // Usar user_ptr diretamente para versões antigas do u8g2
    //OLEDDisplay* display_instance = (OLEDDisplay*)u8x8_handle->user_ptr;
    OLEDDisplay* display_instance = (OLEDDisplay*)arg_ptr;
    
    
    if (!display_instance) {
        return 0;
    }

    switch (msg) {
        case U8X8_MSG_BYTE_SEND:
            // Enviar dados via I2C
            if (arg_int > 0 && arg_ptr != nullptr) {
                // Usar write_data se disponível, ou implementar escrita direta
                uint8_t* data = (uint8_t*)arg_ptr;
                for (uint8_t i = 0; i < arg_int; i++) {
                    // Enviar byte por byte - implementação simplificada
                    display_instance->i2c_manager_->write_register(
                        display_instance->device_address_, 0x40, data[i]);
                }
            }
            break;
            
        case U8X8_MSG_BYTE_INIT:
            break;
            
        case U8X8_MSG_BYTE_SET_DC:
            break;
            
        case U8X8_MSG_BYTE_START_TRANSFER:
            break;
            
        case U8X8_MSG_BYTE_END_TRANSFER:
            break;
    }
    
    return 1;
}

uint8_t OLEDDisplay::gpio_delay_callback(u8x8_t* u8x8_handle, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            break;
            
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(pdMS_TO_TICKS(arg_int));
            break;
            
        case U8X8_MSG_DELAY_10MICRO:
            // Usar delay de busy wait para microssegundos
            esp_rom_delay_us(arg_int * 10);
            break;
            
        case U8X8_MSG_DELAY_100NANO:
            esp_rom_delay_us(1);
            break;
            
        case U8X8_MSG_DELAY_NANO:
            if (arg_int > 1000) {
                esp_rom_delay_us(arg_int / 1000);
            }
            break;
    }
    
    return 1;
}