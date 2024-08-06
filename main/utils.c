#include "utils.h"


Stats remove_first_value(Stats **dynamicArray, int *dataSize) {
    if (*dataSize == 0) {
        // Return a default value (could also return a pointer to indicate no removal)
        return (Stats){0.0, 0.0, 0.0};
    }

    // Save the first value in a variable
    Stats removedValue = (*dynamicArray)[0];

    return removedValue;
}

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


void amend_number(int number, int desired_length, char *result) {
    // Calculate the number of digits in the number
    int num_digits = snprintf(NULL, 0, "%d", number);

    // Ensure we don't have negative or zero desired length
    if (desired_length <= 0) {
        ESP_LOGE(TAG,"Invalid desired length\n");
        return;
    }

    // Calculate how many leading zeros are needed
    int num_leading_zeros = desired_length - num_digits;

    // Ensure there is no overflow in the result buffer
    if (num_leading_zeros < 0) {
        num_leading_zeros = 0;
    }

    // Format the number with leading zeros
    snprintf(result, desired_length + 1, "%0*d", desired_length, number);
}

void insert_value_into_response(const char *command, Stats removedValue, char *response, size_t response_size) {
    char value_str[32];
    char amended_value[6];
    // Determine which value to use based on the command
    if (strcmp(command, "t1") == 0 || strcmp(command, "p1") == 0) {
        amend_number(removedValue.low, 3, amended_value);
        snprintf(value_str, sizeof(value_str), "%s", amended_value);
    } else if (strcmp(command, "t2") == 0 || strcmp(command, "p2") == 0) {
        amend_number(removedValue.avg, 3, amended_value);
        snprintf(value_str, sizeof(value_str), "%s", amended_value);
    } else if (strcmp(command, "t3") == 0 || strcmp(command, "p3") == 0) {
        amend_number(removedValue.high, 3, amended_value);
        snprintf(value_str, sizeof(value_str), "%s", amended_value);
    }
    ESP_LOGI(TAG, "insert_value_into_response: %s=%s;", command, value_str);
    snprintf(response + strlen(response), response_size - strlen(response), "%s=%s;", command, value_str);
}


void process_command(char *command, char *response, size_t response_size) {
    if (strcmp(command, "t1") == 0 || strcmp(command, "t2") == 0 || strcmp(command, "t3") == 0) {
        // Remove the first value and get the removed value
        Stats removedValue = remove_first_value(&temperature, &dataSize);
        ESP_LOGI(TAG, "Remove Values:  %d, %d,  %d", removedValue.low, removedValue.avg, removedValue.high);
        insert_value_into_response(command, removedValue, response, response_size);
    } else if (strcmp(command, "p1") == 0 || strcmp(command, "p2") == 0 || strcmp(command, "p3") == 0) {
        // Remove the first value and get the removed value
        Stats removedValue = remove_first_value(&pressure, &dataSize);
            ESP_LOGI(TAG, "Remove Values:  %d, %d,  %d", removedValue.low, removedValue.avg, removedValue.high);
        insert_value_into_response(command, removedValue, response, response_size);
    } else if (strcmp(command, "anesetpercent1") == 0 || strcmp(command, "anesetpercent2") == 0 || strcmp(command, "anesetpercent3") == 0) {
        // Remove the first value and get the removed value
        Stats removedValue = remove_first_value(&humidity, &dataSize);
            ESP_LOGI(TAG, "Remove Values:  %d, %d,  %d", removedValue.low, removedValue.avg, removedValue.high);
        insert_value_into_response(command, removedValue, response, response_size);
    } else {
        snprintf(response + strlen(response), response_size - strlen(response), "%s;", UNSUPPORTED_FEATURE);
    }
}

// DO I NEED THIS? 

void handle_array_command(const char *command, const char *array_name, int *array, int *array_length, char *response, size_t response_size) {
    if (*array_length > 0) {
        snprintf(response + strlen(response), response_size - strlen(response), "%s=%d;", array_name, array[0]);
        memmove(array, array + 1, (*array_length - 1) * sizeof(int));
        (*array_length)--;
    } else {
        snprintf(response + strlen(response), response_size - strlen(response), "%s=%s;", array_name, ERROR_MESSAGE);
    }
}

