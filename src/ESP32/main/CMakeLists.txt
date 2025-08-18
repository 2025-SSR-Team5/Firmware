idf_component_register(SRCS "main.cpp"
                            "initial_operation.cpp"
                            "../../../components/esp32-I2Cbus/src/I2Cbus.cpp"
                    INCLUDE_DIRS "." "../../../components/esp32-I2Cbus/include"
                    REQUIRES driver console fatfs)