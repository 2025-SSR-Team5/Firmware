#include <../../../../components/esp32-I2Cbus/include/I2Cbus.hpp>
#include <stdio.h>

#define STMAddr 0x32

extern "C" void app_main() {
    I2C_t& myI2C = i2c0;
    esp_err_t err;

    err = myI2C.begin(GPIO_NUM_21, GPIO_NUM_22, 100000);
    printf("begin result: 0x%x\n", err);

    while(true){
        myI2C.scanner();

        uint8_t message[10] = "Check_STM";
        err = myI2C.writeBytes(STMAddr, 0x00, sizeof(message), message);
        printf("writeBytes result: 0x%x\n", err);

        uint8_t buf[17];
        err = myI2C.readBytes(STMAddr, 0x00, sizeof(buf), buf);
        printf("readBytes result: 0x%x\n", err);

        printf("%s\n", buf);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}