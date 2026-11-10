idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_wifi esp_http_server esp_event esp_netif nvs_flash
)