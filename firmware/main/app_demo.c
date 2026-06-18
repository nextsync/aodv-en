#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_demo.h"
#include "aodv_en.h"
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

// LED onboard usado para identificar visualmente o NODE de origem (ENABLE_DATA=y)
// e dar feedback de DATA recebido em qualquer no.
// GPIO 2 e o LED azul nas placas DevKit v1 e similares.
#define APP_BLINK_LED_GPIO       GPIO_NUM_2
#define APP_BLINK_PERIOD_MS      500
#define APP_DELIVER_PULSE_MS     150  // pulse curto no LED quando recebe DATA

#define APP_RX_QUEUE_LEN 8
#define APP_TX_RESULT_QUEUE_LEN 16
#define APP_LOOP_DELAY_MS 10
#define APP_STATSREP_MAGIC 0x53u
#define APP_STATSREP_LEN 17u
#define APP_NEIGHREP_MAGIC 0x4Eu
#define APP_NEIGH_MAX 16u
#define APP_MAX_FRAME_LEN ESP_NOW_MAX_DATA_LEN_V2
#define APP_DRIVER_MAX_PEERS CONFIG_AODV_EN_APP_DRIVER_MAX_PEERS

#if APP_DRIVER_MAX_PEERS < 2 || APP_DRIVER_MAX_PEERS > ESP_NOW_MAX_TOTAL_PEER_NUM
#error "CONFIG_AODV_EN_APP_DRIVER_MAX_PEERS must be in [2, ESP_NOW_MAX_TOTAL_PEER_NUM]"
#endif

#ifdef CONFIG_AODV_EN_APP_ENABLE_DATA
#define APP_ENABLE_DATA 1
#define APP_TARGET_MAC_TEXT CONFIG_AODV_EN_APP_TARGET_MAC
#define APP_PAYLOAD_TEXT_VALUE CONFIG_AODV_EN_APP_PAYLOAD_TEXT
#else
#define APP_ENABLE_DATA 0
#define APP_TARGET_MAC_TEXT ""
#define APP_PAYLOAD_TEXT_VALUE ""
#endif

#ifdef CONFIG_AODV_EN_APP_RREQ_FLOOD_UNICAST_SEQ
#define APP_RREQ_FLOOD_MODE AODV_EN_RREQ_FLOOD_UNICAST_SEQUENTIAL
#define APP_RREQ_FLOOD_MODE_TEXT "unicast_seq"
#else
#define APP_RREQ_FLOOD_MODE AODV_EN_RREQ_FLOOD_BROADCAST
#define APP_RREQ_FLOOD_MODE_TEXT "broadcast"
#endif

typedef struct
{
    uint8_t src_mac[AODV_EN_MAC_ADDR_LEN];
    int8_t rssi;
    uint16_t data_len;
    uint8_t data[APP_MAX_FRAME_LEN];
} app_rx_event_t;

typedef struct
{
    uint8_t dest_mac[AODV_EN_MAC_ADDR_LEN];
    bool success;
} app_tx_result_event_t;

typedef struct
{
    uint8_t mac[AODV_EN_MAC_ADDR_LEN];
    uint32_t last_used_ms;
    bool used;
    bool pinned;
} app_driver_peer_entry_t;

typedef struct
{
    aodv_en_stack_t stack;
    QueueHandle_t rx_queue;
    QueueHandle_t tx_result_queue;
    uint8_t self_mac[AODV_EN_MAC_ADDR_LEN];
    uint8_t target_mac[AODV_EN_MAC_ADDR_LEN];
    bool has_target;
    uint8_t wifi_channel;
    const char *node_name;
    const char *payload_text;
    uint32_t hello_interval_ms;
    uint32_t send_interval_ms;
    uint32_t print_interval_ms;
    uint32_t next_hello_at_ms;
    uint32_t next_send_at_ms;
    uint32_t next_print_at_ms;
    uint8_t report_to_mac[AODV_EN_MAC_ADDR_LEN];
    bool has_report;
    uint32_t report_interval_ms;
    uint32_t next_report_at_ms;
    uint8_t neigh_mac[APP_NEIGH_MAX][AODV_EN_MAC_ADDR_LEN];
    int16_t neigh_rssi[APP_NEIGH_MAX];
    uint8_t neigh_count;
    app_driver_peer_entry_t driver_peers[APP_DRIVER_MAX_PEERS];
} app_context_t;

