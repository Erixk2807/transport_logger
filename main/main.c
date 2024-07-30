#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include <float.h>

#define ECHO_TEST_TXD (GPIO_NUM_15)
#define ECHO_TEST_RXD (GPIO_NUM_16)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM (UART_NUM_1)
#define ECHO_UART_BAUD_RATE (115200)
#define ECHO_TASK_STACK_SIZE (4096)

static const char *TAG = "DIAP SLAVE";

#define BUF_SIZE (511)
#define ARRAY_SIZE 10
#define ERROR_MESSAGE "NO DATA"
#define UNSUPPORTED_FEATURE "UNSUPPORTED FEATURE"
#define HEADER "DIAP000"

// Define a structure to hold low, high, and average values
typedef struct {
    float low;
    float avg;
    float high;
} Stats;

// Define a structure to hold the stats for temperature, pressure, and humidity
typedef struct {
    Stats *temperature;
    Stats *pressure;
    Stats *humidity;
} SensorStats;

SensorStats statsArray;

SensorStats statsArray;
int statsCount;

// Function to count the number of lines in the file
int count_lines(FILE* file) {
    int lines = 0;
    char ch;
    while(!feof(file)) {
        ch = fgetc(file);
        if(ch == '\n') {
            lines++;
        }
    }
    rewind(file); // Reset file pointer to the beginning of the file
    return lines;
}


void send_response(const char *response, int uart_num) {
    char full_response[BUF_SIZE];
    snprintf(full_response, sizeof(full_response), "%s%04X\n", response, 0xFFFF); // Placeholder CRC
    printf("RESPONSE BEFORE IT IS SENT: %s\n", full_response);
    uart_write_bytes(uart_num, full_response, strlen(full_response));
}

void handle_array_command(const char *command, const char *array_name, int *array, int *array_length, char *response, size_t response_size) {
    if (*array_length > 0) {
        snprintf(response + strlen(response), response_size - strlen(response), "%s=%d;", array_name, array[0]);
        memmove(array, array + 1, (*array_length - 1) * sizeof(int));
        (*array_length)--;
    } else {
        snprintf(response + strlen(response), response_size - strlen(response), "%s=%s;", array_name, ERROR_MESSAGE);
    }
}

void process_command(char *command, char *response, size_t response_size) {
    ESP_LOGI(TAG, "process_command %s", command);
    // if (strcmp(command, "t1") == 0) {
    //     handle_array_command(command, "t1", temperature_array, &temperature_length, response, response_size);
    // } else if (strcmp(command, "p1") == 0) {
    //     handle_array_command(command, "p1", pressure_array, &pressure_length, response, response_size);
    // } else if (strcmp(command, "anesetpercent") == 0) {
    //     handle_array_command(command, "anesetpercent", humidity_array, &humidity_length, response, response_size);
    // } else {
    //     snprintf(response + strlen(response), response_size - strlen(response), "%s;", UNSUPPORTED_FEATURE);
    // }
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


static void initi_buffer(void)
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
    statsArray.temperature = (Stats *)malloc(stats_size * sizeof(Stats));
    statsArray.pressure = (Stats *)malloc(stats_size * sizeof(Stats));
    statsArray.humidity = (Stats *)malloc(stats_size * sizeof(Stats));

    if (statsArray.temperature == NULL || statsArray.pressure == NULL || statsArray.humidity == NULL) {
        ESP_LOGI(TAG, "Error: Memory allocation failed");
        fclose(f);
        return;
    }

    char buffer[128];
    char *data;
    // Skip the header buffer
    fgets(buffer, sizeof(buffer), f);

    float temp_low = FLT_MAX, temp_high = FLT_MIN, temp_sum = 0;
    float press_low = FLT_MAX, press_high = FLT_MIN, press_sum = 0;
    float hum_low = FLT_MAX, hum_high = FLT_MIN, hum_sum = 0;
    int count = 0;

    while(fgets(buffer, sizeof(buffer), f)) {
        float temperature, pressure, humidity;
 
        data = strtok(buffer, ",");
        temperature = atof(data);

        data = strtok(NULL, ",");
        pressure = atof(data);

        data = strtok(NULL, ",");
        humidity = atof(data);

        if (temperature < temp_low) temp_low = temperature;
        if (temperature > temp_high) temp_high = temperature;
        temp_sum += temperature;

        if (pressure < press_low) press_low = pressure;
        if (pressure > press_high) press_high = pressure;
        press_sum += pressure;

        if (humidity < hum_low) hum_low = humidity;
        if (humidity > hum_high) hum_high = humidity;
        hum_sum += humidity;

        count++;
 
        // Every three data inputs, calculate stats and add to array
        if (count % 3 == 0) {
            SensorStats stats;
            statsArray.temperature[statsCount].low = temp_low;
            statsArray.temperature[statsCount].high = temp_high;
            statsArray.temperature[statsCount].avg = temp_sum / 3;
            
            statsArray.pressure[statsCount].low = press_low;
            statsArray.pressure[statsCount].high = press_high;
            statsArray.pressure[statsCount].avg = press_sum / 3;
            
            statsArray.humidity[statsCount].low = hum_low;
            statsArray.humidity[statsCount].high = hum_high;
            statsArray.humidity[statsCount].avg = hum_sum / 3;

            statsCount++;

            // Reset for next three entries
            temp_low = FLT_MAX; temp_high = FLT_MIN; temp_sum = 0;
            press_low = FLT_MAX; press_high = FLT_MIN; press_sum = 0;
            hum_low = FLT_MAX; hum_high = FLT_MIN; hum_sum = 0;
        }
    }

    fclose(f);

    esp_vfs_spiffs_unregister(conf.partition_label);
    ESP_LOGI(TAG, "SPIFFS unmounted");
}

void app_main(void) {
    initi_buffer();

    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);

    for (int i = 0; i < statsCount; i++) {
        printf("{'low': %.2f, 'avg': %.2f, 'high': %.2f}, {'low': %.2f, 'avg': %.2f, 'high': %.2f}, {'low': %.2f, 'avg': %.2f, 'high': %.2f}\n",
               statsArray.temperature[i].low, statsArray.temperature[i].avg, statsArray.temperature[i].high,
               statsArray.pressure[i].low, statsArray.pressure[i].avg, statsArray.pressure[i].high,
               statsArray.humidity[i].low, statsArray.humidity[i].avg, statsArray.humidity[i].high);
    }

    // All done, unmount partition and disable SPIFFS
    // Free allocated memory
    free(statsArray.temperature);
    free(statsArray.pressure);
    free(statsArray.humidity);
}