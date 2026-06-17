#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_flood.h"
#include "flood_en.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#define APP_BLINK_LED_GPIO   GPIO_NUM_2
#define APP_BLINK_PERIOD_MS  500
#define APP_DELIVER_PULSE_MS 150

#define APP_RX_QUEUE_LEN 8
#define APP_LOOP_DELAY_MS 10
#define APP_MAX_FRAME_LEN ESP_NOW_MAX_DATA_LEN_V2

#ifdef CONFIG_AODV_EN_APP_ENABLE_DATA
#define APP_ENABLE_DATA 1
#define APP_TARGET_MAC_TEXT CONFIG_AODV_EN_APP_TARGET_MAC
#define APP_PAYLOAD_TEXT_VALUE CONFIG_AODV_EN_APP_PAYLOAD_TEXT
#else
#define APP_ENABLE_DATA 0
#define APP_TARGET_MAC_TEXT ""
#define APP_PAYLOAD_TEXT_VALUE ""
#endif

typedef struct
{
    uint8_t src_mac[FLOOD_EN_MAC_ADDR_LEN];
    int8_t rssi;
    uint16_t data_len;
    uint8_t data[APP_MAX_FRAME_LEN];
} app_rx_event_t;

#define APP_MAX_NEIGHBORS 16

typedef struct
{
    flood_en_node_t node;
    QueueHandle_t rx_queue;
    uint8_t self_mac[FLOOD_EN_MAC_ADDR_LEN];
    uint8_t target_mac[FLOOD_EN_MAC_ADDR_LEN];
    bool has_target;
    uint8_t wifi_channel;
    const char *node_name;
    const char *payload_text;
    uint32_t send_interval_ms;
    uint32_t print_interval_ms;
    uint32_t next_send_at_ms;
    uint32_t next_print_at_ms;
    uint8_t neighbors[APP_MAX_NEIGHBORS][FLOOD_EN_MAC_ADDR_LEN];
    uint8_t neighbor_count;
} app_context_t;