static const char *TAG = "aodv_en_app";
static const uint8_t BROADCAST_MAC[AODV_EN_MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static app_context_t g_app;

static uint32_t app_now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static uint32_t app_adapter_now_ms(void *user_ctx)
{
    (void)user_ctx;
    return app_now_ms();
}

static void app_format_mac(const uint8_t mac[AODV_EN_MAC_ADDR_LEN], char *buffer, size_t buffer_len)
{
    if (buffer_len < 18u)
    {
        return;
    }

    (void)snprintf(
        buffer,
        buffer_len,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]);
}

static bool app_parse_mac(const char *text, uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    unsigned int parts[AODV_EN_MAC_ADDR_LEN];
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
        &parts[0],
        &parts[1],
        &parts[2],
        &parts[3],
        &parts[4],
        &parts[5]);

    if (count != 6)
    {
        return false;
    }

    for (size_t index = 0; index < AODV_EN_MAC_ADDR_LEN; index++)
    {
        mac[index] = (uint8_t)parts[index];
    }

    return true;
}

static bool app_mac_is_broadcast(const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    return memcmp(mac, BROADCAST_MAC, AODV_EN_MAC_ADDR_LEN) == 0;
}

static int app_driver_peer_find_index(
    const app_context_t *app,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    size_t index;

    if (app == NULL || mac == NULL)
    {
        return -1;
    }

    for (index = 0; index < APP_DRIVER_MAX_PEERS; index++)
    {
        if (app->driver_peers[index].used &&
            memcmp(app->driver_peers[index].mac, mac, AODV_EN_MAC_ADDR_LEN) == 0)
        {
            return (int)index;
        }
    }

    return -1;
}

static size_t app_driver_peer_count(
    const app_context_t *app)
{
    size_t index;
    size_t count = 0u;

    if (app == NULL)
    {
        return 0u;
    }

    for (index = 0; index < APP_DRIVER_MAX_PEERS; index++)
    {
        if (app->driver_peers[index].used)
        {
            count++;
        }
    }

    return count;
}

static int app_driver_peer_find_lru_evictable(
    const app_context_t *app)
{
    size_t index;
    int best_index = -1;

    if (app == NULL)
    {
        return -1;
    }

    for (index = 0; index < APP_DRIVER_MAX_PEERS; index++)
    {
        const app_driver_peer_entry_t *entry = &app->driver_peers[index];
        if (!entry->used || entry->pinned)
        {
            continue;
        }

        if (best_index < 0 ||
            entry->last_used_ms < app->driver_peers[(size_t)best_index].last_used_ms)
        {
            best_index = (int)index;
        }
    }

    return best_index;
}

static void app_driver_peer_touch(
    app_context_t *app,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
    bool pinned)
{
    int index;
    uint32_t now_ms;
    size_t free_index;

    if (app == NULL || mac == NULL)
    {
        return;
    }

    now_ms = app_now_ms();
    index = app_driver_peer_find_index(app, mac);
    if (index >= 0)
    {
        app->driver_peers[(size_t)index].last_used_ms = now_ms;
        if (pinned)
        {
            app->driver_peers[(size_t)index].pinned = true;
        }
        return;
    }

    for (free_index = 0; free_index < APP_DRIVER_MAX_PEERS; free_index++)
    {
        app_driver_peer_entry_t *entry = &app->driver_peers[free_index];
        if (entry->used)
        {
            continue;
        }

        memset(entry, 0, sizeof(*entry));
        memcpy(entry->mac, mac, AODV_EN_MAC_ADDR_LEN);
        entry->last_used_ms = now_ms;
        entry->used = true;
        entry->pinned = pinned;
        return;
    }
}

