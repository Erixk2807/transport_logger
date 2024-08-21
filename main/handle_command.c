#include <string.h>
#include "esp_log.h"
#include "globals.h"
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

int get_padding_for_command(const char *command) {
    if (strcmp(command, "t1") == 0 || strcmp(command, "t2") == 0 || strcmp(command, "t3") == 0) {
        return 5;  // Temperature
    } else if (strcmp(command, "cpp") == 0 || strcmp(command, "ccocvp") == 0 || strcmp(command, "ccomap") == 0) {
        return 4;  // Pressure
    } else if (strcmp(command, "sqil") == 0 || strcmp(command, "sqir") == 0 || strcmp(command, "bislsr") == 0 || 
               strcmp(command, "emgl") == 0 || strcmp(command, "emgr") == 0 || strcmp(command, "bisltp") == 0 || 
               strcmp(command, "sbisl") == 0 || strcmp(command, "sbisr") == 0 || strcmp(command, "semgl") == 0) {
        return 3;  // Humidity, Sound, Light
    } else if (strcmp(command, "anesmacins") == 0 || strcmp(command, "anesmacet") == 0 || strcmp(command, "anesmac") == 0) {
        return 6;  // Vibration
    }
    return 3;  // Default padding if no match
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
    char response_str[32];
    char amended_value[6];

    // Get the appropriate padding for the command
    int padding = get_padding_for_command(command);

    // Determine which value to use based on the command
    if (strcmp(command, "t1") == 0 || strcmp(command, "cpp") == 0 || strcmp(command, "sqil") == 0 ||
        strcmp(command, "emgl") == 0 || strcmp(command, "sbisl") == 0 || strcmp(command, "anesmacins") == 0) {
        add_padding(removedValue.low, padding, amended_value);
        snprintf(response_str, sizeof(response_str), "%s", amended_value);
    } else if (strcmp(command, "t2") == 0 || strcmp(command, "ccocvp") == 0 || strcmp(command, "sqir") == 0 ||
               strcmp(command, "emgr") == 0 || strcmp(command, "sbisr") == 0 || strcmp(command, "anesmacet") == 0) {
        add_padding(removedValue.avg, padding, amended_value);
        snprintf(response_str, sizeof(response_str), "%s", amended_value);
    } else if (strcmp(command, "t3") == 0 || strcmp(command, "ccomap") == 0 || strcmp(command, "bislsr") == 0 ||
               strcmp(command, "bisltp") == 0 || strcmp(command, "semgl") == 0 || strcmp(command, "anesmac") == 0) {
        add_padding(removedValue.high, padding, amended_value);
        snprintf(response_str, sizeof(response_str), "%s", amended_value);
    }

    // Add a semicolon (;) at the start of the response if the response already has a command 
    snprintf(response + strlen(response), response_size - strlen(response), "%s%s=%s", (strlen(response) == 0) ? "" : ";", command, response_str);
}


void process_command(char *command, char *response, size_t response_size) {
    // Remove the first value and get the removed value
    if (strcmp(command, "t1") == 0 || strcmp(command, "t2") == 0 || strcmp(command, "t3") == 0) {
        Stats removedValue = remove_first_value(&temperature, &dataSize);
        insert_value_into_response(command, removedValue, response, response_size);
    } else if (strcmp(command, "cpp") == 0 || strcmp(command, "ccocvp") == 0 || strcmp(command, "ccomap") == 0) {
        Stats removedValue = remove_first_value(&pressure, &dataSize);
        insert_value_into_response(command, removedValue, response, response_size);
    } else if (strcmp(command, "sqil") == 0 || strcmp(command, "sqir") == 0 || strcmp(command, "bislsr") == 0) {
        Stats removedValue = remove_first_value(&humidity, &dataSize);
        insert_value_into_response(command, removedValue, response, response_size);
    } else if (strcmp(command, "emgl") == 0 || strcmp(command, "emgr") == 0 || strcmp(command, "bisltp") == 0) {
        Stats removedValue = remove_first_value(&sound, &dataSize);
        insert_value_into_response(command, removedValue, response, response_size);
    } else if (strcmp(command, "sbisl") == 0 || strcmp(command, "sbisr") == 0 || strcmp(command, "semgl") == 0) {
        Stats removedValue = remove_first_value(&light, &dataSize);
        insert_value_into_response(command, removedValue, response, response_size);
    } else if (strcmp(command, "anesmacins") == 0 || strcmp(command, "anesmacet") == 0 || strcmp(command, "anesmac") == 0) {
        Stats removedValue = remove_first_value(&vibration, &dataSize);
        insert_value_into_response(command, removedValue, response, response_size);
    } else {
       snprintf(response + strlen(response), response_size - strlen(response), "%s%s=%s", (strlen(response) == 0) ? "" : ";", command, UNSUPPORTED_FEATURE);
    }
}
