#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_spiffs.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "utils.h"

Stats *temperature;
Stats *pressure;
Stats *humidity;
int dataSize;

void send_response(const char *response, int uart_num) {
    char full_response[BUF_SIZE];
    snprintf(full_response, sizeof(full_response), "%s%04X\n", response, 0xFFFF); // Placeholder CRC
    ESP_LOGI(TAG, "RESPONSE BEFORE IT IS SENT: %s\n", full_response);
    uart_write_bytes(uart_num, full_response, strlen(full_response));
}

void handle_request(char *data) {
    char response[BUF_SIZE] = HEADER "<";

    // Check header
    if (strncmp(data, HEADER, strlen(HEADER)) != 0) {
        send_response("DIAP000<ERR:BAD MSG HEADER>", ECHO_UART_PORT_NUM);
        return;
    }

    // Extract and process commands
    char *command_start = strchr(data, '<') + 1;
    char *command_end = strchr(command_start, '>');
    if (command_end == NULL) {
        send_response("DIAP000<ERR:INVALID COMMAND FORMAT>", ECHO_UART_PORT_NUM);
        return;
    }

    *command_end = '\0'; // Null-terminate the commands part
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

    strcat(response, ">");
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

static void read_data_from_csv(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Check if destination file exists before renaming
    struct stat st;
    if (stat("/spiffs/data.csv", &st) != 0) {
        ESP_LOGE(TAG, "Fail does not exists");
    }

    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen("/spiffs/data.csv", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file");
        return;
    }

    // Open renamed file for reading
    ESP_LOGI(TAG, "Reading file");

     // Count the number of lines in the file
    int line_count = count_lines(f);
    int stats_size = (line_count - 1) / 3; // -1 to skip the header line

    // Dynamically allocate memory for the stats arrays
    temperature = (Stats *)malloc(stats_size * sizeof(Stats));
    pressure = (Stats *)malloc(stats_size * sizeof(Stats));
    humidity = (Stats *)malloc(stats_size * sizeof(Stats));

    if (temperature == NULL || pressure == NULL || humidity == NULL) {
        ESP_LOGI(TAG, "Error: Memory allocation failed");
        fclose(f);
        return;
    }

    char buffer[128];
    char *data;
    // Skip the header buffer
    fgets(buffer, sizeof(buffer), f);

    // Initialize min, max, and sum variables for temperature, pressure, and humidity
    int temp_low = INT_MAX, temp_high = INT_MIN, temp_sum = 0;
    int press_low = INT_MAX, press_high = INT_MIN, press_sum = 0;
    int hum_low = INT_MAX, hum_high = INT_MIN, hum_sum = 0;
    int count = 0;

    // Read lines from the file
    while (fgets(buffer, sizeof(buffer), f)) {
        int t, p, h;

        // Parse temperature
        data = strtok(buffer, ",");
        t = atoi(data);

        // Parse pressure
        data = strtok(NULL, ",");
        p = atoi(data);

        // Parse humidity
        data = strtok(NULL, ",");
        h = atoi(data);

        // Update temperature stats
        if (t < temp_low) temp_low = t;
        if (t > temp_high) temp_high = t;
        temp_sum += t;

        // Update pressure stats
        if (p < press_low) press_low = p;
        if (p > press_high) press_high = p;
        press_sum += p;

        // Update humidity stats
        if (h < hum_low) hum_low = h;
        if (h > hum_high) hum_high = h;
        hum_sum += h;

        count++;

        // Every three data inputs, calculate stats and add to array
        if (count % 3 == 0) {
            temperature[dataSize].low = temp_low;
            temperature[dataSize].high = temp_high;
            temperature[dataSize].avg = temp_sum / 3;

            pressure[dataSize].low = press_low;
            pressure[dataSize].high = press_high;
            pressure[dataSize].avg = press_sum / 3;

            humidity[dataSize].low = hum_low;
            humidity[dataSize].high = hum_high;
            humidity[dataSize].avg = hum_sum / 3;

            dataSize++;

            // Reset for next three entries
            temp_low = INT_MAX; temp_high = INT_MIN; temp_sum = 0;
            press_low = INT_MAX; press_high = INT_MIN; press_sum = 0;
            hum_low = INT_MAX; hum_high = INT_MIN; hum_sum = 0;
        }
    }

    fclose(f);

    esp_vfs_spiffs_unregister(conf.partition_label);
    ESP_LOGI(TAG, "SPIFFS unmounted");
}

void app_main(void) {

    read_data_from_csv();

    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);

    // for (int i = 0; i < dataSize; i++) {
    //     printf("{'low': %d, 'avg': %d, 'high': %d}, {'low': %d, 'avg': %d, 'high': %d}, {'low': %d, 'avg': %d, 'high': %d}\n",
    //            temperature[i].low, temperature[i].avg, temperature[i].high,
    //            pressure[i].low, pressure[i].avg, pressure[i].high,
    //            humidity[i].low, humidity[i].avg, humidity[i].high);
    // }

    // All done, unmount partition and disable SPIFFS
    // Free allocated memory
    // free(temperature);
    // free(pressure);
    // free(humidity);
}