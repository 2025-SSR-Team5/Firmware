#include <stdio.h>
#include <string.h>
#include "initial_operation.hpp"
#include "esp_system.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "driver/uart.h"

#define INITIAL_DELAY 10

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

void ESP_init(I2C_t *myI2C){
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    uart_port_t uart_num = static_cast<uart_port_t>(CONFIG_ESP_CONSOLE_UART_NUM);
    ESP_ERROR_CHECK(uart_driver_install(uart_num, 256, 0, 0, NULL, 0));
    esp_vfs_dev_uart_use_driver(uart_num);
    esp_vfs_dev_uart_port_set_rx_line_endings(uart_num, ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_port_set_tx_line_endings(uart_num, ESP_LINE_ENDINGS_CRLF);

    vTaskDelay(INITIAL_DELAY);

    printf("%s", banner);

    ESP_ERROR_CHECK(myI2C->begin(GPIO_NUM_21, GPIO_NUM_22, 100000));

    myI2C->hello_ESP32();
}