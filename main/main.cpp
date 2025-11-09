#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "i2c_manager.hpp"
#include "bmp280_driver.hpp"
#include "smp3011_driver.hpp"

// Configurações de hardware com nomes descritivos
constexpr gpio_num_t I2C_SDA_PIN = GPIO_NUM_33;
constexpr gpio_num_t I2C_SCL_PIN = GPIO_NUM_32;
constexpr uint8_t BMP280_I2C_ADDRESS = 0x76;
constexpr uint8_t SMP3011_I2C_ADDRESS = 0x78;

// Instâncias globais
I2CManager i2c_bus(I2C_NUM_1);
BMP280Driver environmental_sensor(&i2c_bus, BMP280_I2C_ADDRESS);
SMP3011Driver tire_pressure_sensor(&i2c_bus, SMP3011_I2C_ADDRESS);

extern "C" void app_main(void) {
    // Inicializar NVS
    esp_err_t initialization_result = nvs_flash_init();
    if (initialization_result == ESP_ERR_NVS_NO_FREE_PAGES || initialization_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        initialization_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(initialization_result);

    ESP_LOGI("MAIN", "=== SISTEMA DE MEDIÇÃO DE PRESSÃO DE PNEUS ===");

    // Inicializar barramento I2C
    if (i2c_bus.initialize(I2C_SDA_PIN, I2C_SCL_PIN) == ESP_OK) {
        ESP_LOGI("MAIN", "Barramento I2C inicializado com sucesso");
        
        // Testar scan de dispositivos I2C
        ESP_LOGI("MAIN", "Escaneando dispositivos I2C...");
        uint8_t devices_found_count = 0;
        for (uint8_t device_address = 0x08; device_address <= 0x77; device_address++) {
            if (i2c_bus.probe_device(device_address) == ESP_OK) {
                ESP_LOGI("MAIN", "Dispositivo encontrado: 0x%02X", device_address);
                devices_found_count++;
            }
        }
        ESP_LOGI("MAIN", "Scan completo. Dispositivos encontrados: %d", devices_found_count);

        // Inicializar sensor ambiental BMP280
        if (environmental_sensor.initialize_sensor() == ESP_OK) {
            ESP_LOGI("MAIN", "Sensor ambiental BMP280 inicializado com sucesso");
        } else {
            ESP_LOGE("MAIN", "Falha na inicialização do sensor ambiental BMP280");
        }

        // Inicializar sensor de pressão de pneu SMP3011
        if (tire_pressure_sensor.initialize_sensor() == ESP_OK) {
            ESP_LOGI("MAIN", "Sensor de pressão SMP3011 inicializado com sucesso");
        } else {
            ESP_LOGE("MAIN", "Falha na inicialização do sensor de pressão SMP3011");
        }

        // Verificar se ambos os sensores estão operacionais
        bool environmental_sensor_ready = environmental_sensor.is_sensor_initialized();
        bool tire_sensor_ready = tire_pressure_sensor.is_sensor_initialized();
        
        ESP_LOGI("MAIN", "Status dos sensores - Ambiental: %s, Pneu: %s",
                environmental_sensor_ready ? "PRONTO" : "FALHA",
                tire_sensor_ready ? "PRONTO" : "FALHA");

        // Loop principal de leituras
        uint32_t reading_counter = 0;
        while (true) {
            float current_temperature_celsius;
            float current_atmospheric_pressure_hectopascal;
            float current_tire_pressure_kilopascal;
            
            ESP_LOGI("MAIN", "--- Leitura #%d ---", ++reading_counter);

            // Ler dados do sensor ambiental
            if (environmental_sensor_ready) {
                esp_err_t environmental_read_result = environmental_sensor.read_temperature_and_pressure(
                    &current_temperature_celsius, &current_atmospheric_pressure_hectopascal);
                
                if (environmental_read_result == ESP_OK) {
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
                    // Converter kPa para bar e PSI para referência
                    float tire_pressure_bar = current_tire_pressure_kilopascal / 100.0f;
                    float tire_pressure_psi = current_tire_pressure_kilopascal * 0.145038f;
                    
                    ESP_LOGI("PNEU", "Pressão: %.1f kPa (%.2f bar, %.1f PSI)", 
                            current_tire_pressure_kilopascal, tire_pressure_bar, tire_pressure_psi);
                } else {
                    ESP_LOGE("PNEU", "Erro na leitura: %s", esp_err_to_name(tire_read_result));
                }
            }

            // Calcular pressão relativa se ambos sensores estão funcionando
            if (environmental_sensor_ready && tire_sensor_ready) {
                float atmospheric_pressure_bar = current_atmospheric_pressure_hectopascal / 1000.0f;
                float tire_pressure_bar = current_tire_pressure_kilopascal / 100.0f;
                float relative_pressure_bar = tire_pressure_bar - atmospheric_pressure_bar;
                
                ESP_LOGI("CALCULO", "Pressão Relativa: %.2f bar", relative_pressure_bar);
            }

            ESP_LOGI("MAIN", "Aguardando próxima leitura...");
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    } else {
        ESP_LOGE("MAIN", "Falha crítica na inicialização do barramento I2C");
        
        // Estado de fallback
        while (true) {
            ESP_LOGW("MAIN", "Sistema em estado de recuperação - Verificar hardware I2C");
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
}