#include "handle_command.h"


static Stats remove_first_value(Stats **dynamicArray, int *dataSize) {
    if (*dataSize == 0) {
        // Return a default value (could also return a pointer to indicate no removal)
        return (Stats){0.0, 0.0, 0.0};
    }

    // Save the first value in a variable
    Stats removedValue = (*dynamicArray)[0];

    return removedValue;
}

static void add_padding(int number, int desired_length, char *result) {
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

static void insert_value_into_response(const char *command, Stats removedValue, char *response, size_t response_size) {
    char value_str[32];
    char amended_value[6];
    // Determine which value to use based on the command
    if (strcmp(command, "t1") == 0 || strcmp(command, "p1") == 0) {
        add_padding(removedValue.low, 3, amended_value);
        snprintf(value_str, sizeof(value_str), "%s", amended_value);
    } else if (strcmp(command, "t2") == 0 || strcmp(command, "p2") == 0) {
        add_padding(removedValue.avg, 3, amended_value);
        snprintf(value_str, sizeof(value_str), "%s", amended_value);
    } else if (strcmp(command, "t3") == 0 || strcmp(command, "p3") == 0) {
        add_padding(removedValue.high, 3, amended_value);
        snprintf(value_str, sizeof(value_str), "%s", amended_value);
    }
    // Add a semicolon (;) at the start of the response if the response is empty 
    snprintf(response + strlen(response), response_size - strlen(response), "%s%s=%s", (strlen(response) == 0) ? "" : ";", command, value_str);
}


void process_command(char *command, char *response, size_t response_size) {
    // Remove the first value and get the removed value
    if (strcmp(command, "t1") == 0 || strcmp(command, "t2") == 0 || strcmp(command, "t3") == 0) {
        Stats removedValue = remove_first_value(&temperature, &dataSize);
        insert_value_into_response(command, removedValue, response, response_size);
    } else if (strcmp(command, "p1") == 0 || strcmp(command, "p2") == 0 || strcmp(command, "p3") == 0) {
        Stats removedValue = remove_first_value(&pressure, &dataSize);
        insert_value_into_response(command, removedValue, response, response_size);
    } else if (strcmp(command, "anesetpercent1") == 0 || strcmp(command, "anesetpercent2") == 0 || strcmp(command, "anesetpercent3") == 0) {
        Stats removedValue = remove_first_value(&humidity, &dataSize);
        insert_value_into_response(command, removedValue, response, response_size);
    } else {
        snprintf(response + strlen(response), response_size - strlen(response), "%s;", UNSUPPORTED_FEATURE);
    }
}
