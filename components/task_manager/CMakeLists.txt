idf_component_register(SRCS "src/task_manager.cpp"
    INCLUDE_DIRS "include"
    REQUIRES i2c_manager bmp280_driver smp3011_driver oled_display system_controller button_driver)