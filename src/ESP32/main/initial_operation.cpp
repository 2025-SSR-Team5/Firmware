#include <stdio.h>
#include <string.h>
#include "initial_operation.hpp"
#include "esp_system.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "driver/uart.h"

i2c_master_dev_handle_t dev_STM32;

const char *banner = R"(
   /$$$$$$   /$$$$$$   /$$$$$$   /$$$$$$  /$$$$$$$  /$$$$$$        /$$$$$$   /$$$$$$ 
  /$$__  $$ /$$__  $$ /$$__  $$ /$$__  $$| $$__  $$|_  $$_/       /$$__  $$ /$$__  $$ 
 | $$  \__/| $$  \ $$| $$  \__/| $$  \ $$| $$  \ $$  | $$        | $$  \ $$| $$  \__/ 
 |  $$$$$$ | $$$$$$$$|  $$$$$$ | $$  | $$| $$$$$$$/  | $$        | $$  | $$|  $$$$$$  
  \____  $$| $$__  $$ \____  $$| $$  | $$| $$__  $$  | $$        | $$  | $$ \____  $$ 
  /$$  \ $$| $$  | $$ /$$  \ $$| $$  | $$| $$  \ $$  | $$        | $$  | $$ /$$  \ $$ 
 |  $$$$$$/| $$  | $$|  $$$$$$/|  $$$$$$/| $$  | $$ /$$$$$$      |  $$$$$$/|  $$$$$$/ 
  \______/ |__/  |__/ \______/  \______/ |__/  |__/|______/       \______/  \______/                                                                                    
)";

void hello_STM32(i2c_master_dev_handle_t dev_STM32){
    constexpr int32_t Timeout = -1;
    constexpr uint8_t instruction = 0x00;

    printf(LOG_COLOR_D "\n>> STM32 scanning ..." LOG_RESET_COLOR "\n");

    esp_err_t status;
    if((status = i2c_master_transmit(dev_STM32, &instruction, 1, Timeout)) != ESP_OK){
        printf(LOG_COLOR_E "- Cannot write to STM32 with error code %X!%s", status, LOG_RESET_COLOR "\n");
        return;
    }

    uint8_t buf[17];
    uint32_t id_data[4]; 

    vTaskDelay(pdMS_TO_TICKS(100));
    if((status = i2c_master_receive(dev_STM32, buf, sizeof(buf), Timeout)) == ESP_OK){
        id_data[0] = buf[1] | buf[2] << 8 | buf[3] << 16 | buf[4] << 24;
        id_data[1] = buf[5] | buf[6] << 8 | buf[7] << 16 | buf[8] << 24;
        id_data[2] = buf[9] | buf[10] << 8 | buf[11] << 16 | buf[12] << 24;
        id_data[3] = buf[13] | buf[14] << 8 | buf[15] << 16 | buf[16] << 24;

        printf(LOG_COLOR_I "- STM32 found (status:%d)%s", buf[0], LOG_RESET_COLOR "\n");
        printf(LOG_COLOR_I "- device_id:0x%lX%s", id_data[0], LOG_RESET_COLOR "\n");
        printf(LOG_COLOR_I "- Unique_id:0x%lX 0x%lX 0x%lX%s", id_data[1],id_data[2],id_data[3], LOG_RESET_COLOR "\n");
    }else{
        printf(LOG_COLOR_E "- Cannot read a data from STM32 with error code %X!%s", status, LOG_RESET_COLOR "\n");
    }
}

void ESP_init(i2c_master_bus_handle_t *bus_handle){
    //stdを使うためのコード
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    uart_port_t uart_num = static_cast<uart_port_t>(CONFIG_ESP_CONSOLE_UART_NUM);
    ESP_ERROR_CHECK(uart_driver_install(uart_num, 256, 0, 0, NULL, 0));
    esp_vfs_dev_uart_use_driver(uart_num);
    esp_vfs_dev_uart_port_set_rx_line_endings(uart_num, ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_port_set_tx_line_endings(uart_num, ESP_LINE_ENDINGS_CRLF);

    vTaskDelay(INITIAL_DELAY);

    printf("%s", banner);

    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = I2C_PORT;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.sda_io_num = static_cast<gpio_num_t>(I2C_SDA_IO);
    bus_config.scl_io_num = static_cast<gpio_num_t>(I2C_SCL_IO);
    bus_config.glitch_ignore_cnt = 7;

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    //i2c_master_dev_handle_t dev_STM32;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = STM32ADDR,
        .scl_speed_hz = STM32_CLK_SPEED,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_cfg, &dev_STM32));

    //hello_STM32(dev_STM32);
}