static void app_driver_peer_forget(
    app_context_t *app,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    int index;

    if (app == NULL || mac == NULL)
    {
        return;
    }

    index = app_driver_peer_find_index(app, mac);
    if (index < 0)
    {
        return;
    }

    memset(&app->driver_peers[(size_t)index], 0, sizeof(app->driver_peers[0]));
}

static esp_err_t app_ensure_peer(
    app_context_t *app,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
    uint8_t channel,
    bool pinned)
{
    esp_now_peer_info_t peer;
    esp_err_t err;

    if (app == NULL || mac == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (esp_now_is_peer_exist(mac))
    {
        app_driver_peer_touch(app, mac, pinned);
        return ESP_OK;
    }

    while (app_driver_peer_count(app) >= APP_DRIVER_MAX_PEERS)
    {
        int evict_index = app_driver_peer_find_lru_evictable(app);
        uint8_t evict_mac[AODV_EN_MAC_ADDR_LEN];
        char evict_text[18];

        if (evict_index < 0)
        {
            return ESP_ERR_ESPNOW_FULL;
        }

        memcpy(evict_mac, app->driver_peers[(size_t)evict_index].mac, AODV_EN_MAC_ADDR_LEN);
        err = esp_now_del_peer(evict_mac);
        if (err != ESP_OK && err != ESP_ERR_ESPNOW_NOT_FOUND)
        {
            return err;
        }

        app_format_mac(evict_mac, evict_text, sizeof(evict_text));
        ESP_LOGW(TAG, "evicted ESP-NOW peer via LRU: %s", evict_text);
        app_driver_peer_forget(app, evict_mac);
    }

    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, mac, AODV_EN_MAC_ADDR_LEN);
    peer.channel = channel;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    err = esp_now_add_peer(&peer);
    if (err == ESP_OK || err == ESP_ERR_ESPNOW_EXIST)
    {
        app_driver_peer_touch(app, mac, pinned);
        return ESP_OK;
    }

    if (err != ESP_ERR_ESPNOW_FULL)
    {
        return err;
    }

    while (true)
    {
        int evict_index = app_driver_peer_find_lru_evictable(app);
        uint8_t evict_mac[AODV_EN_MAC_ADDR_LEN];
        char evict_text[18];

        if (evict_index < 0)
        {
            return err;
        }

        memcpy(evict_mac, app->driver_peers[(size_t)evict_index].mac, AODV_EN_MAC_ADDR_LEN);
        err = esp_now_del_peer(evict_mac);
        if (err != ESP_OK && err != ESP_ERR_ESPNOW_NOT_FOUND)
        {
            return err;
        }

        app_format_mac(evict_mac, evict_text, sizeof(evict_text));
        ESP_LOGW(TAG, "evicted ESP-NOW peer after full table: %s", evict_text);
        app_driver_peer_forget(app, evict_mac);

        err = esp_now_add_peer(&peer);
        if (err == ESP_OK || err == ESP_ERR_ESPNOW_EXIST)
        {
            app_driver_peer_touch(app, mac, pinned);
            return ESP_OK;
        }

        if (err != ESP_ERR_ESPNOW_FULL)
        {
            return err;
        }
    }
}

static aodv_en_status_t app_emit_frame(
    void *user_ctx,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    bool broadcast)
{
    app_context_t *app = (app_context_t *)user_ctx;
#if CONFIG_AODV_EN_APP_FAIL_PERIOD_MS > 0
    {
        static bool s_fail_down = false;
        uint32_t fail_phase = app_now_ms() % CONFIG_AODV_EN_APP_FAIL_PERIOD_MS;
        bool fail_down = fail_phase < CONFIG_AODV_EN_APP_FAIL_DOWN_MS;
        if (fail_down != s_fail_down)
        {
            s_fail_down = fail_down;
            ESP_LOGW(TAG, "FAILSIM node %s t=%u", fail_down ? "DOWN" : "UP", (unsigned int)app_now_ms());
        }
        if (fail_down)
        {
            return AODV_EN_OK;
        }
    }
#endif
    const uint8_t *dest_mac = broadcast ? BROADCAST_MAC : next_hop;
    esp_err_t err;
    char mac_text[18];

    if (frame_len > ESP_NOW_MAX_DATA_LEN_V2)
    {
        ESP_LOGE(TAG, "frame too large for ESP-NOW v2: %u", (unsigned int)frame_len);
        return AODV_EN_ERR_SIZE;
    }

    err = app_ensure_peer(app, dest_mac, app->wifi_channel, broadcast);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to add peer: %s", esp_err_to_name(err));
        return AODV_EN_ERR_STATE;
    }

    err = esp_now_send(dest_mac, frame, frame_len);
    if (err != ESP_OK)
    {
        app_format_mac(dest_mac, mac_text, sizeof(mac_text));
        ESP_LOGE(TAG, "esp_now_send failed to %s: %s", mac_text, esp_err_to_name(err));
        return AODV_EN_ERR_STATE;
    }

    return AODV_EN_OK;
}

