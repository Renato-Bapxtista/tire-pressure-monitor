#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "i2c_manager.hpp"
#include "bmp280_driver.hpp"
#include "smp3011_driver.hpp"
#include "oled_display.hpp"
#include "config.hpp"

// Instâncias globais para DOIS barramentos I2C
I2CManager i2c0_bus(I2C_NUM_0);  // Para display OLED
I2CManager i2c1_bus(I2C_NUM_1);  // Para sensores

OLEDDisplay status_display(&i2c0_bus, OLED_I2C_ADDRESS);
BMP280Driver environmental_sensor(&i2c1_bus, BMP280_I2C_ADDRESS);
SMP3011Driver tire_pressure_sensor(&i2c1_bus, SMP3011_I2C_ADDRESS);

void scan_i2c_bus(I2CManager& i2c_bus, const char* bus_name) {
    ESP_LOGI("SCAN", "Escaneando barramento %s...", bus_name);
    uint8_t devices_found_count = 0;
    
    for (uint8_t device_address = 0x08; device_address <= 0x77; device_address++) {
        if (i2c_bus.probe_device(device_address) == ESP_OK) {
            ESP_LOGI("SCAN", "Dispositivo encontrado no barramento %s: 0x%02X", 
                    bus_name, device_address);
            devices_found_count++;
        }
    }
    
    ESP_LOGI("SCAN", "Scan %s completo. Dispositivos encontrados: %d", 
            bus_name, devices_found_count);
}

