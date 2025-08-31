idf_component_register(SRCS "ble.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver bt nvs_flash)