static void app_led_pulse_task(void *arg)
{
    (void)arg;
    // Garantir que o GPIO ja foi configurado (caso nao tenha vindo do blink_task).
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

static void app_put_u32_be(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)(value >> 24);
    buf[1] = (uint8_t)(value >> 16);
    buf[2] = (uint8_t)(value >> 8);
    buf[3] = (uint8_t)(value);
}

static uint32_t app_get_u32_be(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
}

static void app_deliver_data(
    void *user_ctx,
    const uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len)
{
    char mac_text[18];

    (void)user_ctx;
    app_format_mac(originator_mac, mac_text, sizeof(mac_text));

    if (payload_len >= APP_STATSREP_LEN && payload[0] == APP_STATSREP_MAGIC)
    {
        uint32_t tx = app_get_u32_be(&payload[1]);
        uint32_t rx = app_get_u32_be(&payload[5]);
        uint32_t control = app_get_u32_be(&payload[9]);
        uint32_t delivered = app_get_u32_be(&payload[13]);
        ESP_LOGI(TAG, "STATSREP node=%s tx=%" PRIu32 " rx=%" PRIu32 " control=%" PRIu32 " delivered=%" PRIu32,
                 mac_text, tx, rx, control, delivered);
        return;
    }

    if (payload_len >= 2u && payload[0] == APP_NEIGHREP_MAGIC)
    {
        uint8_t count = payload[1];
        char line[320];
        int pos = 0;
        pos += snprintf(line + pos, sizeof(line) - pos, "NEIGHREP node=%s hears=", mac_text);
        for (uint8_t i = 0; i < count && (size_t)(2u + (i + 1u) * 7u) <= payload_len; i++)
        {
            const uint8_t *e = &payload[2u + i * 7u];
            char nb[18];
            app_format_mac(e, nb, sizeof(nb));
            int8_t rssi = (int8_t)e[6];
            pos += snprintf(line + pos, sizeof(line) - pos, "%s%s:%d", (i == 0 ? "" : ","), nb, (int)rssi);
            if (pos >= (int)sizeof(line) - 24) break;
        }
        ESP_LOGI(TAG, "%s", line);
        return;
    }

    ESP_LOGI(TAG, "DATA deliver from %s: %.*s", mac_text, payload_len, (const char *)payload);

    // Pulse curto no LED para feedback visual no destino
    // (independente de o no ser origem ou nao).
    xTaskCreate(app_led_pulse_task, "led_pulse", 1024, NULL, 1, NULL);
}

static void app_ack_received(
    void *user_ctx,
    const uint8_t ack_sender_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t sequence_number,
    uint32_t rtt_ms)
{
    char mac_text[18];

    (void)user_ctx;
    app_format_mac(ack_sender_mac, mac_text, sizeof(mac_text));
    ESP_LOGI(TAG, "ACK received from %s for seq=%" PRIu32, mac_text, sequence_number);
    if (rtt_ms != AODV_EN_RTT_UNKNOWN)
    {
        ESP_LOGI(TAG, "LAT seq=%" PRIu32 " rtt_ms=%" PRIu32, sequence_number, rtt_ms);
    }
}

