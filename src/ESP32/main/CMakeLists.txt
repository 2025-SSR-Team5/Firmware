idf_component_register(SRCS "main.cpp"
                            "initial_operation.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver console fatfs esp_timer vl53l1x-arduino)