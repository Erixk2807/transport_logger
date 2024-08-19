
#include <string.h>
#include "esp_log.h"
#include "handle_command.h"
#include "crc.h"
#include "read_data.h"
#include "globals.h"

void send_response(const char *response, int uart_num) {
    char full_response[BUF_SIZE];
    snprintf(full_response, sizeof(full_response), HEADER "<%s>%04X\n", response, generate_crc_value(0x0, strlen(response), response));
    ESP_LOGI(TAG, "DEVICE RESPONSE: %s\n", full_response);
    uart_write_bytes(uart_num, full_response, strlen(full_response));
}

void handle_error_response(char *response, size_t response_size, const char *error_message) {
    // Format the error message into the response buffer
    snprintf(response, response_size, "%s", error_message);
    send_response(response, ECHO_UART_PORT_NUM);
}

void handle_request(char *data) {
    char response[BUF_SIZE] = "";

    ESP_LOGI(TAG, "REQUEST RECEIVED: %s", data);

    // Check header
    if (strncmp(data, HEADER, strlen(HEADER)) != 0) {
        ESP_LOGE(TAG, "Expected HEADER: %s, but received: %s", HEADER, data);
        handle_error_response(response, sizeof(response), "BAD MSG HEADER");
        return;
    }

    // Check for line feed character
    if (strchr(data, '\n') == NULL && strchr(data, '\r') == NULL) {
        ESP_LOGE(TAG, "No line feed character found in data: %s", data);
        handle_error_response(response, sizeof(response), "NO LINE FEED");
        return;
    }

    char *command_start = strchr(data, '<');
    if (command_start == NULL) {
        ESP_LOGE(TAG, "No open delimiter '<' found in data: %s", data);
        handle_error_response(response, sizeof(response), "NO OPEN DELIMITER");
        return;
    }

    command_start++; // Move past '<'
    char *command_end = strchr(command_start, '>');
    if (command_end == NULL) {
        ESP_LOGE(TAG, "NO CLOSE DELIMITER: Expected '>', but not found in string: %s", command_start);
        handle_error_response(response, sizeof(response), "NO CLOSE DELIMITER");
        return;
    }

    // Calculate the length of the command
    size_t command_length = command_end - command_start;

    if (command_length > MAX_COMMAND_SIZE) {
        ESP_LOGE(TAG, "COMMAND TOO LARGE: Command length %zu exceeds maximum allowed size of %d", command_length, MAX_COMMAND_SIZE); 
        handle_error_response(response, sizeof(response), "COMMAND TOO LARGE");
        return;
    }

    *command_end = '\0'; // Null-terminate the commands part

    // Extract the CRC string after '>'
    char *crc_start = command_end + 1; // Point to the character after '>'

    if (!isHexadecimal(crc_start)) {
        ESP_LOGE(TAG, "INVALID CRC CHAR: Received CRC contains non-hexadecimal characters: 0x%s", crc_start);
        handle_error_response(response, sizeof(response), "INVALID CRC CHAR");
        return;
    }

    unsigned short received_crc = (unsigned short)strtol(crc_start, NULL, 16);

    unsigned short calculated_crc = generate_crc_value(0x0, strlen(command_start), command_start);

    // Check if the response is corrupt
    if (received_crc != calculated_crc) {
        ESP_LOGE(TAG, "Fail CRC check: received: %04X, calculated:%04X", received_crc, calculated_crc);
        handle_error_response(response, sizeof(response), "REQUEST CORRUPT");
        return;
    }    

    char *command = strtok(command_start, ";");
    while (command != NULL) {
        // Calculate the length of the response
        size_t response_size = strlen(response) * sizeof(char);

        // Check if the combined length exceeds the allowed buffer size
        if (response_size + LARGEST_POSSIBLE_COMMAND >= BUF_SIZE) {
            ESP_LOGE(TAG, "RESPONSE TOO LARGE: Response size %zu exceeds buffer size of %d", response_size + LARGEST_POSSIBLE_COMMAND, BUF_SIZE);
            // Send an error response and reset the buffer
            handle_error_response(response, sizeof(response), "RESPONSE TOO LARGE");
            response[0] = '\0'; // Reset the response buffer
        }

        // Process the command
        process_command(command, response, BUF_SIZE);

        // Get the next command
        command = strtok(NULL, ";");
    }

    // Clean data array
    // Remove the first value by shifting elements to the left
    if (dataSize > 0) {
        for (int i = 0; i < dataSize - 1; i++) {
            temperature[i] = temperature[i + 1];
            pressure[i] = pressure[i + 1];
            humidity[i] = humidity[i + 1];
            sound[i] = sound[i + 1];
            light[i] = light[i + 1];
            vibration[i] = vibration[i + 1];
        }

        // Resize the arrays
        temperature = (Stats*)realloc(temperature, (dataSize - 1) * sizeof(Stats));
        pressure = (Stats*)realloc(pressure, (dataSize - 1) * sizeof(Stats));
        humidity = (Stats*)realloc(humidity, (dataSize - 1) * sizeof(Stats));
        sound = (Stats*)realloc(sound, (dataSize - 1) * sizeof(Stats));
        light = (Stats*)realloc(light, (dataSize - 1) * sizeof(Stats));
        vibration = (Stats*)realloc(vibration, (dataSize - 1) * sizeof(Stats));

        dataSize--;
    }

    // Check if the message is just a newline or contains only whitespace characters
    if (response[0] == '\n' || strlen(response) == 0 || strcmp(response, "\n") == 0) {
        ESP_LOGE(TAG, "UNDEFINED ERROR: Response buffer is empty or contains only a newline character.");
        handle_error_response(response, sizeof(response), "UNDEFINED ERROR");
        return;
    }

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
    // Free allocated memory
    free(temperature);
    free(pressure);
    free(humidity);
    free(sound);
    free(light);
    free(vibration);
}

void app_main(void) {

    read_data_from_csv();

    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);

    for (int i = 0; i < 3; i++) {
        printf("{'low': %d, 'avg': %d, 'high': %d}, "
            "{'low': %d, 'avg': %d, 'high': %d}, "
            "{'low': %d, 'avg': %d, 'high': %d}, "
            "{'low': %d, 'avg': %d, 'high': %d}, "
            "{'low': %d, 'avg': %d, 'high': %d}, "
            "{'low': %d, 'avg': %d, 'high': %d}\n",
            temperature[i].low, temperature[i].avg, temperature[i].high,
            pressure[i].low, pressure[i].avg, pressure[i].high,
            humidity[i].low, humidity[i].avg, humidity[i].high,
            sound[i].low, sound[i].avg, sound[i].high,
            light[i].low, light[i].avg, light[i].high,
            vibration[i].low, vibration[i].avg, vibration[i].high);
    }

}