static const char *TAG = "flood_en_app";
static const uint8_t BROADCAST_MAC[FLOOD_EN_MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static app_context_t g_app;

static uint32_t app_now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static void app_format_mac(const uint8_t mac[FLOOD_EN_MAC_ADDR_LEN], char *buffer, size_t buffer_len)
{
    if (buffer_len < 18u)
    {
        return;
    }

    (void)snprintf(
        buffer,
        buffer_len,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static bool app_parse_mac(const char *text, uint8_t mac[FLOOD_EN_MAC_ADDR_LEN])
{
    unsigned int parts[FLOOD_EN_MAC_ADDR_LEN];
    int count;

    if (text == NULL)
    {
        return false;
    }

    while (*text != '\0' && isspace((unsigned char)*text))
    {
        text++;
    }

    if (*text == '\0')
    {
        return false;
    }

    count = sscanf(
        text,
        "%2x:%2x:%2x:%2x:%2x:%2x",
        &parts[0], &parts[1], &parts[2], &parts[3], &parts[4], &parts[5]);

    if (count != 6)
    {
        return false;
    }

    for (size_t index = 0; index < FLOOD_EN_MAC_ADDR_LEN; index++)
    {
        mac[index] = (uint8_t)parts[index];
    }

    return true;
}

static esp_err_t app_ensure_peer(const uint8_t mac[FLOOD_EN_MAC_ADDR_LEN], uint8_t channel)
{
    esp_now_peer_info_t peer;

    if (esp_now_is_peer_exist(mac))
    {
        return ESP_OK;
    }

    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, mac, FLOOD_EN_MAC_ADDR_LEN);
    peer.channel = channel;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    return esp_now_add_peer(&peer);
}

static void app_note_neighbor(app_context_t *app, const uint8_t mac[FLOOD_EN_MAC_ADDR_LEN])
{
    if (memcmp(mac, BROADCAST_MAC, FLOOD_EN_MAC_ADDR_LEN) == 0 ||
        memcmp(mac, app->self_mac, FLOOD_EN_MAC_ADDR_LEN) == 0)
    {
        return;
    }

    for (uint8_t i = 0; i < app->neighbor_count; i++)
    {
        if (memcmp(app->neighbors[i], mac, FLOOD_EN_MAC_ADDR_LEN) == 0)
        {
            return;
        }
    }

    if (app->neighbor_count < APP_MAX_NEIGHBORS)
    {
        memcpy(app->neighbors[app->neighbor_count], mac, FLOOD_EN_MAC_ADDR_LEN);
        app->neighbor_count++;
    }
}

static flood_en_status_t app_send_one(
    app_context_t *app,
    const uint8_t mac[FLOOD_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len)
{
    esp_err_t err;
    char mac_text[18];

    err = app_ensure_peer(mac, app->wifi_channel);
    if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST)
    {
        ESP_LOGE(TAG, "failed to add peer: %s", esp_err_to_name(err));
        return FLOOD_EN_ERR_STATE;
    }

    err = esp_now_send(mac, frame, frame_len);
    if (err != ESP_OK)
    {
        app_format_mac(mac, mac_text, sizeof(mac_text));
        ESP_LOGE(TAG, "esp_now_send failed to %s: %s", mac_text, esp_err_to_name(err));
        return FLOOD_EN_ERR_STATE;
    }

    return FLOOD_EN_OK;
}

static flood_en_status_t app_emit_frame(
    void *user_ctx,
    const uint8_t next_hop[FLOOD_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    bool broadcast)
{
    app_context_t *app = (app_context_t *)user_ctx;
    flood_en_status_t status = FLOOD_EN_OK;

    if (frame_len > ESP_NOW_MAX_DATA_LEN_V2)
    {
        ESP_LOGE(TAG, "frame too large for ESP-NOW v2: %u", (unsigned int)frame_len);
        return FLOOD_EN_ERR_SIZE;
    }

    if (!broadcast)
    {
        return app_send_one(app, next_hop, frame, frame_len);
    }

    // Flooding controlado (TCC 4.6.1d): disseminar por UNICAST para cada vizinho
    // conhecido. Antes de conhecer vizinhos (bootstrap), usa broadcast.
    if (app->neighbor_count == 0)
    {
        return app_send_one(app, BROADCAST_MAC, frame, frame_len);
    }

    for (uint8_t i = 0; i < app->neighbor_count; i++)
    {
        if (app_send_one(app, app->neighbors[i], frame, frame_len) != FLOOD_EN_OK)
        {
            status = FLOOD_EN_ERR_STATE;
        }
    }

    return status;
}

static void app_led_pulse_task(void *arg)
{
    (void)arg;
    static bool gpio_ready = false;
    if (!gpio_ready)
    {
        gpio_config_t cfg = {
            .pin_bit_mask = 1ULL << APP_BLINK_LED_GPIO,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&cfg);
        gpio_ready = true;
    }
    gpio_set_level(APP_BLINK_LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(APP_DELIVER_PULSE_MS));
    gpio_set_level(APP_BLINK_LED_GPIO, 0);
    vTaskDelete(NULL);
}

static void app_deliver_data(
    void *user_ctx,
    const uint8_t originator_mac[FLOOD_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len)
{
    char mac_text[18];

    (void)user_ctx;
    app_format_mac(originator_mac, mac_text, sizeof(mac_text));
    ESP_LOGI(TAG, "DATA deliver from %s: %.*s", mac_text, payload_len, (const char *)payload);

    xTaskCreate(app_led_pulse_task, "led_pulse", 1024, NULL, 1, NULL);
}

static void app_ack_received(
    void *user_ctx,
    const uint8_t ack_sender_mac[FLOOD_EN_MAC_ADDR_LEN],
    uint32_t sequence_number,
    uint32_t rtt_ms)
{
    char mac_text[18];

    (void)user_ctx;
    app_format_mac(ack_sender_mac, mac_text, sizeof(mac_text));
    ESP_LOGI(TAG, "ACK received from %s for seq=%" PRIu32, mac_text, sequence_number);
    if (rtt_ms != FLOOD_EN_RTT_UNKNOWN)
    {
        ESP_LOGI(TAG, "LAT seq=%" PRIu32 " rtt_ms=%" PRIu32, sequence_number, rtt_ms);
    }
}

static void app_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len)
{
    app_rx_event_t event;

    if (recv_info == NULL || recv_info->src_addr == NULL || data == NULL || data_len <= 0)
    {
        return;
    }

    if (data_len > (int)sizeof(event.data))
    {
        return;
    }

    memset(&event, 0, sizeof(event));
    memcpy(event.src_mac, recv_info->src_addr, FLOOD_EN_MAC_ADDR_LEN);
    event.rssi = (recv_info->rx_ctrl != NULL) ? recv_info->rx_ctrl->rssi : 0;
    event.data_len = (uint16_t)data_len;
    memcpy(event.data, data, (size_t)data_len);

    if (xQueueSend(g_app.rx_queue, &event, 0) != pdTRUE)
    {
        ESP_EARLY_LOGW(TAG, "rx queue full, dropping frame");
    }
}

static void app_log_stats(const app_context_t *app)
{
    const flood_en_stats_t *s = &app->node.stats;

    ESP_LOGI(TAG,
             "stats tx=%" PRIu32 " rx=%" PRIu32 " rebroadcast=%" PRIu32
             " delivered=%" PRIu32 " ack=%" PRIu32 " dup=%" PRIu32 " ttl_drop=%" PRIu32,
             s->tx_frames,
             s->rx_frames,
             s->rebroadcast_frames,
             s->delivered_frames,
             s->ack_received,
             s->duplicate_drops,
             s->ttl_drops);
}

static void app_process_rx_queue(app_context_t *app)
{
    app_rx_event_t event;

    while (xQueueReceive(app->rx_queue, &event, 0) == pdTRUE)
    {
        app_note_neighbor(app, event.src_mac);
        (void)flood_en_node_on_recv(
            &app->node,
            event.src_mac,
            event.data,
            event.data_len,
            event.rssi,
            app_now_ms());
    }
}

static void app_protocol_task(void *arg)
{
    app_context_t *app = (app_context_t *)arg;

    for (;;)
    {
        uint32_t now_ms = app_now_ms();
        flood_en_status_t status;

        app_process_rx_queue(app);
        flood_en_node_tick(&app->node, now_ms);

        if (APP_ENABLE_DATA && app->has_target && now_ms >= app->next_send_at_ms)
        {
            status = flood_en_node_send_data(
                &app->node,
                app->target_mac,
                (const uint8_t *)app->payload_text,
                (uint16_t)strlen(app->payload_text),
                true,
                now_ms);

            if (status != FLOOD_EN_OK)
            {
                ESP_LOGW(TAG, "flood DATA send status=%d", status);
            }
            else
            {
                ESP_LOGI(TAG, "flood DATA broadcast");
            }

            app->next_send_at_ms = now_ms + app->send_interval_ms;
        }

        if (now_ms >= app->next_print_at_ms)
        {
            app_log_stats(app);
            app->next_print_at_ms = now_ms + app->print_interval_ms;
        }

        vTaskDelay(pdMS_TO_TICKS(APP_LOOP_DELAY_MS));
    }
}

static void app_init_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);
}

static void app_init_wifi(uint8_t channel)
{
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {0};

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    (void)esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE));
    if (CONFIG_AODV_EN_APP_TX_POWER_QDBM > 0)
    {
        ESP_ERROR_CHECK(esp_wifi_set_max_tx_power((int8_t)CONFIG_AODV_EN_APP_TX_POWER_QDBM));
    }
    {
        int8_t tx_power = 0;
        (void)esp_wifi_get_max_tx_power(&tx_power);
        ESP_LOGI(TAG, "wifi channel=%u tx_power_qdbm=%d", channel, (int)tx_power);
    }
}

