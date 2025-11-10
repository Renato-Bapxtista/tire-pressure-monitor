#include "button_driver.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ButtonDriver";

ButtonDriver::ButtonDriver(gpio_num_t up_pin, gpio_num_t down_pin, gpio_num_t mode_pin)
    : up_pin_(up_pin), down_pin_(down_pin), mode_pin_(mode_pin),
      debounce_time_ms_(50), long_press_time_ms_(1000), very_long_press_time_ms_(3000),
      event_queue_(nullptr) {
    
    // Inicializar estados dos botões
    up_state_ = {false, false, 0, 0, false};
    down_state_ = {false, false, 0, 0, false};
    mode_state_ = {false, false, 0, 0, false};
}

ButtonDriver::~ButtonDriver() {
    if (event_queue_) {
        vQueueDelete(event_queue_);
    }
}

esp_err_t ButtonDriver::initialize() {
    ESP_LOGI(TAG, "Inicializando driver de botões");
    
    // Configurar GPIOs
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << up_pin_) | (1ULL << down_pin_) | (1ULL << mode_pin_);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // Botões conectados com pull-up interno
    
    esp_err_t result = gpio_config(&io_conf);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Falha na configuração GPIO dos botões: %s", esp_err_to_name(result));
        return result;
    }

    // Criar queue para eventos
    event_queue_ = xQueueCreate(10, sizeof(ButtonEvent));
    if (event_queue_ == nullptr) {
        ESP_LOGE(TAG, "Falha ao criar queue de eventos dos botões");
        return ESP_ERR_NO_MEM;
    }

    // Criar task para processamento dos botões
    BaseType_t task_result = xTaskCreate(
        button_task,
        "button_task",
        4096,
        this,
        2,  // Prioridade baixa
        nullptr
    );

    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar task dos botões");
        vQueueDelete(event_queue_);
        event_queue_ = nullptr;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Driver de botões inicializado com sucesso");
    return ESP_OK;
}

bool ButtonDriver::check_event(ButtonEvent* event) {
    if (event_queue_ == nullptr) {
        return false;
    }
    
    return xQueueReceive(event_queue_, event, 0) == pdTRUE;
}

void ButtonDriver::set_debounce_time(uint32_t debounce_ms) {
    debounce_time_ms_ = debounce_ms;
}

void ButtonDriver::set_long_press_time(uint32_t long_press_ms) {
    long_press_time_ms_ = long_press_ms;
}

void ButtonDriver::set_very_long_press_time(uint32_t very_long_press_ms) {
    very_long_press_time_ms_ = very_long_press_ms;
}

void ButtonDriver::button_task(void* arg) {
    ButtonDriver* driver = static_cast<ButtonDriver*>(arg);
    
    while (true) {
        driver->process_button(ButtonType::UP, driver->up_state_);
        driver->process_button(ButtonType::DOWN, driver->down_state_);
        driver->process_button(ButtonType::MODE, driver->mode_state_);
        
        vTaskDelay(pdMS_TO_TICKS(10)); // Verificar a cada 10ms
    }
}

void ButtonDriver::process_button(ButtonType button, ButtonState& state) {
    // Ler estado atual do botão
    gpio_num_t pin;
    switch (button) {
        case ButtonType::UP: pin = up_pin_; break;
        case ButtonType::DOWN: pin = down_pin_; break;
        case ButtonType::MODE: pin = mode_pin_; break;
        default: return;
    }
    
    bool current_state = gpio_get_level(pin) == 0; // Lógica invertida com pull-up
    
    // Debouncing
    if (current_state != state.last_state) {
        state.last_debounce_time = xTaskGetTickCount();
    }
    
    if ((xTaskGetTickCount() - state.last_debounce_time) > pdMS_TO_TICKS(debounce_time_ms_)) {
        if (current_state != state.current_state) {
            state.current_state = current_state;
            
            if (current_state) {
                // Botão pressionado
                state.press_start_time = xTaskGetTickCount();
                state.press_detected = true;
            } else if (state.press_detected) {
                // Botão liberado - determinar tipo de pressão
                uint32_t press_duration = xTaskGetTickCount() - state.press_start_time;
                handle_press(button, press_duration);
                state.press_detected = false;
            }
        }
    }
    
    state.last_state = current_state;
    
    // Verificar pressão longa contínua
    if (state.press_detected && state.current_state) {
        uint32_t press_duration = xTaskGetTickCount() - state.press_start_time;
        
        if (press_duration > pdMS_TO_TICKS(very_long_press_time_ms_)) {
            ButtonEvent event;
            event.button = button;
            event.press_type = PressType::VERY_LONG_PRESS;
            event.timestamp = xTaskGetTickCount();
            
            xQueueSend(event_queue_, &event, 0);
            state.press_detected = false; // Reset para não enviar múltiplos eventos
        }
    }
}

void ButtonDriver::handle_press(ButtonType button, uint32_t press_duration) {
    ButtonEvent event;
    event.button = button;
    event.timestamp = xTaskGetTickCount();
    
    if (press_duration < pdMS_TO_TICKS(long_press_time_ms_)) {
        event.press_type = PressType::SHORT_PRESS;
    } else if (press_duration < pdMS_TO_TICKS(very_long_press_time_ms_)) {
        event.press_type = PressType::LONG_PRESS;
    } else {
        // VERY_LONG_PRESS já é enviado durante a pressão contínua
        return;
    }
    
    xQueueSend(event_queue_, &event, 0);
}