static void app_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    app_tx_result_event_t event;
    char mac_text[18];

    if (tx_info == NULL || tx_info->des_addr == NULL)
    {
        return;
    }

    if (app_mac_is_broadcast(tx_info->des_addr))
    {
        return;
    }

    memset(&event, 0, sizeof(event));
    memcpy(event.dest_mac, tx_info->des_addr, AODV_EN_MAC_ADDR_LEN);
    event.success = (status == ESP_NOW_SEND_SUCCESS);

    if (!event.success)
    {
        app_format_mac(tx_info->des_addr, mac_text, sizeof(mac_text));
        ESP_LOGW(TAG, "ESP-NOW send fail to %s", mac_text);
    }

    if (g_app.tx_result_queue != NULL &&
        xQueueSend(g_app.tx_result_queue, &event, 0) != pdTRUE)
    {
        ESP_EARLY_LOGW(TAG, "tx result queue full, dropping event");
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
    memcpy(event.src_mac, recv_info->src_addr, AODV_EN_MAC_ADDR_LEN);
    event.rssi = (recv_info->rx_ctrl != NULL) ? recv_info->rx_ctrl->rssi : 0;
    event.data_len = (uint16_t)data_len;
    memcpy(event.data, data, (size_t)data_len);

    if (xQueueSend(g_app.rx_queue, &event, 0) != pdTRUE)
    {
        ESP_EARLY_LOGW(TAG, "rx queue full, dropping frame");
    }
}

static void app_log_routes(const aodv_en_stack_t *stack)
{
    aodv_en_overview_t overview;
    size_t route_count;
    char dest_text[18];
    char next_hop_text[18];

    if (aodv_en_stack_get_overview(stack, &overview) != AODV_EN_OK)
    {
        ESP_LOGW(TAG, "failed to query stack overview");
        return;
    }

    ESP_LOGI(TAG, "routes=%u neighbors=%u tx=%" PRIu32 " rx=%" PRIu32 " delivered=%" PRIu32 " control=%" PRIu32 " acks=%" PRIu32,
             overview.routes_count,
             overview.neighbors_count,
             overview.stats.tx_frames,
             overview.stats.rx_frames,
             overview.stats.delivered_frames,
             overview.stats.control_tx_frames,
             overview.stats.ack_received);

    route_count = aodv_en_stack_get_route_count(stack);
    for (size_t index = 0; index < route_count; index++)
    {
        aodv_en_route_snapshot_t route;

        if (aodv_en_stack_get_route_at(stack, index, &route) != AODV_EN_OK)
        {
            break;
        }

        app_format_mac(route.destination_mac, dest_text, sizeof(dest_text));
        app_format_mac(route.next_hop_mac, next_hop_text, sizeof(next_hop_text));
        ESP_LOGI(TAG,
                 "route[%u] dest=%s via=%s hops=%u metric=%u state=%u expires=%" PRIu32,
                 (unsigned int)index,
                 dest_text,
                 next_hop_text,
                 route.hop_count,
                 route.metric,
                 route.state,
                 route.expires_at_ms);
    }
}

static void app_note_neigh(app_context_t *app, const uint8_t mac[AODV_EN_MAC_ADDR_LEN], int8_t rssi)
{
    for (uint8_t i = 0; i < app->neigh_count; i++)
    {
        if (memcmp(app->neigh_mac[i], mac, AODV_EN_MAC_ADDR_LEN) == 0)
        {
            app->neigh_rssi[i] = (int16_t)((app->neigh_rssi[i] * 3 + rssi) / 4);
            return;
        }
    }
    if (app->neigh_count < APP_NEIGH_MAX)
    {
        memcpy(app->neigh_mac[app->neigh_count], mac, AODV_EN_MAC_ADDR_LEN);
        app->neigh_rssi[app->neigh_count] = rssi;
        app->neigh_count++;
    }
}

