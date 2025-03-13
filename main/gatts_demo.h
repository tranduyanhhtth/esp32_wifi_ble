#ifndef __GATTS_DEMO_H
#define __GATTS_DEMO_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"

#define GATTS_TAG "GATTS_DEMO"
#define ESP_TAG "ESP_SEND"

typedef void (*ble_data_recv_handle_t)(uint8_t *data, uint16_t length);
void app_ble_send_data(uint8_t *data, uint16_t len);
void app_ble_start(void);
void app_ble_stop(void);
void app_ble_data_recv_callback(uint8_t *data, uint16_t length);
void app_ble_set_data_recv_callback(void *callback);

#endif