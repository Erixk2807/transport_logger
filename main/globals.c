#include <ctype.h>
#include "globals.h"


// Define the global tag for logging
const char *TAG = "DIAP DEVICE";

Stats *temperature = NULL;
Stats *pressure = NULL;
Stats *humidity = NULL;
Stats *sound = NULL;
Stats *light = NULL;
Stats *vibration = NULL;

int dataSize = 0;

// Util functions 
int isHexadecimal(const char *str) {
    // Check if the string is empty
    if (*str == '\0') {
        return 0;
    }
    
    // Check if each character in the string is a valid hexadecimal digit
    for (int i = 0; str[i] != '\0'; i++) {

        if (str[i] == '\n') {
            break;
        }

        if (!isxdigit((unsigned char)str[i])) {
            return 0; 
        }
    }

    return 1; 
}

// Free allocated memory
static void free_memory(void **data_memory) {
    // Check if the pointer is not NULL
    if (*data_memory != NULL) {
        free(*data_memory);   // Free the allocated memory
        *data_memory = NULL;  // Set the pointer to NULL to avoid dangling pointers
    }
}

void free_all_memory() {
    // Free previously allocated memory
    free_memory((void**)&temperature);
    free_memory((void**)&pressure);
    free_memory((void**)&humidity);
    free_memory((void**)&sound);
    free_memory((void**)&light);
    free_memory((void**)&vibration);
}