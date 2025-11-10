#pragma once
#include "button_driver.hpp"
#include "oled_display.hpp"
#include "bmp280_driver.hpp"
#include "smp3011_driver.hpp"

class SystemController {
public:
    enum class OperationMode {
        QUICK_READ,
        DETAILED_READ, 
        CALIBRATION,
        SETTINGS
    };

    SystemController(ButtonDriver* buttons, OLEDDisplay* display,
                    BMP280Driver* bmp280, SMP3011Driver* smp3011);
    ~SystemController();

    esp_err_t initialize();
    void process_events();
    void update_display();

private:
    ButtonDriver* buttons_;
    OLEDDisplay* display_;
    BMP280Driver* bmp280_;
    SMP3011Driver* smp3011_;

    OperationMode current_mode_;
    uint32_t last_sensor_read_;
    float current_temperature_;
    float current_atmospheric_pressure_;
    float current_tire_pressure_;

    // Estado da calibração
    bool calibration_active_;
    float calibration_offset_;

    void handle_button_event(const ButtonDriver::ButtonEvent& event);
    void change_mode(OperationMode new_mode);
    void read_sensors();
    void start_calibration();
    void stop_calibration();
    void apply_calibration(float offset);
    void show_current_mode();
};