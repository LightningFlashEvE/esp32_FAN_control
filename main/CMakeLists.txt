idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        nvs_flash
        esp_http_server
        esp_netif
        esp_event
        esp_wifi
        mqtt
        wifi_provision
        temp_sensor
        fan_control
        user_input
        oled_display
        mqtt_comm
)