static void app_process_rx_queue(app_context_t *app)
{
    app_rx_event_t event;

    while (xQueueReceive(app->rx_queue, &event, 0) == pdTRUE)
    {
        char src_text[18];
        app_format_mac(event.src_mac, src_text, sizeof(src_text));
        ESP_LOGI(TAG, "RSSIPROBE src=%s rssi=%d", src_text, (int)event.rssi);
        app_note_neigh(app, event.src_mac, event.rssi);

        (void)aodv_en_stack_on_recv_at(
            &app->stack,
            event.src_mac,
            event.data,
            event.data_len,
            event.rssi,
            app_now_ms());
    }
}

static void app_process_tx_result_queue(app_context_t *app)
{
    app_tx_result_event_t event;

    while (xQueueReceive(app->tx_result_queue, &event, 0) == pdTRUE)
    {
        size_t invalidated = 0u;
        aodv_en_status_t status = aodv_en_stack_on_link_tx_result_at(
            &app->stack,
            event.dest_mac,
            event.success,
            app_now_ms(),
            &invalidated);

        if (!event.success && invalidated > 0u)
        {
            char mac_text[18];
            app_format_mac(event.dest_mac, mac_text, sizeof(mac_text));
            ESP_LOGW(
                TAG,
                "invalidated %u route(s) via %s after link failures (status=%d)",
                (unsigned int)invalidated,
                mac_text,
                status);
        }
    }
}

