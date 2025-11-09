idf_component_register(SRCS "src/i2c_manager.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES i2c_manager bmp280_driver driver esp_timer)

