idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES i2c_manager nvs_flash esp_timer)