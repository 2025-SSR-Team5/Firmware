idf_component_register(SRCS "main.cpp"
                            "initial_operation.cpp"
                            "bleserver.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver bt nvs_flash console fatfs ps3_controller_host esp_timer Chassis vl53l1x-arduino ESP32Servo)