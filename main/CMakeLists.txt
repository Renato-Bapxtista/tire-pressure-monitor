idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES i2c_manager bmp280_driver smp3011_driver oled_display button_driver task_manager nvs_flash esp_timer)


                    






