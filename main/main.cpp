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
#include "button_driver.hpp"
#include "system_controller.hpp"

// Configurações de hardware
constexpr gpio_num_t I2C0_SDA_PIN = GPIO_NUM_5;
constexpr gpio_num_t I2C0_SCL_PIN = GPIO_NUM_4;
constexpr gpio_num_t I2C1_SDA_PIN = GPIO_NUM_33;
constexpr gpio_num_t I2C1_SCL_PIN = GPIO_NUM_32;

constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
constexpr uint8_t BMP280_I2C_ADDRESS = 0x76;
constexpr uint8_t SMP3011_I2C_ADDRESS = 0x78;

constexpr gpio_num_t BUTTON_UP_PIN = GPIO_NUM_12;
constexpr gpio_num_t BUTTON_DOWN_PIN = GPIO_NUM_14;
constexpr gpio_num_t BUTTON_MODE_PIN = GPIO_NUM_27;

// Instâncias globais
I2CManager i2c0_bus(I2C_NUM_0);
I2CManager i2c1_bus(I2C_NUM_1);

OLEDDisplay status_display(&i2c0_bus, OLED_I2C_ADDRESS);
BMP280Driver environmental_sensor(&i2c1_bus, BMP280_I2C_ADDRESS);
SMP3011Driver tire_pressure_sensor(&i2c1_bus, SMP3011_I2C_ADDRESS);
ButtonDriver button_control(BUTTON_UP_PIN, BUTTON_DOWN_PIN, BUTTON_MODE_PIN);

SystemController system_controller(&button_control, &status_display,
                                  &environmental_sensor, &tire_pressure_sensor);

void scan_i2c_bus(I2CManager& i2c_bus, const char* bus_name) {
    ESP_LOGI("SCAN", "Escaneando barramento %s...", bus_name);
    uint8_t devices_found_count = 0;

    for (uint8_t device_address = 0x08; device_address <= 0x77; device_address++) {
        if (i2c_bus.probe_device(device_address) == ESP_OK) {
            ESP_LOGI("SCAN", "Dispositivo encontrado: 0x%02X", device_address);
            devices_found_count++;
        }
    }

    ESP_LOGI("SCAN", "Scan completo. Dispositivos encontrados: %d", devices_found_count);
}

extern "C" void app_main(void) {
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI("MAIN", "=== SISTEMA DE MEDIÇÃO DE PRESSÃO DE PNEUS ===");

    // Inicializar I2C0 (Display)
    if (i2c0_bus.initialize(I2C0_SDA_PIN, I2C0_SCL_PIN) == ESP_OK) {
        ESP_LOGI("MAIN", "I2C0 (display) inicializado");
        scan_i2c_bus(i2c0_bus, "I2C0");

        // Inicializar display
        if (status_display.initialize_display() == ESP_OK) {
            status_display.display_welcome_screen();
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    // Inicializar I2C1 (Sensores)
    if (i2c1_bus.initialize(I2C1_SDA_PIN, I2C1_SCL_PIN) == ESP_OK) {
        ESP_LOGI("MAIN", "I2C1 (sensores) inicializado");
        scan_i2c_bus(i2c1_bus, "I2C1");

        // Inicializar sensores
        environmental_sensor.initialize_sensor();
        tire_pressure_sensor.initialize_sensor();
        
        // Inicializar botões
        button_control.initialize();
        
        // Inicializar system controller
        system_controller.initialize();

        ESP_LOGI("MAIN", "Sistema totalmente inicializado - Entrando no loop principal");

        // Loop principal
        while (true) {
            system_controller.process_events();
            vTaskDelay(pdMS_TO_TICKS(50)); // 20Hz
        }
    } else {
        ESP_LOGE("MAIN", "Falha crítica na inicialização do I2C1");
    }
}