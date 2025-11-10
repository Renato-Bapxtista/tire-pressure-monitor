#include "task_manager.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h" 
#include "esp_random.h" 

static const char *TAG = "TaskManager";

TaskManager::TaskManager()
    : sensor_data_queue_(nullptr),
      display_command_queue_(nullptr),
      button_event_queue_(nullptr),
      i2c_mutex_(nullptr),
      display_mutex_(nullptr),
      sensor_reader_handle_(nullptr),
      button_handler_handle_(nullptr),
      display_manager_handle_(nullptr),
      system_controller_handle_(nullptr),
      power_manager_handle_(nullptr) {}

TaskManager::~TaskManager() {
    // Não destruímos as tasks aqui - o sistema as gerencia
    if (sensor_data_queue_) vQueueDelete(sensor_data_queue_);
    if (display_command_queue_) vQueueDelete(display_command_queue_);
    if (button_event_queue_) vQueueDelete(button_event_queue_);
    if (i2c_mutex_) vSemaphoreDelete(i2c_mutex_);
    if (display_mutex_) vSemaphoreDelete(display_mutex_);
}

esp_err_t TaskManager::initialize() {
    ESP_LOGI(TAG, "Inicializando Task Manager FreeRTOS");

    // Criar queues
    sensor_data_queue_ = xQueueCreate(QUEUE_SIZE, sizeof(SensorData));
    display_command_queue_ = xQueueCreate(QUEUE_SIZE, sizeof(DisplayCommand));
    button_event_queue_ = xQueueCreate(QUEUE_SIZE, sizeof(ButtonEvent));

    if (!sensor_data_queue_ || !display_command_queue_ || !button_event_queue_) {
        ESP_LOGE(TAG, "Falha ao criar queues");
        return ESP_ERR_NO_MEM;
    }

    // Criar mutexes
    i2c_mutex_ = xSemaphoreCreateMutex();
    display_mutex_ = xSemaphoreCreateMutex();

    if (!i2c_mutex_ || !display_mutex_) {
        ESP_LOGE(TAG, "Falha ao criar mutexes");
        return ESP_ERR_NO_MEM;
    }

    // Criar tasks
    BaseType_t result;

    result = xTaskCreate(
        sensor_reader_task,
        "sensor_reader",
        SENSOR_TASK_STACK_SIZE,
        this,
        SENSOR_TASK_PRIORITY,
        &sensor_reader_handle_
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar task sensor_reader");
        return ESP_ERR_NO_MEM;
    }

    result = xTaskCreate(
        button_handler_task,
        "button_handler",
        BUTTON_TASK_STACK_SIZE,
        this,
        BUTTON_TASK_PRIORITY,
        &button_handler_handle_
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar task button_handler");
        return ESP_ERR_NO_MEM;
    }

    result = xTaskCreate(
        display_manager_task,
        "display_manager",
        DISPLAY_TASK_STACK_SIZE,
        this,
        DISPLAY_TASK_PRIORITY,
        &display_manager_handle_
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar task display_manager");
        return ESP_ERR_NO_MEM;
    }

    result = xTaskCreate(
        system_controller_task,
        "system_controller",
        SYSTEM_TASK_STACK_SIZE,
        this,
        SYSTEM_TASK_PRIORITY,
        &system_controller_handle_
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar task system_controller");
        return ESP_ERR_NO_MEM;
    }

    result = xTaskCreate(
        power_manager_task,
        "power_manager",
        POWER_TASK_STACK_SIZE,
        this,
        POWER_TASK_PRIORITY,
        &power_manager_handle_
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar task power_manager");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Task Manager inicializado com sucesso");
    ESP_LOGI(TAG, "Tasks criadas: Sensor, Button, Display, System, Power");
    
    return ESP_OK;
}

