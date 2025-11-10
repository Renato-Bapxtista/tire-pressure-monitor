idf_component_register(SRCS "src/system_controller.cpp"
    INCLUDE_DIRS "include"
    REQUIRES button_driver oled_display bmp280_driver smp3011_driver)