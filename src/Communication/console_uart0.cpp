#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "console_dispatcher.h"


extern "C"{
#include "console_uart0.h"

#define BUF_SIZE 1024

static const char * TAG = "Console_UART0";

uint8_t data[BUF_SIZE];


void console_uart0_init(){
    uart_config_t uart_config{
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, BUF_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}


void console_uart0_read(){
    int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE - 1, pdMS_TO_TICKS(10));
    if(len  > 0){
        data[len] = '\0';
        ESP_LOGI(TAG, "Recieved %s", data);
        console_input((char *)data);
    }
}


//Extern "C" end
}


    