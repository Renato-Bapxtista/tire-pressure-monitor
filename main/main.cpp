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
#include "task_manager.hpp"
#include "config.hpp"

// Instâncias globais
I2CManager i2c0_bus(I2C_NUM_0);
I2CManager i2c1_bus(I2C_NUM_1);

OLEDDisplay status_display(&i2c0_bus, OLED_I2C_ADDRESS);
BMP280Driver environmental_sensor(&i2c1_bus, BMP280_I2C_ADDRESS);
SMP3011Driver tire_pressure_sensor(&i2c1_bus, SMP3011_I2C_ADDRESS);
ButtonDriver buttons(BUTTON_UP_PIN, BUTTON_DOWN_PIN, BUTTON_MODE_PIN);
TaskManager task_manager;

// Task handles para monitoramento
TaskHandle_t main_monitor_task_handle = nullptr;

void main_monitor_task(void* param) {
    ESP_LOGI("MONITOR", "Main Monitor Task iniciada");
    
    while (true) {
        // Obter handles usando os getters públicos
        TaskHandle_t sensor_handle = task_manager.get_sensor_reader_handle();
        TaskHandle_t button_handle = task_manager.get_button_handler_handle();
        TaskHandle_t display_handle = task_manager.get_display_manager_handle();
        TaskHandle_t system_handle = task_manager.get_system_controller_handle();
        TaskHandle_t power_handle = task_manager.get_power_manager_handle();
        
        UBaseType_t sensor_stack = 0, button_stack = 0, display_stack = 0, system_stack = 0, power_stack = 0;
        
        // Verificar se todos os handles são válidos antes de usar
        if (sensor_handle != nullptr) {
            sensor_stack = uxTaskGetStackHighWaterMark(sensor_handle);
        }
        if (button_handle != nullptr) {
            button_stack = uxTaskGetStackHighWaterMark(button_handle);
        }
        if (display_handle != nullptr) {
            display_stack = uxTaskGetStackHighWaterMark(display_handle);
        }
        if (system_handle != nullptr) {
            system_stack = uxTaskGetStackHighWaterMark(system_handle);
        }
        if (power_handle != nullptr) {
            power_stack = uxTaskGetStackHighWaterMark(power_handle);
        }
        
        ESP_LOGI("MONITOR", "Stack usage - Sensor: %lu, Button: %lu, Display: %lu, System: %lu, Power: %lu",
                sensor_stack, button_stack, display_stack, system_stack, power_stack);
        
        // Monitorar uso de CPU (aproximado)
        ESP_LOGI("MONITOR", "Free heap: %lu bytes", esp_get_free_heap_size());
        
        vTaskDelay(pdMS_TO_TICKS(10000)); // Verificar a cada 10 segundos
    }
}

extern "C" void app_main(void) {
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI("MAIN", "=== SISTEMA FREERTOS - MEDIÇÃO DE PRESSÃO DE PNEUS ===");
    ESP_LOGI("MAIN", "ESP-IDF Version: %s", esp_get_idf_version());
    ESP_LOGI("MAIN", "FreeRTOS Kernel Version: %s", tskKERNEL_VERSION_NUMBER);

    // Inicializar hardware
    ESP_LOGI("MAIN", "Inicializando hardware...");
    
    // I2C0 - Display
    if (i2c0_bus.initialize(I2C0_SDA_PIN, I2C0_SCL_PIN) == ESP_OK) {
        ESP_LOGI("MAIN", "I2C0 (display) inicializado");
        
        if (status_display.initialize_display() == ESP_OK) {
            ESP_LOGI("MAIN", "Display OLED inicializado");
            status_display.display_welcome_screen();
        }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    // I2C1 - Sensores
    if (i2c1_bus.initialize(I2C1_SDA_PIN, I2C1_SCL_PIN) == ESP_OK) {
        ESP_LOGI("MAIN", "I2C1 (sensores) inicializado");

        if (environmental_sensor.initialize_sensor() == ESP_OK) {
            ESP_LOGI("MAIN", "BMP280 inicializado");
        }

        if (tire_pressure_sensor.initialize_sensor() == ESP_OK) {
            ESP_LOGI("MAIN", "SMP3011 inicializado");
        }
    }

    // Botões
    if (buttons.initialize() == ESP_OK) {
        ESP_LOGI("MAIN", "Botões inicializados");
    }

    // Task Manager (FreeRTOS)
    if (task_manager.initialize() == ESP_OK) {
        ESP_LOGI("MAIN", "Task Manager FreeRTOS inicializado");
    }

    // Criar task de monitoramento
    xTaskCreate(
        main_monitor_task,
        "main_monitor",
        2048,
        nullptr,
        1,  // Prioridade baixa
        &main_monitor_task_handle
    );

    ESP_LOGI("MAIN", "=== SISTEMA INICIALIZADO COM SUCESSO ===");
    ESP_LOGI("MAIN", "Tasks FreeRTOS em execução:");
    ESP_LOGI("MAIN", "  - Sensor Reader");
    ESP_LOGI("MAIN", "  - Button Handler"); 
    ESP_LOGI("MAIN", "  - Display Manager");
    ESP_LOGI("MAIN", "  - System Controller");
    ESP_LOGI("MAIN", "  - Power Manager");
    ESP_LOGI("MAIN", "  - Main Monitor");

    // Task principal fica bloqueada - o sistema é gerenciado pelas tasks FreeRTOS
    while (true) {
        ESP_LOGI("MAIN", "Sistema FreeRTOS executando...");
        vTaskDelay(pdMS_TO_TICKS(30000)); // Log a cada 30 segundos
    }
}