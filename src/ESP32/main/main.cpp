#include <../../../components/esp32-I2Cbus/include/I2Cbus.hpp>
#include <initial_operation.hpp>
#include <stdio.h>
#include <string.h>


#define STMAddr 0x32

extern "C" void app_main() {

    I2C_t& myI2C = i2c0;
    esp_err_t err;

    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_init(&myI2C);

    while(true){
        uint8_t instruction;
        scanf("%c", &instruction);
        vTaskDelay(pdMS_TO_TICKS(10));
        myI2C.hello_ESP32();
    }
}