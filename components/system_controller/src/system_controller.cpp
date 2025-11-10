    #include "system_controller.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SystemController";

// Configurações do sistema
constexpr uint32_t SENSOR_READ_INTERVAL_MS = 2000;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 1000;
constexpr uint32_t BUTTON_VERY_LONG_PRESS_MS = 3000;

SystemController::SystemController(ButtonDriver* buttons, OLEDDisplay* display,
                                 BMP280Driver* bmp280, SMP3011Driver* smp3011)
    : buttons_(buttons), display_(display), bmp280_(bmp280), smp3011_(smp3011),
      current_mode_(OperationMode::QUICK_READ), last_sensor_read_(0),
      current_temperature_(0), current_atmospheric_pressure_(0), current_tire_pressure_(0),
      calibration_active_(false), calibration_offset_(0) {}

SystemController::~SystemController() {
    ESP_LOGI(TAG, "Controlador do sistema finalizado");
}

esp_err_t SystemController::initialize() {
    ESP_LOGI(TAG, "Inicializando controlador do sistema");

    // Configurar botões
    buttons_->set_debounce_time(BUTTON_DEBOUNCE_MS);
    buttons_->set_long_press_time(BUTTON_LONG_PRESS_MS);
    buttons_->set_very_long_press_time(BUTTON_VERY_LONG_PRESS_MS);

    // Mostrar modo atual
    show_current_mode();

    ESP_LOGI(TAG, "Controlador do sistema inicializado");
    return ESP_OK;
}

void SystemController::process_events() {
    ButtonDriver::ButtonEvent event;

    while (buttons_->check_event(&event)) {
        handle_button_event(event);
    }

    // Atualizar leituras dos sensores periodicamente
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (current_time - last_sensor_read_ > SENSOR_READ_INTERVAL_MS) {
        read_sensors();
        update_display();
        last_sensor_read_ = current_time;
    }
}

void SystemController::handle_button_event(const ButtonDriver::ButtonEvent& event) {
    ESP_LOGI(TAG, "Evento: Botão=%d, Tipo=%d", 
             static_cast<int>(event.button), static_cast<int>(event.press_type));

    switch (event.button) {
        case ButtonDriver::ButtonType::MODE:
            if (event.press_type == ButtonDriver::PressType::SHORT_PRESS) {
                // Ciclar entre modos
                OperationMode next_mode = static_cast<OperationMode>(
                    (static_cast<int>(current_mode_) + 1) % 4);
                change_mode(next_mode);
            } else if (event.press_type == ButtonDriver::PressType::LONG_PRESS) {
                // Ativar/desativar calibração
                if (!calibration_active_) {
                    start_calibration();
                } else {
                    stop_calibration();
                }
            }
            break;

        case ButtonDriver::ButtonType::UP:
            if (calibration_active_) {
                calibration_offset_ += 10.0f; // Aumentar offset em 10 kPa
                apply_calibration(calibration_offset_);
                ESP_LOGI(TAG, "Offset aumentado para: %.1f kPa", calibration_offset_);
            }
            break;

        case ButtonDriver::ButtonType::DOWN:
            if (calibration_active_) {
                calibration_offset_ -= 10.0f; // Diminuir offset em 10 kPa
                apply_calibration(calibration_offset_);
                ESP_LOGI(TAG, "Offset diminuido para: %.1f kPa", calibration_offset_);
            }
            break;

        default:
            break;
    }
}

void SystemController::change_mode(OperationMode new_mode) {
    current_mode_ = new_mode;
    ESP_LOGI(TAG, "Modo alterado para: %d", static_cast<int>(new_mode));
    show_current_mode();
    
    // Atualizar display imediatamente
    update_display();
}

void SystemController::read_sensors() {
    // Ler BMP280
    if (bmp280_->read_temperature_and_pressure(&current_temperature_, &current_atmospheric_pressure_) != ESP_OK) {
        ESP_LOGE(TAG, "Erro na leitura do BMP280");
        current_temperature_ = 0;
        current_atmospheric_pressure_ = 0;
    }

    // Ler SMP3011
    if (smp3011_->read_pressure(&current_tire_pressure_) != ESP_OK) {
        ESP_LOGE(TAG, "Erro na leitura do SMP3011");
        current_tire_pressure_ = 0;
    }

    ESP_LOGD(TAG, "Leituras: Temp=%.1fC, Atm=%.1fhPa, Pneu=%.1fkPa", 
             current_temperature_, current_atmospheric_pressure_, current_tire_pressure_);
}

void SystemController::update_display() {
    if (calibration_active_) {
        // Modo calibração - mostrar offset atual
        char calibration_msg[64];
        snprintf(calibration_msg, sizeof(calibration_msg), 
                "CALIBRACAO: Offset=%.1f kPa", calibration_offset_);
        display_->display_system_status(calibration_msg);
    } else {
        switch (current_mode_) {
            case OperationMode::QUICK_READ:
            case OperationMode::DETAILED_READ:
            case OperationMode::SETTINGS:
                display_->display_sensor_readings(current_temperature_, 
                                                 current_atmospheric_pressure_, 
                                                 current_tire_pressure_);
                break;
            default:
                break;
        }
    }
}

void SystemController::start_calibration() {
    calibration_active_ = true;
    calibration_offset_ = 0;
    ESP_LOGI(TAG, "Modo calibração ativado");
    
    char msg[64];
    snprintf(msg, sizeof(msg), "CALIBRACAO ATIVA");
    display_->display_system_status(msg);
}

void SystemController::stop_calibration() {
    calibration_active_ = false;
    ESP_LOGI(TAG, "Modo calibração desativado. Offset final: %.1f kPa", calibration_offset_);
    update_display();
}

void SystemController::apply_calibration(float offset) {
    smp3011_->set_pressure_offset(offset);
    update_display(); // Atualizar display para mostrar novo offset
}

void SystemController::show_current_mode() {
    const char* mode_names[] = {"LEITURA RAPIDA", "LEITURA DETALHADA", "CALIBRACAO", "CONFIGURACOES"};
    ESP_LOGI(TAG, "Modo atual: %s", mode_names[static_cast<int>(current_mode_)]);
}