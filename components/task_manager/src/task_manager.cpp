#include "task_manager.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"

// Construtor
TaskManager::TaskManager() :
    sensor_data_queue_(nullptr),
    display_command_queue_(nullptr),
    button_event_queue_(nullptr),
    i2c_mutex_(nullptr),
    display_mutex_(nullptr),
    sensor_reader_handle_(nullptr),
    button_handler_handle_(nullptr),
    display_manager_handle_(nullptr),
    system_controller_handle_(nullptr),
    power_manager_handle_(nullptr)
{
    ESP_LOGI("TASK_MANAGER", "TaskManager criado");
}

// Destrutor
TaskManager::~TaskManager()
{
    ESP_LOGI("TASK_MANAGER", "TaskManager destruído");
    
    // Deletar tasks se existirem
    if (sensor_reader_handle_) vTaskDelete(sensor_reader_handle_);
    if (button_handler_handle_) vTaskDelete(button_handler_handle_);
    if (display_manager_handle_) vTaskDelete(display_manager_handle_);
    if (system_controller_handle_) vTaskDelete(system_controller_handle_);
    if (power_manager_handle_) vTaskDelete(power_manager_handle_);
    
    // Deletar queues
    if (sensor_data_queue_) vQueueDelete(sensor_data_queue_);
    if (display_command_queue_) vQueueDelete(display_command_queue_);
    if (button_event_queue_) vQueueDelete(button_event_queue_);
    
    // Deletar mutexes
    if (i2c_mutex_) vSemaphoreDelete(i2c_mutex_);
    if (display_mutex_) vSemaphoreDelete(display_mutex_);
}

// Inicialização
esp_err_t TaskManager::initialize()
{
    ESP_LOGI("TASK_MANAGER", "Inicializando TaskManager...");
    
    // Criar queues
    sensor_data_queue_ = xQueueCreate(QUEUE_SIZE, sizeof(SensorData));
    if (!sensor_data_queue_) {
        ESP_LOGE("TASK_MANAGER", "Falha ao criar sensor_data_queue");
        return ESP_ERR_NO_MEM;
    }
    
    display_command_queue_ = xQueueCreate(QUEUE_SIZE, sizeof(DisplayCommand));
    if (!display_command_queue_) {
        ESP_LOGE("TASK_MANAGER", "Falha ao criar display_command_queue");
        vQueueDelete(sensor_data_queue_);
        return ESP_ERR_NO_MEM;
    }
    
    button_event_queue_ = xQueueCreate(QUEUE_SIZE, sizeof(ButtonEvent));
    if (!button_event_queue_) {
        ESP_LOGE("TASK_MANAGER", "Falha ao criar button_event_queue");
        vQueueDelete(sensor_data_queue_);
        vQueueDelete(display_command_queue_);
        return ESP_ERR_NO_MEM;
    }
    
    // Criar mutexes
    i2c_mutex_ = xSemaphoreCreateMutex();
    if (!i2c_mutex_) {
        ESP_LOGE("TASK_MANAGER", "Falha ao criar i2c_mutex");
        vQueueDelete(sensor_data_queue_);
        vQueueDelete(display_command_queue_);
        vQueueDelete(button_event_queue_);
        return ESP_ERR_NO_MEM;
    }
    
    display_mutex_ = xSemaphoreCreateMutex();
    if (!display_mutex_) {
        ESP_LOGE("TASK_MANAGER", "Falha ao criar display_mutex");
        vQueueDelete(sensor_data_queue_);
        vQueueDelete(display_command_queue_);
        vQueueDelete(button_event_queue_);
        vSemaphoreDelete(i2c_mutex_);
        return ESP_ERR_NO_MEM;
    }
    
    // Criar tasks
    BaseType_t result;
    
    result = xTaskCreate(sensor_reader_task, "sensor_reader", 
                        SENSOR_TASK_STACK_SIZE, this, 
                        SENSOR_TASK_PRIORITY, &sensor_reader_handle_);
    if (result != pdPASS) {
        ESP_LOGE("TASK_MANAGER", "Falha ao criar sensor_reader_task");
        goto cleanup;
    }
    
    result = xTaskCreate(button_handler_task, "button_handler", 
                        BUTTON_TASK_STACK_SIZE, this, 
                        BUTTON_TASK_PRIORITY, &button_handler_handle_);
    if (result != pdPASS) {
        ESP_LOGE("TASK_MANAGER", "Falha ao criar button_handler_task");
        goto cleanup;
    }
    
    result = xTaskCreate(display_manager_task, "display_manager", 
                        DISPLAY_TASK_STACK_SIZE, this, 
                        DISPLAY_TASK_PRIORITY, &display_manager_handle_);
    if (result != pdPASS) {
        ESP_LOGE("TASK_MANAGER", "Falha ao criar display_manager_task");
        goto cleanup;
    }
    
    result = xTaskCreate(system_controller_task, "system_controller", 
                        SYSTEM_TASK_STACK_SIZE, this, 
                        SYSTEM_TASK_PRIORITY, &system_controller_handle_);
    if (result != pdPASS) {
        ESP_LOGE("TASK_MANAGER", "Falha ao criar system_controller_task");
        goto cleanup;
    }
    
    result = xTaskCreate(power_manager_task, "power_manager", 
                        POWER_TASK_STACK_SIZE, this, 
                        POWER_TASK_PRIORITY, &power_manager_handle_);
    if (result != pdPASS) {
        ESP_LOGE("TASK_MANAGER", "Falha ao criar power_manager_task");
        goto cleanup;
    }
    
    ESP_LOGI("TASK_MANAGER", "TaskManager inicializado com sucesso");
    return ESP_OK;

cleanup:
    // Cleanup em caso de erro
    if (sensor_reader_handle_) vTaskDelete(sensor_reader_handle_);
    if (button_handler_handle_) vTaskDelete(button_handler_handle_);
    if (display_manager_handle_) vTaskDelete(display_manager_handle_);
    if (system_controller_handle_) vTaskDelete(system_controller_handle_);
    if (power_manager_handle_) vTaskDelete(power_manager_handle_);
    
    vQueueDelete(sensor_data_queue_);
    vQueueDelete(display_command_queue_);
    vQueueDelete(button_event_queue_);
    if (i2c_mutex_) vSemaphoreDelete(i2c_mutex_);
    if (display_mutex_) vSemaphoreDelete(display_mutex_);
    
    return ESP_ERR_NO_MEM;
}

