#ifndef UTILS_H
#define UTILS_H

#include "stdio.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "DIAP SLAVE";

// Define a structure to hold low, high, and average values
typedef struct {
    int low;
    int avg;
    int high;
} Stats;

// Define a structure to hold the stats for temperature, pressure, and humidity
extern Stats *temperature;
extern Stats *pressure;
extern Stats *humidity;

extern int dataSize;

#define ECHO_TEST_TXD (GPIO_NUM_15)
#define ECHO_TEST_RXD (GPIO_NUM_16)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM (UART_NUM_1)
#define ECHO_UART_BAUD_RATE (115200)
#define ECHO_TASK_STACK_SIZE (4096)

#define BUF_SIZE (511)
#define ARRAY_SIZE 10
#define ERROR_MESSAGE "NO DATA"
#define UNSUPPORTED_FEATURE "UNSUPPORTED FEATURE"
#define HEADER "DIAP000"

// Declare the functions
Stats remove_first_value(Stats **dynamicArray, int *dataSize);
int count_lines(FILE* file);
void process_command(char *command, char *response, size_t response_size);

#endif // UTILS_H