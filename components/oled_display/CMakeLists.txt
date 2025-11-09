idf_component_register(SRCS "src/oled_display.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES i2c_manager u8g2)