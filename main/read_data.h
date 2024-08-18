#ifndef READ_DATA_H
#define READ_DATA_H

#include "string.h"
#include "freertos/FreeRTOS.h"
#include "esp_spiffs.h"
#include "sys/stat.h"
#include "esp_log.h"
#include "globals.h"

// Declare the functions
void read_data_from_csv(void);

#endif