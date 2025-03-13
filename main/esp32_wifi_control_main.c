#include "esp32_wifi_control_main.h"

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG_AP = "WiFi SoftAP";
static const char *TAG_STA = "WiFi Station";
static const char *TAG_HTTP = "HTTP Server";
static const char *TAG_NVS = "NVS";
int s_retry_num = 0;

esp_netif_t *esp_netif_sta;
esp_netif_t *esp_netif_ap;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG_AP, "Station " MACSTR " joined, AID = %d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG_AP, "Station " MACSTR " left, AID = %d, reason: %d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG_STA, "Station started");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_STA, "Retry to connect to the AP");
        }
        else
        {
            ESP_LOGI(TAG_STA, "Connect to the AP fail");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            wifi_init_softap();

            /************** Register the GATT callback **************/
            ESP_LOGI(GATTS_TAG, "ESP_BLUETOOTH_MODE");
            app_ble_set_data_recv_callback(app_ble_data_recv_callback);
            app_ble_start();
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_STA, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        s_retry_num = 0;
    }
}

// Hàm ghi ID và mật khẩu Wi-Fi vào NVS
static esp_err_t save_wifi_credentials(const char *ssid, const char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    // Mở NVS với namespace "storage"
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error opening NVS handle!");
        return err;
    }
    // Ghi SSID
    err = nvs_set_str(nvs_handle, "wifi_ssid", ssid);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Failed to write SSID to NVS!");
        nvs_close(nvs_handle);
        return err;
    }
    // Ghi mật khẩu
    err = nvs_set_str(nvs_handle, "wifi_pass", password);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Failed to write password to NVS!");
        nvs_close(nvs_handle);
        return err;
    }
    // Commit dữ liệu vào NVS
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Failed to commit wifi credentials to NVS!");
    }
    // Đóng NVS
    nvs_close(nvs_handle);
    return err;
}

// Hàm đọc ID và mật khẩu Wi-Fi từ NVS
static esp_err_t load_wifi_credentials(char *ssid, size_t ssid_len,
                                       char *password, size_t password_len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    // Mở NVS với namespace "storage"
    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error opening NVS handle!");
        return err;
    }
    // Đọc SSID
    err = nvs_get_str(nvs_handle, "wifi_ssid", ssid, &ssid_len);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Failed to read SSID from NVS!");
        nvs_close(nvs_handle);
        return err;
    }
    // Đọc mật khẩu
    err = nvs_get_str(nvs_handle, "wifi_pass", password, &password_len);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Failed to read password from NVS!");
    }
    // Đóng NVS
    nvs_close(nvs_handle);
    return err;
}

void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    esp_netif_set_default_netif(esp_netif_ap);

    // Wi-Fi configuration
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_AP_SSID,
            .ssid_len = strlen(ESP_WIFI_AP_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_AP_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    if (strlen(ESP_WIFI_AP_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_AP, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ESP_WIFI_AP_SSID, ESP_WIFI_AP_PASS, ESP_WIFI_CHANNEL);
}

void wifi_init_station(char *SSID, char *PASSWORD)
{
    s_wifi_event_group = xEventGroupCreate();
    esp_netif_set_default_netif(esp_netif_sta);

    // Wi-Fi configuration
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = ESP_MAXIMUM_RETRY,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    strncpy((char *)wifi_config.sta.ssid, SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, PASSWORD, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_STA, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT)
     * or connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
     * The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence
     * we can test which event actually happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG_STA, "Connected to ap SSID:%s password:%s", SSID, PASSWORD);
        app_ble_stop();
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG_STA, "Failed to connect to SSID:%s, password:%s", SSID, PASSWORD);
        ESP_LOGI(TAG_STA, "Please try again!");
        vEventGroupDelete(s_wifi_event_group);
    }
    else
    {
        ESP_LOGE(TAG_STA, "UNEXPECTED EVENT");
    }
}

void url_decode(char *dst, const char *src, size_t dst_len)
{
    char a, b;
    size_t i, j;
    for (i = 0, j = 0; src[i] != '\0' && j < dst_len - 1; i++, j++)
    {
        if ((src[i] == '%') && ((a = src[i + 1]) && (b = src[i + 2])) && (isxdigit(a) && isxdigit(b)))
        {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            dst[j] = 16 * a + b;
            i += 2;
        }
        else if (src[i] == '+')
        {
            dst[j] = ' ';
        }
        else
        {
            dst[j] = src[i];
        }
    }
    dst[j] = '\0';
}

// Xử lý HTTP Server cho URI '/set_wifi'
esp_err_t set_wifi_handler(httpd_req_t *req)
{
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    if (httpd_req_recv(req, content, recv_size) <= 0)
    {
        ESP_LOGE(TAG_HTTP, "Failed to receive request content");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Failed to receive request content");
        return ESP_FAIL;
    }
    content[recv_size] = '\0';

    char ssid[32] = {0}, password[64] = {0};
    char *token = strtok(content, "&");
    while (token != NULL)
    {
        if (strncmp(token, "ssid=", 5) == 0)
        {
            url_decode(ssid, token + 5, sizeof(ssid));
        }
        else if (strncmp(token, "password=", 9) == 0)
        {
            url_decode(password, token + 9, sizeof(password));
        }
        token = strtok(NULL, "&");
    }

    ESP_LOGI(TAG_HTTP, "SSID: %s", ssid);
    ESP_LOGI(TAG_HTTP, "Password: %s", password);

    wifi_init_station(ssid, password);

    esp_err_t wifi_err = esp_wifi_connect();
    if (wifi_err == ESP_OK)
    {
        save_wifi_credentials(ssid, password);
    }
    return wifi_err;
}

// Xử lý HTTP Server cho URI '/'
esp_err_t index_get_handler(httpd_req_t *req)
{
    extern const uint8_t index_html_start[] asm("_binary_index_html_start");
    extern const uint8_t index_html_end[] asm("_binary_index_html_end");
    size_t buffer_len = (size_t)(index_html_end - index_html_start);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, buffer_len);
    return ESP_OK;
}

// Hàm khởi tạo webserver
httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t uri_send = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_get_handler,
        .user_ctx = NULL,
    };

    httpd_uri_t uri_receive = {
        .uri = "/set_wifi",
        .method = HTTP_POST,
        .handler = set_wifi_handler,
        .user_ctx = NULL,
    };

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_send);
        httpd_register_uri_handler(server, &uri_receive);
    }
    return server;
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    char ssid[MAX_WIFI_SSID_LEN] = "";
    char password[MAX_WIFI_PASS_LEN] = "";
    err = load_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password));
    wifi_init();
    if (err == ESP_OK)
    {
        /* Initialize STA */
        ESP_LOGI(TAG_STA, "ESP_WIFI_MODE_STA");
        wifi_init_station(ssid, password);
    }
    else
    {
        printf("Failed to load Wi-Fi credentials: %s\n", esp_err_to_name(err));
        /********** Initialize AP **********/
        ESP_LOGI(TAG_AP, "ESP_WIFI_MODE_AP");
        wifi_init_softap();

        /************** Register the GATT callback **************/
        ESP_LOGI(GATTS_TAG, "ESP_BLUETOOTH_MODE");
        app_ble_set_data_recv_callback(app_ble_data_recv_callback);
        app_ble_start();
    }
    start_webserver();
}