static void app_blink_task(void *arg)
{
    (void)arg;
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << APP_BLINK_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    bool level = false;
    for (;;)
    {
        level = !level;
        gpio_set_level(APP_BLINK_LED_GPIO, level);
        vTaskDelay(pdMS_TO_TICKS(APP_BLINK_PERIOD_MS));
    }
}

static void app_init_espnow(uint8_t channel)
{
    uint32_t version = 0;

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(app_recv_cb));
    ESP_ERROR_CHECK(app_ensure_peer(BROADCAST_MAC, channel));
    ESP_ERROR_CHECK(esp_now_get_version(&version));
    ESP_LOGI(TAG, "ESP-NOW version=%" PRIu32, version);
}

void app_flood_run(void)
{
    flood_en_config_t node_config;
    flood_en_callbacks_t callbacks;
    char self_mac_text[18];
    char target_mac_text[18];

    memset(&g_app, 0, sizeof(g_app));
    g_app.node_name = CONFIG_AODV_EN_APP_NODE_NAME;
    g_app.payload_text = APP_PAYLOAD_TEXT_VALUE;
    g_app.wifi_channel = CONFIG_AODV_EN_APP_WIFI_CHANNEL;
    g_app.send_interval_ms = CONFIG_AODV_EN_APP_SEND_INTERVAL_MS;
    g_app.print_interval_ms = CONFIG_AODV_EN_APP_PRINT_INTERVAL_MS;

    app_init_nvs();
    app_init_wifi(g_app.wifi_channel);
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, g_app.self_mac));
    app_init_espnow(g_app.wifi_channel);

    flood_en_config_set_defaults(&node_config);
    node_config.network_id = CONFIG_AODV_EN_APP_NETWORK_ID;

    ESP_ERROR_CHECK(
        flood_en_node_init(&g_app.node, &node_config, g_app.self_mac) == FLOOD_EN_OK
            ? ESP_OK
            : ESP_FAIL);

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.tx_frame = app_emit_frame;
    callbacks.deliver_data = app_deliver_data;
    callbacks.ack_received = app_ack_received;
    callbacks.user_ctx = &g_app;
    flood_en_node_set_callbacks(&g_app.node, &callbacks);

    g_app.rx_queue = xQueueCreate(APP_RX_QUEUE_LEN, sizeof(app_rx_event_t));
    assert(g_app.rx_queue != NULL);

    g_app.has_target = app_parse_mac(APP_TARGET_MAC_TEXT, g_app.target_mac);
    if (g_app.has_target && memcmp(g_app.target_mac, g_app.self_mac, FLOOD_EN_MAC_ADDR_LEN) == 0)
    {
        ESP_LOGE(TAG, "DATA target MAC equals self MAC; disabling periodic DATA");
        g_app.has_target = false;
    }

    g_app.next_send_at_ms = app_now_ms() + 3000u;
    g_app.next_print_at_ms = app_now_ms() + g_app.print_interval_ms;

    app_format_mac(g_app.self_mac, self_mac_text, sizeof(self_mac_text));
    ESP_LOGI(TAG, "node=%s self_mac=%s channel=%u network_id=0x%08" PRIX32 " mode=FLOOD",
             g_app.node_name,
             self_mac_text,
             g_app.wifi_channel,
             node_config.network_id);

    if (g_app.has_target)
    {
        app_format_mac(g_app.target_mac, target_mac_text, sizeof(target_mac_text));
        ESP_LOGI(TAG, "periodic flood DATA enabled target=%s payload=\"%s\"", target_mac_text, g_app.payload_text);
    }
    else if (APP_ENABLE_DATA)
    {
        ESP_LOGW(TAG, "DATA enabled but target MAC invalid or empty");
    }

    xTaskCreate(app_protocol_task, "flood_en_task", 8192, &g_app, 5, NULL);

    if (g_app.has_target)
    {
        ESP_LOGI(TAG, "blink LED on GPIO%d (origem do flooding)", APP_BLINK_LED_GPIO);
        xTaskCreate(app_blink_task, "blink", 1024, NULL, 1, NULL);
    }
}
