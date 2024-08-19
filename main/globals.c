#include <ctype.h>
#include "globals.h"


// Define the global tag for logging
const char *TAG = "DIAP DEVICE";

Stats *temperature = NULL;
Stats *pressure = NULL;
Stats *humidity = NULL;
Stats *light = NULL;
Stats *vibration = NULL;
Stats *sound = NULL;

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