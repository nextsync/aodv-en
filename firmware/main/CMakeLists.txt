idf_component_register(
    SRCS
        "main.c"
        "app_demo.c"
    INCLUDE_DIRS "."
    REQUIRES aodv_en esp_wifi nvs_flash esp_event esp_netif esp_timer
)
