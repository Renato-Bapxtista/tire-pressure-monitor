#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "i2c_manager.hpp"

// Definições de pinos para teste
#define I2C1_SDA    GPIO_NUM_33
#define I2C1_SCL    GPIO_NUM_32

// Instância global
I2CManager i2c1(I2C_NUM_1);

extern "C" void app_main(void) {
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI("MAIN", "=== TESTE I2C MANAGER ===");

    // Inicializar I2C
    if (i2c1.initialize(I2C1_SDA, I2C1_SCL) == ESP_OK) {
        ESP_LOGI("MAIN", "I2C inicializado com sucesso");
        
        // Testar scan de dispositivos I2C
        ESP_LOGI("MAIN", "Escaneando dispositivos I2C...");
        int found_devices = 0;
        for (uint8_t address = 0x08; address <= 0x77; address++) {
            if (i2c1.probe_device(address) == ESP_OK) {
                ESP_LOGI("MAIN", "Dispositivo encontrado: 0x%02X", address);
                found_devices++;
            }
        }
        ESP_LOGI("MAIN", "Scan completo. Dispositivos encontrados: %d", found_devices);
    } else {
        ESP_LOGE("MAIN", "Falha na inicialização do I2C");
    }

    // Loop principal
    int counter = 0;
    while (1) {
        ESP_LOGI("MAIN", "Sistema ativo - ciclo %d", ++counter);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}