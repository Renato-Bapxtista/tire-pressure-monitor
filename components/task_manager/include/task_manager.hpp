#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"

class TaskManager {
public:
    enum class TaskID {
        SENSOR_READER,
        BUTTON_HANDLER,
        DISPLAY_MANAGER,
        SYSTEM_CONTROLLER,
        POWER_MANAGER
    };

    struct SensorData {
        float temperature_celsius;
        float atmospheric_pressure_hpa;
        float tire_pressure_kpa;
        uint32_t timestamp;
    };

    struct DisplayCommand {
        enum class CommandType {
            UPDATE_READINGS,
            SHOW_CALIBRATION,
            SHOW_MENU,
            SHOW_ERROR,
            CLEAR_DISPLAY
        };
        
        CommandType type;
        SensorData sensor_data;
        float calibration_offset;
        char error_message[64];
        char menu_title[32];
    };

    struct ButtonEvent {
        enum class ButtonType { UP, DOWN, MODE, NONE };
        enum class PressType { SHORT, LONG, VERY_LONG };
        
        ButtonType button;
        PressType press_type;
        uint32_t timestamp;
    };

    TaskManager();
    ~TaskManager();

    esp_err_t initialize();
    esp_err_t send_sensor_data(const SensorData& data);
    esp_err_t send_display_command(const DisplayCommand& command);
    esp_err_t send_button_event(const ButtonEvent& event);
    bool receive_sensor_data(SensorData* data, TickType_t timeout = 0);
    bool receive_display_command(DisplayCommand* command, TickType_t timeout = 0);
    bool receive_button_event(ButtonEvent* event, TickType_t timeout = 0);

    static void sensor_reader_task(void* param);
    static void button_handler_task(void* param);
    static void display_manager_task(void* param);
    static void system_controller_task(void* param);
    static void power_manager_task(void* param);

    // GETTERS PÚBLICOS
    TaskHandle_t get_sensor_reader_handle() const { return sensor_reader_handle_; }
    TaskHandle_t get_button_handler_handle() const { return button_handler_handle_; }
    TaskHandle_t get_display_manager_handle() const { return display_manager_handle_; }
    TaskHandle_t get_system_controller_handle() const { return system_controller_handle_; }
    TaskHandle_t get_power_manager_handle() const { return power_manager_handle_; }

private:
    QueueHandle_t sensor_data_queue_;
    QueueHandle_t display_command_queue_;
    QueueHandle_t button_event_queue_;
    
    SemaphoreHandle_t i2c_mutex_;
    SemaphoreHandle_t display_mutex_;
    
    TaskHandle_t sensor_reader_handle_;
    TaskHandle_t button_handler_handle_;
    TaskHandle_t display_manager_handle_;
    TaskHandle_t system_controller_handle_;
    TaskHandle_t power_manager_handle_;

    // Configurações das tasks
    static constexpr uint32_t SENSOR_TASK_STACK_SIZE = 4096;
    static constexpr uint32_t BUTTON_TASK_STACK_SIZE = 2048;
    static constexpr uint32_t DISPLAY_TASK_STACK_SIZE = 4096;
    static constexpr uint32_t SYSTEM_TASK_STACK_SIZE = 4096;
    static constexpr uint32_t POWER_TASK_STACK_SIZE = 2048;

    static constexpr UBaseType_t SENSOR_TASK_PRIORITY = 3;
    static constexpr UBaseType_t BUTTON_TASK_PRIORITY = 4;
    static constexpr UBaseType_t DISPLAY_TASK_PRIORITY = 2;
    static constexpr UBaseType_t SYSTEM_TASK_PRIORITY = 1;
    static constexpr UBaseType_t POWER_TASK_PRIORITY = 0;

    static constexpr uint32_t QUEUE_SIZE = 10;
};