// Métodos de envio/recebimento de dados
esp_err_t TaskManager::send_sensor_data(const SensorData& data) {
    if (xQueueSend(sensor_data_queue_, &data, pdMS_TO_TICKS(100)) == pdTRUE) {
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t TaskManager::send_display_command(const DisplayCommand& command) {
    if (xQueueSend(display_command_queue_, &command, pdMS_TO_TICKS(100)) == pdTRUE) {
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t TaskManager::send_button_event(const ButtonEvent& event) {
    if (xQueueSend(button_event_queue_, &event, pdMS_TO_TICKS(100)) == pdTRUE) {
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

bool TaskManager::receive_sensor_data(SensorData* data, TickType_t timeout) {
    return xQueueReceive(sensor_data_queue_, data, timeout) == pdTRUE;
}

bool TaskManager::receive_display_command(DisplayCommand* command, TickType_t timeout) {
    return xQueueReceive(display_command_queue_, command, timeout) == pdTRUE;
}

bool TaskManager::receive_button_event(ButtonEvent* event, TickType_t timeout) {
    return xQueueReceive(button_event_queue_, event, timeout) == pdTRUE;
}

// Implementação das tasks
void TaskManager::sensor_reader_task(void* param) {
    TaskManager* manager = static_cast<TaskManager*>(param);
    ESP_LOGI(TAG, "Sensor Reader Task iniciada");
    
    SensorData sensor_data = {};
    const TickType_t read_interval = pdMS_TO_TICKS(2000); // Ler a cada 2 segundos
    
    while (true) {
        // Aqui seria a leitura real dos sensores
        // Por enquanto, simulamos dados
        sensor_data.temperature_celsius = 25.0f + (esp_random() % 100) * 0.1f;
        sensor_data.atmospheric_pressure_hpa = 1013.0f + (esp_random() % 200) * 0.1f;
        sensor_data.tire_pressure_kpa = 250.0f + (esp_random() % 100) * 0.1f;
        sensor_data.timestamp = xTaskGetTickCount();
        
        // Enviar dados para o sistema
        manager->send_sensor_data(sensor_data);
        
        ESP_LOGD(TAG, "Sensor data: %.1fC, %.1fhPa, %.1fkPa", 
                sensor_data.temperature_celsius,
                sensor_data.atmospheric_pressure_hpa,
                sensor_data.tire_pressure_kpa);
        
        vTaskDelay(read_interval);
    }
}

void TaskManager::button_handler_task(void* param) {
    TaskManager* manager = static_cast<TaskManager*>(param);
    ESP_LOGI(TAG, "Button Handler Task iniciada");
    
    ButtonEvent button_event = {};
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t check_interval = pdMS_TO_TICKS(50); // Verificar a cada 50ms
    
    while (true) {
        // Simulação de eventos de botão para teste
        // Em uma implementação real, isso viria do driver de botões
        static uint32_t counter = 0;
        
        if (counter % 40 == 0) { // A cada ~2 segundos
            button_event.button = ButtonEvent::ButtonType::MODE;
            button_event.press_type = ButtonEvent::PressType::SHORT;
            button_event.timestamp = xTaskGetTickCount();
            
            manager->send_button_event(button_event);
            ESP_LOGD(TAG, "Button event: MODE SHORT");
        }
        
        if (counter % 100 == 0) { // A cada ~5 segundos
            button_event.button = ButtonEvent::ButtonType::UP;
            button_event.press_type = ButtonEvent::PressType::SHORT;
            button_event.timestamp = xTaskGetTickCount();
            
            manager->send_button_event(button_event);
            ESP_LOGD(TAG, "Button event: UP SHORT");
        }
        
        counter++;
        vTaskDelayUntil(&last_wake_time, check_interval);
    }
}

void TaskManager::display_manager_task(void* param) {
    TaskManager* manager = static_cast<TaskManager*>(param);
    ESP_LOGI(TAG, "Display Manager Task iniciada");
    
    DisplayCommand display_command;
    
    while (true) {
        if (manager->receive_display_command(&display_command, portMAX_DELAY)) {
            // Processar comando de display
            switch (display_command.type) {
                case DisplayCommand::CommandType::UPDATE_READINGS:
                    ESP_LOGI("DISPLAY", "Update: %.1fC, %.1fhPa, %.1fkPa",
                            display_command.sensor_data.temperature_celsius,
                            display_command.sensor_data.atmospheric_pressure_hpa,
                            display_command.sensor_data.tire_pressure_kpa);
                    break;
                    
                case DisplayCommand::CommandType::SHOW_CALIBRATION:
                    ESP_LOGI("DISPLAY", "Calibration: Offset=%.1f kPa", 
                            display_command.calibration_offset);
                    break;
                    
                case DisplayCommand::CommandType::SHOW_MENU:
                    ESP_LOGI("DISPLAY", "Menu: %s", display_command.menu_title);
                    break;
                    
                case DisplayCommand::CommandType::SHOW_ERROR:
                    ESP_LOGE("DISPLAY", "Error: %s", display_command.error_message);
                    break;
                    
                case DisplayCommand::CommandType::CLEAR_DISPLAY:
                    ESP_LOGI("DISPLAY", "Clear display");
                    break;
            }
        }
    }
}

void TaskManager::system_controller_task(void* param) {
    TaskManager* manager = static_cast<TaskManager*>(param);
    ESP_LOGI(TAG, "System Controller Task iniciada");
    
    SensorData sensor_data;
    ButtonEvent button_event;
    DisplayCommand display_command;
    
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t control_interval = pdMS_TO_TICKS(100); // Control loop a cada 100ms
    
    OperationMode current_mode = OperationMode::QUICK_READ;
    float calibration_offset = 0.0f;
    bool calibration_active = false;
    
    while (true) {
        // Processar dados de sensores
        if (manager->receive_sensor_data(&sensor_data, 0)) {
            // Atualizar display com novas leituras
            display_command.type = DisplayCommand::CommandType::UPDATE_READINGS;
            display_command.sensor_data = sensor_data;
            manager->send_display_command(display_command);
        }
        
        // Processar eventos de botão
        if (manager->receive_button_event(&button_event, 0)) {
            switch (button_event.button) {
                case ButtonEvent::ButtonType::MODE:
                    if (button_event.press_type == ButtonEvent::PressType::SHORT) {
                        // Ciclar entre modos
                        current_mode = static_cast<OperationMode>(
                            (static_cast<int>(current_mode) + 1) % 4);
                        
                        ESP_LOGI("SYSTEM", "Modo alterado para: %d", 
                                static_cast<int>(current_mode));
                    }
                    break;
                    
                case ButtonEvent::ButtonType::UP:
                    if (calibration_active) {
                        calibration_offset += 10.0f;
                        display_command.type = DisplayCommand::CommandType::SHOW_CALIBRATION;
                        display_command.calibration_offset = calibration_offset;
                        manager->send_display_command(display_command);
                    }
                    break;
                    
                case ButtonEvent::ButtonType::DOWN:
                    if (calibration_active) {
                        calibration_offset -= 10.0f;
                        display_command.type = DisplayCommand::CommandType::SHOW_CALIBRATION;
                        display_command.calibration_offset = calibration_offset;
                        manager->send_display_command(display_command);
                    }
                    break;
            }
        }
        
        vTaskDelayUntil(&last_wake_time, control_interval);
    }
}

void TaskManager::power_manager_task(void* param) {
    TaskManager* manager = static_cast<TaskManager*>(param);
    ESP_LOGI(TAG, "Power Manager Task iniciada");
    
    const TickType_t power_check_interval = pdMS_TO_TICKS(5000); // Verificar a cada 5 segundos
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (true) {
        // Monitorar consumo de energia
        // Em uma implementação real, verificaríamos:
        // - Nível da bateria
        // - Temperatura do sistema
        // - Consumo de CPU
        
        ESP_LOGD(TAG, "Power management check");
        
        // Simular condições críticas ocasionais
        static uint32_t check_count = 0;
        if (check_count % 10 == 0) { // A cada ~50 segundos
            ESP_LOGW(TAG, "Simulação: Bateria baixa (teste)");
            
            DisplayCommand display_command;
            display_command.type = DisplayCommand::CommandType::SHOW_ERROR;
            strncpy(display_command.error_message, "Bateria Baixa!", 
                    sizeof(display_command.error_message) - 1);
            manager->send_display_command(display_command);
        }
        
        check_count++;
        vTaskDelayUntil(&last_wake_time, power_check_interval);
    }
}