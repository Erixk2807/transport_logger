#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include "driver/uart.h"
#include "driver/gpio.h"

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

// 5 minutes in milliseconds to update data
#define CHECK_INTERVAL_MS (1 * 60 * 1000)  // 5 minutes in milliseconds

// DIAP limitations
#define BUF_SIZE (511)
#define LARGEST_POSSIBLE_COMMAND (22)
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
extern Stats *sound;
extern Stats *light;
extern Stats *vibration;

extern int dataSize;

int isHexadecimal(const char *str);
void free_all_memory();

#endif