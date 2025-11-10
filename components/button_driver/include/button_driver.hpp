#pragma once
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

class ButtonDriver {
public:
    enum class ButtonType {
        UP,
        DOWN, 
        MODE,
        NONE
    };

    enum class PressType {
        SHORT_PRESS,
        LONG_PRESS,
        VERY_LONG_PRESS
    };

    struct ButtonEvent {
        ButtonType button;
        PressType press_type;
        uint32_t timestamp;
    };

    ButtonDriver(gpio_num_t up_pin, gpio_num_t down_pin, gpio_num_t mode_pin);
    ~ButtonDriver();

    esp_err_t initialize();
    bool check_event(ButtonEvent* event);
    void set_debounce_time(uint32_t debounce_ms);
    void set_long_press_time(uint32_t long_press_ms);
    void set_very_long_press_time(uint32_t very_long_press_ms);

private:
    gpio_num_t up_pin_;
    gpio_num_t down_pin_;
    gpio_num_t mode_pin_;
    
    uint32_t debounce_time_ms_;
    uint32_t long_press_time_ms_;
    uint32_t very_long_press_time_ms_;
    
    QueueHandle_t event_queue_;
    
    struct ButtonState {
        bool current_state;
        bool last_state;
        uint32_t last_debounce_time;
        uint32_t press_start_time;
        bool press_detected;
    } up_state_, down_state_, mode_state_;

    static void button_task(void* arg);
    void process_button(ButtonType button, ButtonState& state);
    void handle_press(ButtonType button, uint32_t press_duration);
};