extern "C" void app_main(void) {
    // Inicializar NVS
    esp_err_t initialization_result = nvs_flash_init();
    if (initialization_result == ESP_ERR_NVS_NO_FREE_PAGES || initialization_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        initialization_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(initialization_result);

    ESP_LOGI("MAIN", "=== SISTEMA DE MEDIÇÃO DE PRESSÃO DE PNEUS ===");
    ESP_LOGI("MAIN", "Configuração: 2 barramentos I2C separados");

    // Inicializar barramento I2C0 para display OLED
    ESP_LOGI("MAIN", "Inicializando I2C0 (Display OLED) - SDA: GPIO%d, SCL: GPIO%d", 
            I2C0_SDA_PIN, I2C0_SCL_PIN);
    
    if (i2c0_bus.initialize(I2C0_SDA_PIN, I2C0_SCL_PIN) == ESP_OK) {
        ESP_LOGI("MAIN", "Barramento I2C0 (display) inicializado com sucesso");
        
        // Scan I2C0 para display
        scan_i2c_bus(i2c0_bus, "I2C0 (Display)");
        
        // Inicializar display OLED
        if (status_display.initialize_display() == ESP_OK) {
            ESP_LOGI("MAIN", "Display OLED inicializado com sucesso");
            status_display.display_welcome_screen();
        } else {
            ESP_LOGE("MAIN", "Falha na inicialização do display OLED");
            // Continuar sem display
        }
    } else {    
        ESP_LOGE("MAIN", "Falha crítica na inicialização do barramento I2C0 (display)");
    }

    // Aguardar um pouco para mostrar tela de boas-vindas
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Inicializar barramento I2C1 para sensores
    ESP_LOGI("MAIN", "Inicializando I2C1 (Sensores) - SDA: GPIO%d, SCL: GPIO%d", 
            I2C1_SDA_PIN, I2C1_SCL_PIN);
    
    if (i2c1_bus.initialize(I2C1_SDA_PIN, I2C1_SCL_PIN) == ESP_OK) {
        ESP_LOGI("MAIN", "Barramento I2C1 (sensores) inicializado com sucesso");
        
        // Scan I2C1 para sensores
        scan_i2c_bus(i2c1_bus, "I2C1 (Sensores)");

        // Inicializar sensor ambiental BMP280
        bool environmental_sensor_ready = false;
        if (environmental_sensor.initialize_sensor() == ESP_OK) {
            ESP_LOGI("MAIN", "Sensor ambiental BMP280 inicializado com sucesso");
            environmental_sensor_ready = true;
        } else {
            ESP_LOGE("MAIN", "Falha na inicialização do sensor ambiental BMP280");
            status_display.display_error_message("Erro: Sensor BMP280 falhou");
        }

        // Inicializar sensor de pressão de pneu SMP3011
        bool tire_sensor_ready = false;
        if (tire_pressure_sensor.initialize_sensor() == ESP_OK) {
            ESP_LOGI("MAIN", "Sensor de pressão SMP3011 inicializado com sucesso");
            tire_sensor_ready = true;
        } else {
            ESP_LOGE("MAIN", "Falha na inicialização do sensor de pressão SMP3011");
            status_display.display_error_message("Erro: Sensor SMP3011 falhou");
        }

        // Mostrar status dos sensores no display
        if (environmental_sensor_ready && tire_sensor_ready) {
            status_display.display_system_status("Sensores OK");
        } else if (environmental_sensor_ready) {
            status_display.display_system_status("Aguardando SMP3011");
        } else if (tire_sensor_ready) {
            status_display.display_system_status("Aguardando BMP280");
        } else {
            status_display.display_error_message("Todos sensores falharam");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));

        // Loop principal de leituras
        uint32_t reading_counter = 0;
        while (true) {
            float current_temperature_celsius = 0.0f;
            float current_atmospheric_pressure_hectopascal = 0.0f;
            float current_tire_pressure_kilopascal = 0.0f;
            
            bool environmental_read_success = false;
            bool tire_read_success = false;

            ESP_LOGI("MAIN", "--- Leitura #%d ---", ++reading_counter);

            // Ler dados do sensor ambiental
            if (environmental_sensor_ready) {
                esp_err_t environmental_read_result = environmental_sensor.read_temperature_and_pressure(
                    &current_temperature_celsius, &current_atmospheric_pressure_hectopascal);
                
                if (environmental_read_result == ESP_OK) {
                    environmental_read_success = true;
                    ESP_LOGI("AMBIENTE", "Temperatura: %.2f°C, Pressão Atmosférica: %.2f hPa", 
                            current_temperature_celsius, current_atmospheric_pressure_hectopascal);
                } else {
                    ESP_LOGE("AMBIENTE", "Erro na leitura: %s", esp_err_to_name(environmental_read_result));
                }
            }

            // Ler dados do sensor de pneu
            if (tire_sensor_ready) {
                esp_err_t tire_read_result = tire_pressure_sensor.read_pressure(&current_tire_pressure_kilopascal);
                
                if (tire_read_result == ESP_OK) {
                    tire_read_success = true;
                    
                    // Converter kPa para bar e PSI para referência
                    float tire_pressure_bar = current_tire_pressure_kilopascal / 100.0f;
                    float tire_pressure_psi = current_tire_pressure_kilopascal * 0.145038f;
                    
                    ESP_LOGI("PNEU", "Pressão: %.1f kPa (%.2f bar, %.1f PSI)", 
                            current_tire_pressure_kilopascal, tire_pressure_bar, tire_pressure_psi);
                } else {
                    ESP_LOGE("PNEU", "Erro na leitura: %s", esp_err_to_name(tire_read_result));
                }
            }

            // Atualizar display com as leituras
            if (environmental_read_success && tire_read_success) {
                status_display.display_sensor_readings(
                    current_temperature_celsius,
                    current_atmospheric_pressure_hectopascal,
                    current_tire_pressure_kilopascal
                );
            } else if (!environmental_read_success && !tire_read_success) {
                status_display.display_error_message("Falha em ambos sensores");
            } else if (!environmental_read_success) {
                status_display.display_error_message("Falha no BMP280");
            } else if (!tire_read_success) {
                status_display.display_error_message("Falha no SMP3011");
            }

            // Calcular pressão relativa se ambos sensores estão funcionando
            if (environmental_read_success && tire_read_success) {
                float atmospheric_pressure_bar = current_atmospheric_pressure_hectopascal / 1000.0f;
                float tire_pressure_bar = current_tire_pressure_kilopascal / 100.0f;
                float relative_pressure_bar = tire_pressure_bar - atmospheric_pressure_bar;
                
                ESP_LOGI("CALCULO", "Pressão Relativa: %.2f bar", relative_pressure_bar);
            }

            ESP_LOGI("MAIN", "Aguardando próxima leitura...");
            vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
        }
    } else {
        ESP_LOGE("MAIN", "Falha crítica na inicialização do barramento I2C1 (sensores)");
        status_display.display_error_message("Falha I2C Sensores");
        
        // Estado de fallback
        while (true) {
            ESP_LOGW("MAIN", "Sistema em estado de recuperação - Verificar hardware I2C1");
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
}