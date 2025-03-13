#ifndef __ESP32_WIFI_CONTROL_MAIN_H
#define __ESP32_WIFI_CONTROL_MAIN_H

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs.h"
#include "esp_http_server.h"
#include <ctype.h>

#include "gatts_demo.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* STA Configuration */
#define ESP_MAXIMUM_RETRY 5
#define MAX_WIFI_SSID_LEN 32
#define MAX_WIFI_PASS_LEN 64
#define DEFAULT_SCAN_LIST_SIZE 10

/* AP Configuration */
#define ESP_WIFI_AP_SSID "ESP32_Wifi"
#define ESP_WIFI_AP_PASS "123456789"
#define ESP_WIFI_CHANNEL 1
#define MAX_STA_CONN 5

#define STORAGE_NAMESPACE "storage"
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER ""
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

void wifi_init_softap(void);
void wifi_init_station(char *SSID, char *PASSWORD);

#endif