// Implementações das tasks (placeholder por enquanto)
void TaskManager::sensor_reader_task(void* param)
{
    TaskManager* manager = static_cast<TaskManager*>(param);
    ESP_LOGI("SENSOR_READER", "Task iniciada");
    
    while (true) {
        // Simular leitura de sensores
        SensorData data = {
            .temperature_celsius = 25.0f,
            .atmospheric_pressure_hpa = 1013.25f,
            .tire_pressure_kpa = 220.0f,
            .timestamp = xTaskGetTickCount()
        };
        
        manager->send_sensor_data(data);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskManager::button_handler_task(void* param)
{
    ESP_LOGI("BUTTON_HANDLER", "Task iniciada");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void TaskManager::display_manager_task(void* param)
{
    ESP_LOGI("DISPLAY_MANAGER", "Task iniciada");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void TaskManager::system_controller_task(void* param)
{
    ESP_LOGI("SYSTEM_CONTROLLER", "Task iniciada");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void TaskManager::power_manager_task(void* param)
{
    ESP_LOGI("POWER_MANAGER", "Task iniciada");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Métodos de envio
esp_err_t TaskManager::send_sensor_data(const SensorData& data)
{
    if (xQueueSend(sensor_data_queue_, &data, 0) != pdPASS) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t TaskManager::send_display_command(const DisplayCommand& command)
{
    if (xQueueSend(display_command_queue_, &command, 0) != pdPASS) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t TaskManager::send_button_event(const ButtonEvent& event)
{
    if (xQueueSend(button_event_queue_, &event, 0) != pdPASS) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

// Métodos de recebimento
bool TaskManager::receive_sensor_data(SensorData* data, TickType_t timeout)
{
    return xQueueReceive(sensor_data_queue_, data, timeout) == pdTRUE;
}

bool TaskManager::receive_display_command(DisplayCommand* command, TickType_t timeout)
{
    return xQueueReceive(display_command_queue_, command, timeout) == pdTRUE;
}

bool TaskManager::receive_button_event(ButtonEvent* event, TickType_t timeout)
{
    return xQueueReceive(button_event_queue_, event, timeout) == pdTRUE;
}