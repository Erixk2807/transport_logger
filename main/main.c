#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_spiffs.h"
#include "sys/unistd.h"
#include "sys/stat.h"
#include "esp_err.h"
#include "handle_command.h"
#include "crc.h"
#include "read_data.h"
#include "globals.h"

void send_response(const char *response, int uart_num) {
    char full_response[BUF_SIZE];
    snprintf(full_response, sizeof(full_response), HEADER "<%s>%04X\n", response, generate_crc_value(0x0, strlen(response), response));
    ESP_LOGI(TAG, "RESPONSE BEFORE IT IS SENT: %s\n", full_response);
    uart_write_bytes(uart_num, full_response, strlen(full_response));
}

void handle_request(char *data) {
    char response[BUF_SIZE] = "";

    // Check header
    if (strncmp(data, HEADER, strlen(HEADER)) != 0) {
        snprintf(response, sizeof(response), "BAD MSG HEADER");
        send_response(response, ECHO_UART_PORT_NUM);
        return;
    }

    // Extract and process commands
    char *command_start = strchr(data, '<') + 1;
    if (command_start == NULL) {
        snprintf(response, sizeof(response), "NO OPEN DELIMITER");
        send_response(response, ECHO_UART_PORT_NUM);
        return;
    }

    char *command_end = strchr(command_start, '>');
    if (command_end == NULL) {
        snprintf(response, sizeof(response), "NO CLOSE DELIMITER");
        send_response(response, ECHO_UART_PORT_NUM);
        return;
    }


    // Calculate the length of the command
    size_t command_length = command_end - command_start;

    if (command_length > MAX_COMMAND_SIZE) {
        snprintf(response, sizeof(response), "COMMAND TOO LARGE");
        send_response(response, ECHO_UART_PORT_NUM); 
        return;
    }


    *command_end = '\0'; // Null-terminate the commands part

    // Extract the CRC string after '>'
    char *crc_start = command_end + 1; // Point to the character after '>'
    // Convert CRC string to an integer using strtol with base 16
    unsigned short received_crc = (unsigned short)strtol(crc_start, NULL, 16);
    unsigned short calculated_crc = generate_crc_value(0x0, strlen(command_start), command_start);

    // Check if the response is corrupt
    if (received_crc != calculated_crc) {
        ESP_LOGE(TAG, "Fail CRC check: received: %04X, calculated:%04X", received_crc, calculated_crc);
        snprintf(response, sizeof(response), "REQUEST CORRUPT");
        send_response(response, ECHO_UART_PORT_NUM);
        return;
    }    

    char *command = strtok(command_start, ";");
    while (command != NULL) {
        process_command(command, response, BUF_SIZE);
        command = strtok(NULL, ";");
    }

    // Clean data array
    // Remove the first value by shifting elements to the left
    for (int i = 0; i < dataSize - 1; i++) {
        (temperature)[i] = (temperature)[i + 1];
        (pressure)[i] = (pressure)[i + 1];
        (humidity)[i] = (humidity)[i + 1];
    }

    // Resize the arrays
    temperature = (Stats*)realloc(temperature, (dataSize - 1) * sizeof(Stats));
    pressure = (Stats*)realloc(pressure, (dataSize - 1) * sizeof(Stats));
    humidity = (Stats*)realloc(humidity, (dataSize - 1) * sizeof(Stats));

    dataSize--;

    send_response(response, ECHO_UART_PORT_NUM);
}

static void echo_task(void *arg) {
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0'; // Null-terminate the string
            handle_request((char *)data);
        }
    }
}

void app_main(void) {

    read_data_from_csv();

    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);

    for (int i = 0; i < dataSize; i++) {
        printf("{'low': %d, 'avg': %d, 'high': %d}, {'low': %d, 'avg': %d, 'high': %d}, {'low': %d, 'avg': %d, 'high': %d}\n",
               temperature[i].low, temperature[i].avg, temperature[i].high,
               pressure[i].low, pressure[i].avg, pressure[i].high,
               humidity[i].low, humidity[i].avg, humidity[i].high);
    }

    // All done, unmount partition and disable SPIFFS
    // Free allocated memory
    // free(temperature);
    // free(pressure);
    // free(humidity);
}