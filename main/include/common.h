#ifndef COMMON_H
#define COMMON_H

// STD APIs
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// ESP APIs
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

// FREERTOS APIs
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// NimBLE Stack APIs
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

// Defines
#define TAG "BLE_PROJECT"
#define DEVICE_NAME "TEMP_MON"

#define BYTE_0(X) ((uint8_t) ((X>>0) & (0xFF)) )
#define BYTE_1(X) ((uint8_t) ((X>>8) & (0xFF)) )  
#define BYTE_2(X) ((uint8_t) ((X>>16) & (0xFF)) )
#define BYTE_3(X) ((uint8_t) ((X>>24) & (0xFF)) )   

#endif