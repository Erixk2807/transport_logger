#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_spiffs.h"
#include "sys/stat.h"
#include "esp_log.h"
#include "globals.h"

// Function to count the number of lines in the file
static int count_lines(FILE* file) {
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

static void parse_and_update_values(char* data, int *low, int *high, int *sum) {
    int value = (int)(atof(data) * 10);

    // Update stats
    if (value < *low) *low = value;
    if (value > *high) *high = value;
    *sum += value;
}


void read_data_from_csv(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    // Free previously allocated memory
    free_all_memory();

    // Reset the dataSize counter
    dataSize = 0;

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
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

    struct stat st;
    if (stat("/spiffs/data.csv", &st) != 0) {
        ESP_LOGE(TAG, "File does not exist");
        return;
    }

    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen("/spiffs/data.csv", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file");
        return;
    }

    ESP_LOGI(TAG, "Reading file");

    int line_count = count_lines(f);
    int stats_size = (line_count - 1) / 3; // -1 to skip the header line

    // Dynamically allocate memory for the stats arrays
    temperature = (Stats *)malloc(stats_size * sizeof(Stats));
    pressure = (Stats *)malloc(stats_size * sizeof(Stats));
    humidity = (Stats *)malloc(stats_size * sizeof(Stats));
    sound = (Stats *)malloc(stats_size * sizeof(Stats));
    light = (Stats *)malloc(stats_size * sizeof(Stats));
    vibration = (Stats *)malloc(stats_size * sizeof(Stats));

    if (temperature == NULL || pressure == NULL || humidity == NULL || sound == NULL || light == NULL || vibration == NULL) {
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
    int sound_low = INT_MAX, sound_high = INT_MIN, sound_sum = 0;
    int light_low = INT_MAX, light_high = INT_MIN, light_sum = 0;
    int vib_low = INT_MAX, vib_high = INT_MIN, vib_sum = 0;
    
    int count = 0;

    // Read lines from the file
    while (fgets(buffer, sizeof(buffer), f)) {
        data = strtok(buffer, ",");
        parse_and_update_values(data, &temp_low, &temp_high, &temp_sum);

        data = strtok(NULL, ",");
        parse_and_update_values(data, &press_low, &press_high, &press_sum);

        data = strtok(NULL, ",");
        parse_and_update_values(data, &hum_low, &hum_high, &hum_sum);

        data = strtok(NULL, ",");
        parse_and_update_values(data, &sound_low, &sound_high, &sound_sum);

        data = strtok(NULL, ",");
        parse_and_update_values(data, &light_low, &light_high, &light_sum);

        data = strtok(NULL, ",");
        parse_and_update_values(data, &vib_low, &vib_high, &vib_sum);

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

            sound[dataSize].low = sound_low;
            sound[dataSize].high = sound_high;
            sound[dataSize].avg = sound_sum / 3;

            light[dataSize].low = light_low;
            light[dataSize].high = light_high;
            light[dataSize].avg = light_sum / 3;

            vibration[dataSize].low = vib_low;
            vibration[dataSize].high = vib_high;
            vibration[dataSize].avg = vib_sum / 3;

            dataSize++;

            // Reset for next three entries
            temp_low = INT_MAX; temp_high = INT_MIN; temp_sum = 0;
            press_low = INT_MAX; press_high = INT_MIN; press_sum = 0;
            hum_low = INT_MAX; hum_high = INT_MIN; hum_sum = 0;
            sound_low = INT_MAX, sound_high = INT_MIN, sound_sum = 0;
            light_low = INT_MAX, light_high = INT_MIN, light_sum = 0;
            vib_low = INT_MAX, vib_high = INT_MIN, vib_sum = 0;
        }
    }

    fclose(f);

    esp_vfs_spiffs_unregister(conf.partition_label);
    ESP_LOGI(TAG, "SPIFFS unmounted");
}
