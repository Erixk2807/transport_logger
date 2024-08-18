#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>

// Global tag for logging
extern const char *TAG;

// UART configuration
#define ECHO_TEST_TXD (GPIO_NUM_15)
#define ECHO_TEST_RXD (GPIO_NUM_16)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM (UART_NUM_1)
#define ECHO_UART_BAUD_RATE (115200)
#define ECHO_TASK_STACK_SIZE (4096)

// DIAP limitations
#define BUF_SIZE (511)
#define MAX_COMMAND_SIZE (255)
#define ARRAY_SIZE 10

#define UNSUPPORTED_FEATURE "UNSUPPORTED FEATURE"
#define HEADER "DIAP000"

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

#endif // GLOBALS_H