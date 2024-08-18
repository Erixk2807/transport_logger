#ifndef HANDLE_COMMAND_H
#define HANDLE_COMMAND_H

#include "stdio.h"
#include "string.h"
#include "esp_log.h"
#include "globals.h"

// Declare the functions
void process_command(char *command, char *response, size_t response_size);

#endif