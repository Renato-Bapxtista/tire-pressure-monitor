#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "i2c_manager.hpp"
#include "bmp280_driver.hpp"

// Configurações de hardware com nomes descritivos
constexpr gpio_num_t I2C_SDA_PIN = GPIO_NUM_33;
constexpr gpio_num_t I2C_SCL_PIN = GPIO_NUM_32;
constexpr uint8_t BMP280_I2C_ADDRESS = 0x76;

// Instâncias globais
I2CManager i2c_bus(I2C_NUM_1);
BMP280Driver environmental_sensor(&i2c_bus, BMP280_I2C_ADDRESS);

extern "C" void app_main(void) {
    // Inicializar NVS
    esp_err_t initialization_result = nvs_flash_init();
    if (initialization_result == ESP_ERR_NVS_NO_FREE_PAGES || initialization_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        initialization_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(initialization_result);

    ESP_LOGI("MAIN", "=== SISTEMA DE TESTE BMP280 ===");

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

        // Inicializar sensor BMP280
        if (environmental_sensor.initialize_sensor() == ESP_OK) {
            ESP_LOGI("MAIN", "Sensor BMP280 inicializado com sucesso");

            // Loop principal de leituras
            uint32_t reading_counter = 0;
            while (true) {
                float current_temperature;
                float current_pressure;
                
                esp_err_t read_result = environmental_sensor.read_temperature_and_pressure(&current_temperature, &current_pressure);
                
                if (read_result == ESP_OK) {
                    ESP_LOGI("SENSOR", "Leitura #%d: Temperatura = %.2f°C, Pressão = %.2f hPa", 
                            ++reading_counter, current_temperature, current_pressure);
                } else {
                    ESP_LOGE("SENSOR", "Erro na leitura #%d: %s", ++reading_counter, esp_err_to_name(read_result));
                }
                
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
        } else {
            ESP_LOGE("MAIN", "Falha na inicialização do sensor BMP280");
        }
    } else {
        ESP_LOGE("MAIN", "Falha na inicialização do barramento I2C");
    }

    // Estado de fallback em caso de erro
    while (true) {
        ESP_LOGW("MAIN", "Sistema em estado de recuperação");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}