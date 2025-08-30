#include <initial_operation.hpp>
#include <../../../components/vl53l1x-arduino/src/include/VL53L1X.hpp>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TOF_A_ADDR 0x30

extern i2c_master_dev_handle_t dev_STM32;

extern "C" void app_main() {
    i2c_master_bus_handle_t bus_handle;
    VL53L1X tof_sensor_A;

    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_init(&bus_handle);

    tof_sensor_A.setBus(&bus_handle);
    tof_sensor_A.setTimeout(500);
init:
    if(!tof_sensor_A.init()){
        printf("Failed to detect and initialize sensor!\n");
        goto init;
    }

    tof_sensor_A.setDistanceMode(VL53L1X::Long);
    tof_sensor_A.setMeasurementTimingBudget(50000);
    tof_sensor_A.setAddress(TOF_A_ADDR, &bus_handle);
    tof_sensor_A.startContinuous(50);

    while(true){
        /*
        uint8_t instruction;
        scanf("%c", &instruction);
        vTaskDelay(pdMS_TO_TICKS(10));
        myI2C.hello_ESP32();
        */

        printf("distance:%d\n",tof_sensor_A.read());
        if (tof_sensor_A.timeoutOccurred()){
            printf("Timeout!\n");
        }
        hello_STM32(dev_STM32);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}