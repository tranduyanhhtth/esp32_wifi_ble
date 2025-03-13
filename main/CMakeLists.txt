idf_component_register(SRCS "esp32_wifi_control_main.c" "gatts_demo.c"
                    INCLUDE_DIRS ""
                    REQUIRES nvs_flash esp_wifi esp_event esp_netif esp_http_server bt
                    EMBED_FILES "index.html")

# Add the include directories for the bt component
target_include_directories(${COMPONENT_LIB} PRIVATE "${IDF_PATH}/components/bt/include")
target_include_directories(${COMPONENT_LIB} PRIVATE "${IDF_PATH}/components/bt/include/esp32/include")
target_include_directories(${COMPONENT_LIB} PRIVATE "${IDF_PATH}/components/bt/host/bluedroid/api/include/api")