#include "driver/i2c_master.h"
#include <stdio.h>

#define STM32ADDR 0x32
#define STM32_CLK_SPEED 100000

#define I2C_PORT 0
#define I2C_SDA_IO 21
#define I2C_SCL_IO 22

#define INITIAL_DELAY 10

void i2c_master_bus_recover(gpio_num_t sda_pin, gpio_num_t scl_pin);
void hello_STM32(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t dev_STM32);
void ESP_init(i2c_master_bus_handle_t *bus_handle);