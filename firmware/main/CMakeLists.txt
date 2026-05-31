idf_component_register(
    SRCS
        "aodv_en_app_proto.c"
        "app_proto_example.c"
        "main.c"
        "app_demo.c"
        "app_flood.c"
    INCLUDE_DIRS "."
    REQUIRES aodv_en flood_en esp_wifi nvs_flash esp_event esp_netif esp_timer esp_driver_gpio
)
