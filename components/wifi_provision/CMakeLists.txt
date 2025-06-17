idf_component_register(
    SRCS "wifi_provision.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_http_server esp_netif esp_event esp_wifi)