static void app_protocol_task(void *arg)
{
    app_context_t *app = (app_context_t *)arg;

    for (;;)
    {
        uint32_t now_ms = app_now_ms();
        aodv_en_status_t status;

        app_process_rx_queue(app);
        app_process_tx_result_queue(app);
        aodv_en_stack_tick_at(&app->stack, now_ms);

        if (now_ms >= app->next_hello_at_ms)
        {
            status = aodv_en_stack_send_hello_at(&app->stack, now_ms);
            if (status != AODV_EN_OK)
            {
                ESP_LOGW(TAG, "HELLO send status=%d", status);
            }
            app->next_hello_at_ms = now_ms + app->hello_interval_ms;
        }

        if (APP_ENABLE_DATA && app->has_target && now_ms >= app->next_send_at_ms)
        {
            status = aodv_en_stack_send_data_at(
                &app->stack,
                app->target_mac,
                (const uint8_t *)app->payload_text,
                (uint16_t)strlen(app->payload_text),
                true,
                now_ms);

            if (status == AODV_EN_QUEUED)
            {
                ESP_LOGI(TAG, "DATA queued while route discovery is in progress");
            }
            else if (status != AODV_EN_OK)
            {
                ESP_LOGW(TAG, "DATA send status=%d", status);
            }
            else
            {
                ESP_LOGI(TAG, "DATA queued to route");
            }

            app->next_send_at_ms = now_ms + app->send_interval_ms;
        }

        if (now_ms >= app->next_print_at_ms)
        {
            char self_text[18];
            app_format_mac(app->self_mac, self_text, sizeof(self_text));
            ESP_LOGI(TAG, "RSSISELF self=%s", self_text);
            app_log_routes(&app->stack);
            app->next_print_at_ms = now_ms + app->print_interval_ms;
        }

        if (app->has_report && now_ms >= app->next_report_at_ms)
        {
            aodv_en_overview_t overview;
            if (aodv_en_stack_get_overview(&app->stack, &overview) == AODV_EN_OK)
            {
                uint8_t payload[APP_STATSREP_LEN];
                payload[0] = APP_STATSREP_MAGIC;
                app_put_u32_be(&payload[1], overview.stats.tx_frames);
                app_put_u32_be(&payload[5], overview.stats.rx_frames);
                app_put_u32_be(&payload[9], overview.stats.control_tx_frames);
                app_put_u32_be(&payload[13], overview.stats.delivered_frames);
                (void)aodv_en_stack_send_data_at(
                    &app->stack, app->report_to_mac, payload, APP_STATSREP_LEN, true, now_ms);
            }

            {
                uint8_t np[2u + APP_NEIGH_MAX * 7u];
                uint16_t off = 0u;
                np[off++] = APP_NEIGHREP_MAGIC;
                np[off++] = app->neigh_count;
                for (uint8_t i = 0; i < app->neigh_count; i++)
                {
                    memcpy(&np[off], app->neigh_mac[i], AODV_EN_MAC_ADDR_LEN);
                    off += AODV_EN_MAC_ADDR_LEN;
                    np[off++] = (uint8_t)(int8_t)app->neigh_rssi[i];
                }
                (void)aodv_en_stack_send_data_at(
                    &app->stack, app->report_to_mac, np, off, true, now_ms);
            }

            app->next_report_at_ms = now_ms + app->report_interval_ms;
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

static bool app_is_connected(app_context_t *app)
{
    if (app->has_report)
    {
        size_t count = aodv_en_stack_get_route_count(&app->stack);
        for (size_t index = 0; index < count; index++)
        {
            aodv_en_route_snapshot_t route;
            if (aodv_en_stack_get_route_at(&app->stack, index, &route) != AODV_EN_OK)
            {
                continue;
            }
            if (route.state == AODV_EN_ROUTE_VALID &&
                memcmp(route.destination_mac, app->report_to_mac, AODV_EN_MAC_ADDR_LEN) == 0)
            {
                return true;
            }
        }
        return false;
    }
    aodv_en_overview_t overview;
    if (aodv_en_stack_get_overview(&app->stack, &overview) != AODV_EN_OK)
    {
        return false;
    }
    return overview.neighbors_count > 0;
}

static void app_blink_task(void *arg)
{
    app_context_t *app = (app_context_t *)arg;
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
        if (app_is_connected(app))
        {
            level = !level;
            gpio_set_level(APP_BLINK_LED_GPIO, level);
        }
        else
        {
            level = false;
            gpio_set_level(APP_BLINK_LED_GPIO, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(APP_BLINK_PERIOD_MS));
    }
}

static void app_init_espnow(uint8_t channel)
{
    uint32_t version = 0;

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(app_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(app_recv_cb));
    ESP_ERROR_CHECK(app_ensure_peer(&g_app, BROADCAST_MAC, channel, true));
    ESP_ERROR_CHECK(esp_now_get_version(&version));
    ESP_LOGI(TAG, "ESP-NOW version=%" PRIu32, version);
}

void app_demo_run(void)
{
    aodv_en_config_t node_config;
    aodv_en_adapter_t adapter;
    aodv_en_app_callbacks_t app_callbacks;
    char self_mac_text[18];
    char target_mac_text[18];

    memset(&g_app, 0, sizeof(g_app));
    g_app.node_name = CONFIG_AODV_EN_APP_NODE_NAME;
    g_app.payload_text = APP_PAYLOAD_TEXT_VALUE;
    g_app.wifi_channel = CONFIG_AODV_EN_APP_WIFI_CHANNEL;
    g_app.hello_interval_ms = CONFIG_AODV_EN_APP_HELLO_INTERVAL_MS;
    g_app.send_interval_ms = CONFIG_AODV_EN_APP_SEND_INTERVAL_MS;
    g_app.print_interval_ms = CONFIG_AODV_EN_APP_PRINT_INTERVAL_MS;

    app_init_nvs();
    app_init_wifi(g_app.wifi_channel);
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, g_app.self_mac));
    app_init_espnow(g_app.wifi_channel);

    aodv_en_config_set_defaults(&node_config);
    node_config.network_id = CONFIG_AODV_EN_APP_NETWORK_ID;
    node_config.wifi_channel = g_app.wifi_channel;
    node_config.ack_timeout_ms = AODV_EN_ACK_TIMEOUT_MS_DEFAULT;
    node_config.rreq_flood_mode = APP_RREQ_FLOOD_MODE;
    node_config.link_fail_threshold = AODV_EN_LINK_FAIL_THRESHOLD;
    node_config.route_metric_hop_weight = (uint8_t)CONFIG_AODV_EN_APP_ROUTE_METRIC_HOP_WEIGHT;
    node_config.route_metric_rssi_weight = (uint8_t)CONFIG_AODV_EN_APP_ROUTE_METRIC_RSSI_WEIGHT;
    node_config.route_metric_rssi_best_dbm = (int8_t)CONFIG_AODV_EN_APP_ROUTE_RSSI_BEST_DBM;
    node_config.route_metric_rssi_worst_dbm = (int8_t)CONFIG_AODV_EN_APP_ROUTE_RSSI_WORST_DBM;

    memset(&adapter, 0, sizeof(adapter));
    adapter.user_ctx = &g_app;
    adapter.now_ms = app_adapter_now_ms;
    adapter.tx_frame = app_emit_frame;

    memset(&app_callbacks, 0, sizeof(app_callbacks));
    app_callbacks.user_ctx = &g_app;
    app_callbacks.on_data = app_deliver_data;
    app_callbacks.on_ack = app_ack_received;

    ESP_ERROR_CHECK(
        aodv_en_stack_init(&g_app.stack, &node_config, g_app.self_mac, &adapter, &app_callbacks) == AODV_EN_OK
            ? ESP_OK
            : ESP_FAIL);

    g_app.rx_queue = xQueueCreate(APP_RX_QUEUE_LEN, sizeof(app_rx_event_t));
    assert(g_app.rx_queue != NULL);
    g_app.tx_result_queue = xQueueCreate(APP_TX_RESULT_QUEUE_LEN, sizeof(app_tx_result_event_t));
    assert(g_app.tx_result_queue != NULL);

    g_app.has_target = app_parse_mac(APP_TARGET_MAC_TEXT, g_app.target_mac);
    if (g_app.has_target && memcmp(g_app.target_mac, g_app.self_mac, AODV_EN_MAC_ADDR_LEN) == 0)
    {
        ESP_LOGE(TAG, "DATA target MAC equals self MAC; disabling periodic DATA");
        g_app.has_target = false;
    }

    g_app.report_interval_ms = (uint32_t)CONFIG_AODV_EN_APP_REPORT_INTERVAL_MS;
    g_app.has_report = app_parse_mac(CONFIG_AODV_EN_APP_REPORT_TO_MAC, g_app.report_to_mac);
    if (g_app.has_report && memcmp(g_app.report_to_mac, g_app.self_mac, AODV_EN_MAC_ADDR_LEN) == 0)
    {
        g_app.has_report = false;
    }

    g_app.next_hello_at_ms = app_now_ms() + 1000u;
    g_app.next_send_at_ms = app_now_ms() + 3000u;
    g_app.next_print_at_ms = app_now_ms() + g_app.print_interval_ms;
    g_app.next_report_at_ms = app_now_ms() + g_app.report_interval_ms;

    app_format_mac(g_app.self_mac, self_mac_text, sizeof(self_mac_text));
    ESP_LOGI(TAG, "node=%s self_mac=%s channel=%u network_id=0x%08" PRIX32 " rreq_flood=%s max_driver_peers=%d",
             g_app.node_name,
             self_mac_text,
             g_app.wifi_channel,
             node_config.network_id,
             APP_RREQ_FLOOD_MODE_TEXT,
             APP_DRIVER_MAX_PEERS);
    ESP_LOGI(TAG, "route_metric hop_w=%u rssi_w=%u best=%d worst=%d",
             (unsigned int)node_config.route_metric_hop_weight,
             (unsigned int)node_config.route_metric_rssi_weight,
             (int)node_config.route_metric_rssi_best_dbm,
             (int)node_config.route_metric_rssi_worst_dbm);

    if (g_app.has_target)
    {
        app_format_mac(g_app.target_mac, target_mac_text, sizeof(target_mac_text));
        ESP_LOGI(TAG, "periodic DATA enabled target=%s payload=\"%s\"", target_mac_text, g_app.payload_text);
    }
    else if (APP_ENABLE_DATA)
    {
        ESP_LOGW(TAG, "DATA enabled but target MAC invalid or empty");
    }

    xTaskCreate(app_protocol_task, "aodv_en_task", 8192, &g_app, 5, NULL);

    ESP_LOGI(TAG, "blink LED on GPIO%d (azul = conectado a malha/coletor)", APP_BLINK_LED_GPIO);
    xTaskCreate(app_blink_task, "blink", 2048, &g_app, 1, NULL);
}
