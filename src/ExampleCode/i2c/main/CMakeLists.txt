idf_component_register(SRCS "i2c.cpp"
                            "../../../components/esp32-I2Cbus/src/I2Cbus.cpp"
                    INCLUDE_DIRS "." "../../../../components/esp32-I2Cbus/include"
                    REQUIRES driver)