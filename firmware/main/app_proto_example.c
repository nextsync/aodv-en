#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>

#include "aodv_en.h"
#include "aodv_en_app_proto.h"
#include "app_proto_example.h"
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

#define APP_RX_QUEUE_LEN 8
#define APP_TX_RESULT_QUEUE_LEN 16
#define APP_CLI_QUEUE_LEN 8
#define APP_LOOP_DELAY_MS 100
#define APP_MAX_FRAME_LEN ESP_NOW_MAX_DATA_LEN_V2
#define APP_CLI_LINE_MAX 192
#define APP_CLI_TEXT_MAX 120
#define APP_CLI_COMMAND_MAX 16
#define APP_CLI_ARGS_MAX 96

#ifndef CONFIG_AODV_EN_APP_PROTO_HEALTH_INTERVAL_MS
#define CONFIG_AODV_EN_APP_PROTO_HEALTH_INTERVAL_MS 10000
#endif

#ifndef CONFIG_AODV_EN_APP_PROTO_UNICAST_INTERVAL_MS
#define CONFIG_AODV_EN_APP_PROTO_UNICAST_INTERVAL_MS 15000
#endif

#ifdef CONFIG_AODV_EN_APP_PROTO_ENABLE_UNICAST
#define APP_PROTO_ENABLE_UNICAST 1
#define APP_PROTO_TARGET_MAC_TEXT CONFIG_AODV_EN_APP_PROTO_TARGET_MAC
#define APP_PROTO_TEXT_VALUE CONFIG_AODV_EN_APP_PROTO_TEXT
#define APP_PROTO_COMMAND_VALUE CONFIG_AODV_EN_APP_PROTO_COMMAND
#define APP_PROTO_COMMAND_ARGS_VALUE CONFIG_AODV_EN_APP_PROTO_COMMAND_ARGS
#else
#define APP_PROTO_ENABLE_UNICAST 0
#define APP_PROTO_TARGET_MAC_TEXT ""
#define APP_PROTO_TEXT_VALUE ""
#define APP_PROTO_COMMAND_VALUE ""
#define APP_PROTO_COMMAND_ARGS_VALUE ""
#endif

#ifdef CONFIG_AODV_EN_APP_PROTO_ENABLE_CLI
#define APP_PROTO_ENABLE_CLI 1
#else
#define APP_PROTO_ENABLE_CLI 0
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

typedef enum
{
    APP_CLI_EVT_HEALTH_BROADCAST = 1,
    APP_CLI_EVT_HEALTH_UNICAST = 2,
    APP_CLI_EVT_TEXT = 3,
    APP_CLI_EVT_CMD = 4,
    APP_CLI_EVT_PRINT_ROUTES = 5,
} app_cli_event_type_t;

typedef struct
{
    app_cli_event_type_t type;
    uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN];
    char text[APP_CLI_TEXT_MAX];
    char command[APP_CLI_COMMAND_MAX];
    char args[APP_CLI_ARGS_MAX];
} app_cli_event_t;

typedef struct
{
    aodv_en_stack_t stack;
    aodv_en_app_proto_t proto;
    QueueHandle_t rx_queue;
    QueueHandle_t tx_result_queue;
    QueueHandle_t cli_queue;
    uint8_t self_mac[AODV_EN_MAC_ADDR_LEN];
    uint8_t target_mac[AODV_EN_MAC_ADDR_LEN];
    bool has_target;
    uint8_t wifi_channel;
    const char *node_name;
    const char *unicast_text;
    const char *command_name;
    const char *command_args;
    uint32_t hello_interval_ms;
    uint32_t print_interval_ms;
    uint32_t health_interval_ms;
    uint32_t unicast_interval_ms;
    uint32_t next_hello_at_ms;
    uint32_t next_print_at_ms;
    uint32_t next_health_at_ms;
    uint32_t next_unicast_at_ms;
} app_context_t;

static const char *TAG = "aodv_en_proto";
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

static char *app_skip_spaces(char *text)
{
    while (text != NULL && *text != '\0' && isspace((unsigned char)*text))
    {
        text++;
    }
    return text;
}

static void app_trim_right(char *text)
{
    size_t len;

    if (text == NULL)
    {
        return;
    }

    len = strlen(text);
    while (len > 0u && isspace((unsigned char)text[len - 1u]))
    {
        text[len - 1u] = '\0';
        len--;
    }
}

static void app_log_cli_help(void)
{
    ESP_LOGI(TAG, "CLI commands:");
    ESP_LOGI(TAG, "  help");
    ESP_LOGI(TAG, "  health all");
    ESP_LOGI(TAG, "  health <AA:BB:CC:DD:EE:FF>");
    ESP_LOGI(TAG, "  text <AA:BB:CC:DD:EE:FF> <mensagem>");
    ESP_LOGI(TAG, "  cmd <AA:BB:CC:DD:EE:FF> <comando> [args]");
    ESP_LOGI(TAG, "  routes");
}

static bool app_cli_parse_line(
    const char *line_in,
    app_cli_event_t *event)
{
    char line[APP_CLI_LINE_MAX];
    char *cursor;
    char *token;
    char *next;

    if (line_in == NULL || event == NULL)
    {
        return false;
    }

    memset(event, 0, sizeof(*event));
    strncpy(line, line_in, sizeof(line) - 1u);
    app_trim_right(line);
    cursor = app_skip_spaces(line);
    if (cursor == NULL || *cursor == '\0')
    {
        return false;
    }

    if (strcasecmp(cursor, "help") == 0)
    {
        app_log_cli_help();
        return false;
    }

    if (strcasecmp(cursor, "routes") == 0)
    {
        event->type = APP_CLI_EVT_PRINT_ROUTES;
        return true;
    }

    if (strncasecmp(cursor, "health", 6) == 0 && (cursor[6] == '\0' || isspace((unsigned char)cursor[6])))
    {
        cursor = app_skip_spaces(cursor + 6);
        if (cursor == NULL || *cursor == '\0')
        {
            ESP_LOGW(TAG, "usage: health all|<mac>");
            return false;
        }

        if (strcasecmp(cursor, "all") == 0)
        {
            event->type = APP_CLI_EVT_HEALTH_BROADCAST;
            return true;
        }

        if (!app_parse_mac(cursor, event->destination_mac))
        {
            ESP_LOGW(TAG, "invalid MAC for health: %s", cursor);
            return false;
        }

        event->type = APP_CLI_EVT_HEALTH_UNICAST;
        return true;
    }

    if (strncasecmp(cursor, "text", 4) == 0 && (cursor[4] == '\0' || isspace((unsigned char)cursor[4])))
    {
        cursor = app_skip_spaces(cursor + 4);
        if (cursor == NULL || *cursor == '\0')
        {
            ESP_LOGW(TAG, "usage: text <mac> <mensagem>");
            return false;
        }

        token = cursor;
        next = token;
        while (*next != '\0' && !isspace((unsigned char)*next))
        {
            next++;
        }
        if (*next != '\0')
        {
            *next++ = '\0';
        }

        if (!app_parse_mac(token, event->destination_mac))
        {
            ESP_LOGW(TAG, "invalid MAC for text: %s", token);
            return false;
        }

        cursor = app_skip_spaces(next);
        if (cursor == NULL || *cursor == '\0')
        {
            ESP_LOGW(TAG, "text message is empty");
            return false;
        }

        strncpy(event->text, cursor, sizeof(event->text) - 1u);
        event->type = APP_CLI_EVT_TEXT;
        return true;
    }

    if (strncasecmp(cursor, "cmd", 3) == 0 && (cursor[3] == '\0' || isspace((unsigned char)cursor[3])))
    {
        cursor = app_skip_spaces(cursor + 3);
        if (cursor == NULL || *cursor == '\0')
        {
            ESP_LOGW(TAG, "usage: cmd <mac> <comando> [args]");
            return false;
        }

        token = cursor;
        next = token;
        while (*next != '\0' && !isspace((unsigned char)*next))
        {
            next++;
        }
        if (*next != '\0')
        {
            *next++ = '\0';
        }

        if (!app_parse_mac(token, event->destination_mac))
        {
            ESP_LOGW(TAG, "invalid MAC for cmd: %s", token);
            return false;
        }

        cursor = app_skip_spaces(next);
        if (cursor == NULL || *cursor == '\0')
        {
            ESP_LOGW(TAG, "missing command name");
            return false;
        }

        token = cursor;
        next = token;
        while (*next != '\0' && !isspace((unsigned char)*next))
        {
            next++;
        }
        if (*next != '\0')
        {
            *next++ = '\0';
        }

        if (strlen(token) >= sizeof(event->command))
        {
            ESP_LOGW(TAG, "command name too long");
            return false;
        }

        strncpy(event->command, token, sizeof(event->command) - 1u);
        cursor = app_skip_spaces(next);
        if (cursor != NULL && *cursor != '\0')
        {
            strncpy(event->args, cursor, sizeof(event->args) - 1u);
        }

        event->type = APP_CLI_EVT_CMD;
        return true;
    }

    ESP_LOGW(TAG, "unknown command: %s", cursor);
    app_log_cli_help();
    return false;
}

static esp_err_t app_ensure_peer(const uint8_t mac[AODV_EN_MAC_ADDR_LEN], uint8_t channel)
{
    esp_now_peer_info_t peer;

    if (esp_now_is_peer_exist(mac))
    {
        return ESP_OK;
    }

    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, mac, AODV_EN_MAC_ADDR_LEN);
    peer.channel = channel;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    return esp_now_add_peer(&peer);
}

static aodv_en_status_t app_emit_frame(
    void *user_ctx,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    bool broadcast)
{
    app_context_t *app = (app_context_t *)user_ctx;
    const uint8_t *dest_mac = broadcast ? BROADCAST_MAC : next_hop;
    esp_err_t err;
    char mac_text[18];

    if (frame_len > ESP_NOW_MAX_DATA_LEN_V2)
    {
        ESP_LOGE(TAG, "frame too large for ESP-NOW v2: %u", (unsigned int)frame_len);
        return AODV_EN_ERR_SIZE;
    }

    err = app_ensure_peer(dest_mac, app->wifi_channel);
    if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST)
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

static aodv_en_status_t app_proto_send_data(
    void *user_ctx,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len,
    bool ack_required)
{
    app_context_t *app = (app_context_t *)user_ctx;

    return aodv_en_stack_send_data_at(
        &app->stack,
        destination_mac,
        payload,
        payload_len,
        ack_required,
        app_now_ms());
}

static uint32_t app_proto_now_ms(void *user_ctx)
{
    (void)user_ctx;
    return app_now_ms();
}

static void app_proto_on_health_req(
    void *user_ctx,
    const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t request_id)
{
    char mac_text[18];

    (void)user_ctx;
    app_format_mac(source_mac, mac_text, sizeof(mac_text));
    ESP_LOGI(TAG, "HEALTH_REQ from %s request_id=%" PRIu32, mac_text, request_id);
}

static void app_proto_on_health_rsp(
    void *user_ctx,
    const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t request_id,
    const char *status_text)
{
    char mac_text[18];

    (void)user_ctx;
    app_format_mac(source_mac, mac_text, sizeof(mac_text));
    ESP_LOGI(TAG, "HEALTH_RSP from %s request_id=%" PRIu32 " status=\"%s\"",
             mac_text,
             request_id,
             status_text);
}

static void app_proto_on_text(
    void *user_ctx,
    const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t request_id,
    const char *text)
{
    char mac_text[18];

    (void)user_ctx;
    app_format_mac(source_mac, mac_text, sizeof(mac_text));
    ESP_LOGI(TAG, "TEXT from %s request_id=%" PRIu32 " text=\"%s\"",
             mac_text,
             request_id,
             text);
}

static void app_proto_on_cmd_req(
    void *user_ctx,
    const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t request_id,
    const char *command,
    const char *args)
{
    char mac_text[18];

    (void)user_ctx;
    app_format_mac(source_mac, mac_text, sizeof(mac_text));
    ESP_LOGI(TAG, "CMD_REQ from %s request_id=%" PRIu32 " cmd=%s args=\"%s\"",
             mac_text,
             request_id,
             command,
             args);
}

static void app_proto_on_cmd_rsp(
    void *user_ctx,
    const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t request_id,
    const char *command,
    int32_t status_code,
    const char *response_text)
{
    char mac_text[18];

    (void)user_ctx;
    app_format_mac(source_mac, mac_text, sizeof(mac_text));
    ESP_LOGI(TAG, "CMD_RSP from %s request_id=%" PRIu32 " cmd=%s status=%" PRId32 " response=\"%s\"",
             mac_text,
             request_id,
             command,
             status_code,
             response_text);
}

static aodv_en_status_t app_cmd_ping(
    aodv_en_app_proto_t *proto,
    void *user_ctx,
    const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
    const char *args,
    char *response,
    size_t response_size,
    int32_t *status_code)
{
    (void)proto;
    (void)user_ctx;
    (void)source_mac;
    (void)args;

    if (response == NULL || response_size == 0u || status_code == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    *status_code = 0;
    snprintf(response, response_size, "pong");
    return AODV_EN_OK;
}

static aodv_en_status_t app_cmd_echo(
    aodv_en_app_proto_t *proto,
    void *user_ctx,
    const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
    const char *args,
    char *response,
    size_t response_size,
    int32_t *status_code)
{
    (void)proto;
    (void)user_ctx;
    (void)source_mac;

    if (response == NULL || response_size == 0u || status_code == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    *status_code = 0;
    snprintf(response, response_size, "%s", (args != NULL) ? args : "");
    return AODV_EN_OK;
}

static aodv_en_status_t app_cmd_info(
    aodv_en_app_proto_t *proto,
    void *user_ctx,
    const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
    const char *args,
    char *response,
    size_t response_size,
    int32_t *status_code)
{
    app_context_t *app = (app_context_t *)user_ctx;
    aodv_en_overview_t overview;

    (void)proto;
    (void)source_mac;
    (void)args;

    if (app == NULL || response == NULL || response_size == 0u || status_code == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    if (aodv_en_stack_get_overview(&app->stack, &overview) != AODV_EN_OK)
    {
        *status_code = -1;
        snprintf(response, response_size, "overview unavailable");
        return AODV_EN_OK;
    }

    *status_code = 0;
    snprintf(response,
             response_size,
             "node=%s routes=%u neighbors=%u tx=%" PRIu32 " rx=%" PRIu32,
             app->node_name,
             overview.routes_count,
             overview.neighbors_count,
             overview.stats.tx_frames,
             overview.stats.rx_frames);
    return AODV_EN_OK;
}

static void app_stack_on_data(
    void *user_ctx,
    const uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len)
{
    app_context_t *app = (app_context_t *)user_ctx;
    aodv_en_status_t status;

    if (app == NULL || payload == NULL || payload_len == 0u)
    {
        return;
    }

    status = aodv_en_app_proto_on_mesh_payload(
        &app->proto,
        originator_mac,
        payload,
        payload_len);

    if (status != AODV_EN_OK)
    {
        ESP_LOGW(TAG, "app proto parse/handle status=%d", status);
    }
}

static void app_stack_on_ack(
    void *user_ctx,
    const uint8_t ack_sender_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t sequence_number)
{
    char mac_text[18];

    (void)user_ctx;
    app_format_mac(ack_sender_mac, mac_text, sizeof(mac_text));
    ESP_LOGI(TAG, "ACK received from %s for seq=%" PRIu32, mac_text, sequence_number);
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

    ESP_LOGI(TAG, "routes=%u neighbors=%u tx=%" PRIu32 " rx=%" PRIu32 " delivered=%" PRIu32,
             overview.routes_count,
             overview.neighbors_count,
             overview.stats.tx_frames,
             overview.stats.rx_frames,
             overview.stats.delivered_frames);

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

static void app_process_rx_queue(app_context_t *app)
{
    app_rx_event_t event;

    while (xQueueReceive(app->rx_queue, &event, 0) == pdTRUE)
    {
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

static void app_process_cli_queue(app_context_t *app)
{
    app_cli_event_t event;

    while (app->cli_queue != NULL && xQueueReceive(app->cli_queue, &event, 0) == pdTRUE)
    {
        aodv_en_status_t status = AODV_EN_ERR_STATE;
        uint32_t request_id = 0u;
        char mac_text[18];

        switch (event.type)
        {
        case APP_CLI_EVT_HEALTH_BROADCAST:
            status = aodv_en_app_proto_send_health_req(&app->proto, BROADCAST_MAC, true, &request_id);
            if (status == AODV_EN_OK)
            {
                ESP_LOGI(TAG, "CLI health all queued request_id=%" PRIu32, request_id);
            }
            else if (status == AODV_EN_QUEUED)
            {
                ESP_LOGI(TAG, "CLI health all queued while route discovery is in progress");
            }
            else
            {
                ESP_LOGW(TAG, "CLI health all failed status=%d", status);
            }
            break;

        case APP_CLI_EVT_HEALTH_UNICAST:
            status = aodv_en_app_proto_send_health_req(&app->proto, event.destination_mac, false, &request_id);
            app_format_mac(event.destination_mac, mac_text, sizeof(mac_text));
            if (status == AODV_EN_OK)
            {
                ESP_LOGI(TAG, "CLI health %s queued request_id=%" PRIu32, mac_text, request_id);
            }
            else if (status == AODV_EN_QUEUED)
            {
                ESP_LOGI(TAG, "CLI health %s queued while route discovery is in progress", mac_text);
            }
            else
            {
                ESP_LOGW(TAG, "CLI health %s failed status=%d", mac_text, status);
            }
            break;

        case APP_CLI_EVT_TEXT:
            status = aodv_en_app_proto_send_text(
                &app->proto,
                event.destination_mac,
                event.text,
                true,
                &request_id);
            app_format_mac(event.destination_mac, mac_text, sizeof(mac_text));
            if (status == AODV_EN_OK)
            {
                ESP_LOGI(TAG, "CLI text %s queued request_id=%" PRIu32 " text=\"%s\"",
                         mac_text,
                         request_id,
                         event.text);
            }
            else if (status == AODV_EN_QUEUED)
            {
                ESP_LOGI(TAG, "CLI text %s queued while route discovery is in progress", mac_text);
            }
            else
            {
                ESP_LOGW(TAG, "CLI text %s failed status=%d", mac_text, status);
            }
            break;

        case APP_CLI_EVT_CMD:
            status = aodv_en_app_proto_send_command(
                &app->proto,
                event.destination_mac,
                event.command,
                event.args,
                true,
                &request_id);
            app_format_mac(event.destination_mac, mac_text, sizeof(mac_text));
            if (status == AODV_EN_OK)
            {
                ESP_LOGI(TAG, "CLI cmd %s queued request_id=%" PRIu32 " cmd=%s args=\"%s\"",
                         mac_text,
                         request_id,
                         event.command,
                         event.args);
            }
            else if (status == AODV_EN_QUEUED)
            {
                ESP_LOGI(TAG, "CLI cmd %s queued while route discovery is in progress", mac_text);
            }
            else
            {
                ESP_LOGW(TAG, "CLI cmd %s failed status=%d", mac_text, status);
            }
            break;

        case APP_CLI_EVT_PRINT_ROUTES:
            app_log_routes(&app->stack);
            break;

        default:
            ESP_LOGW(TAG, "CLI unknown event type=%d", (int)event.type);
            break;
        }
    }
}

static void app_cli_task(void *arg)
{
    app_context_t *app = (app_context_t *)arg;
    char line[APP_CLI_LINE_MAX];
    size_t line_len = 0u;

    ESP_LOGI(TAG, "CLI enabled. Type 'help' and press Enter.");
    app_log_cli_help();

    for (;;)
    {
        app_cli_event_t event;
        int ch = fgetc(stdin);

        if (ch == EOF)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if (ch == '\r' || ch == '\n')
        {
            if (line_len == 0u)
            {
                continue;
            }

            line[line_len] = '\0';
            line_len = 0u;

            if (!app_cli_parse_line(line, &event))
            {
                continue;
            }

            if (app->cli_queue == NULL ||
                xQueueSend(app->cli_queue, &event, pdMS_TO_TICKS(100)) != pdTRUE)
            {
                ESP_LOGW(TAG, "CLI queue full, dropping command");
            }
            continue;
        }

        if (ch == '\b' || ch == 0x7f)
        {
            if (line_len > 0u)
            {
                line_len--;
            }
            continue;
        }

        if (!isprint((unsigned char)ch))
        {
            continue;
        }

        if (line_len < sizeof(line) - 1u)
        {
            line[line_len++] = (char)ch;
        }
        else
        {
            ESP_LOGW(TAG, "CLI input line too long, clearing buffer");
            line_len = 0u;
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
        app_process_cli_queue(app);
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

        if (now_ms >= app->next_health_at_ms)
        {
            uint32_t request_id = 0u;
            status = aodv_en_app_proto_send_health_req(
                &app->proto,
                BROADCAST_MAC,
                true,
                &request_id);

            if (status == AODV_EN_OK)
            {
                ESP_LOGI(TAG, "HEALTH_REQ broadcast request_id=%" PRIu32, request_id);
            }
            else if (status == AODV_EN_QUEUED)
            {
                ESP_LOGI(TAG, "HEALTH_REQ queued while route discovery is in progress");
            }
            else
            {
                ESP_LOGW(TAG, "HEALTH_REQ send status=%d", status);
            }

            app->next_health_at_ms = now_ms + app->health_interval_ms;
        }

        if (APP_PROTO_ENABLE_UNICAST && app->has_target && now_ms >= app->next_unicast_at_ms)
        {
            uint32_t text_request_id = 0u;
            uint32_t cmd_request_id = 0u;

            status = aodv_en_app_proto_send_text(
                &app->proto,
                app->target_mac,
                app->unicast_text,
                true,
                &text_request_id);

            if (status == AODV_EN_OK)
            {
                ESP_LOGI(TAG, "TEXT queued request_id=%" PRIu32 " text=\"%s\"",
                         text_request_id,
                         app->unicast_text);
            }
            else if (status == AODV_EN_QUEUED)
            {
                ESP_LOGI(TAG, "TEXT queued while route discovery is in progress");
            }
            else
            {
                ESP_LOGW(TAG, "TEXT send status=%d", status);
            }

            status = aodv_en_app_proto_send_command(
                &app->proto,
                app->target_mac,
                app->command_name,
                app->command_args,
                true,
                &cmd_request_id);

            if (status == AODV_EN_OK)
            {
                ESP_LOGI(TAG, "CMD queued request_id=%" PRIu32 " cmd=%s args=\"%s\"",
                         cmd_request_id,
                         app->command_name,
                         app->command_args);
            }
            else if (status == AODV_EN_QUEUED)
            {
                ESP_LOGI(TAG, "CMD queued while route discovery is in progress");
            }
            else
            {
                ESP_LOGW(TAG, "CMD send status=%d", status);
            }

            app->next_unicast_at_ms = now_ms + app->unicast_interval_ms;
        }

        if (now_ms >= app->next_print_at_ms)
        {
            app_log_routes(&app->stack);
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
}

static void app_init_espnow(uint8_t channel)
{
    uint32_t version = 0;

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(app_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(app_recv_cb));
    ESP_ERROR_CHECK(app_ensure_peer(BROADCAST_MAC, channel));
    ESP_ERROR_CHECK(esp_now_get_version(&version));
    ESP_LOGI(TAG, "ESP-NOW version=%" PRIu32, version);
}

void app_proto_example_run(void)
{
    aodv_en_config_t node_config;
    aodv_en_adapter_t adapter;
    aodv_en_app_callbacks_t stack_callbacks;
    aodv_en_app_proto_transport_t proto_transport;
    aodv_en_app_proto_callbacks_t proto_callbacks;
    char self_mac_text[18];
    char target_mac_text[18];

    memset(&g_app, 0, sizeof(g_app));
    g_app.node_name = CONFIG_AODV_EN_APP_NODE_NAME;
    g_app.wifi_channel = CONFIG_AODV_EN_APP_WIFI_CHANNEL;
    g_app.hello_interval_ms = CONFIG_AODV_EN_APP_HELLO_INTERVAL_MS;
    g_app.print_interval_ms = CONFIG_AODV_EN_APP_PRINT_INTERVAL_MS;
    g_app.health_interval_ms = CONFIG_AODV_EN_APP_PROTO_HEALTH_INTERVAL_MS;
    g_app.unicast_interval_ms = CONFIG_AODV_EN_APP_PROTO_UNICAST_INTERVAL_MS;
    g_app.unicast_text = APP_PROTO_TEXT_VALUE;
    g_app.command_name = APP_PROTO_COMMAND_VALUE;
    g_app.command_args = APP_PROTO_COMMAND_ARGS_VALUE;

    app_init_nvs();
    app_init_wifi(g_app.wifi_channel);
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, g_app.self_mac));
    app_init_espnow(g_app.wifi_channel);

    aodv_en_config_set_defaults(&node_config);
    node_config.network_id = CONFIG_AODV_EN_APP_NETWORK_ID;
    node_config.wifi_channel = g_app.wifi_channel;
    node_config.ack_timeout_ms = AODV_EN_ACK_TIMEOUT_MS_DEFAULT;
    node_config.link_fail_threshold = AODV_EN_LINK_FAIL_THRESHOLD;

    memset(&adapter, 0, sizeof(adapter));
    adapter.user_ctx = &g_app;
    adapter.now_ms = app_adapter_now_ms;
    adapter.tx_frame = app_emit_frame;

    memset(&stack_callbacks, 0, sizeof(stack_callbacks));
    stack_callbacks.user_ctx = &g_app;
    stack_callbacks.on_data = app_stack_on_data;
    stack_callbacks.on_ack = app_stack_on_ack;

    ESP_ERROR_CHECK(
        aodv_en_stack_init(&g_app.stack, &node_config, g_app.self_mac, &adapter, &stack_callbacks) == AODV_EN_OK
            ? ESP_OK
            : ESP_FAIL);

    g_app.rx_queue = xQueueCreate(APP_RX_QUEUE_LEN, sizeof(app_rx_event_t));
    assert(g_app.rx_queue != NULL);
    g_app.tx_result_queue = xQueueCreate(APP_TX_RESULT_QUEUE_LEN, sizeof(app_tx_result_event_t));
    assert(g_app.tx_result_queue != NULL);
    g_app.cli_queue = xQueueCreate(APP_CLI_QUEUE_LEN, sizeof(app_cli_event_t));
    assert(g_app.cli_queue != NULL);

    memset(&proto_transport, 0, sizeof(proto_transport));
    proto_transport.user_ctx = &g_app;
    proto_transport.send_data = app_proto_send_data;
    proto_transport.now_ms = app_proto_now_ms;

    memset(&proto_callbacks, 0, sizeof(proto_callbacks));
    proto_callbacks.user_ctx = &g_app;
    proto_callbacks.on_health_req = app_proto_on_health_req;
    proto_callbacks.on_health_rsp = app_proto_on_health_rsp;
    proto_callbacks.on_text = app_proto_on_text;
    proto_callbacks.on_cmd_req = app_proto_on_cmd_req;
    proto_callbacks.on_cmd_rsp = app_proto_on_cmd_rsp;

    aodv_en_app_proto_init(
        &g_app.proto,
        g_app.node_name,
        &proto_transport,
        &proto_callbacks);

    (void)aodv_en_app_proto_register_command(&g_app.proto, "ping", app_cmd_ping, &g_app);
    (void)aodv_en_app_proto_register_command(&g_app.proto, "echo", app_cmd_echo, &g_app);
    (void)aodv_en_app_proto_register_command(&g_app.proto, "info", app_cmd_info, &g_app);

    g_app.has_target = app_parse_mac(APP_PROTO_TARGET_MAC_TEXT, g_app.target_mac);
    if (g_app.has_target && memcmp(g_app.target_mac, g_app.self_mac, AODV_EN_MAC_ADDR_LEN) == 0)
    {
        ESP_LOGW(TAG, "unicast target MAC equals self MAC; disabling unicast sends");
        g_app.has_target = false;
    }

    g_app.next_hello_at_ms = app_now_ms() + 1000u;
    g_app.next_print_at_ms = app_now_ms() + g_app.print_interval_ms;
    g_app.next_health_at_ms = app_now_ms() + 3000u;
    g_app.next_unicast_at_ms = app_now_ms() + 6000u;

    app_format_mac(g_app.self_mac, self_mac_text, sizeof(self_mac_text));
    ESP_LOGI(TAG, "node=%s self_mac=%s channel=%u network_id=0x%08" PRIX32,
             g_app.node_name,
             self_mac_text,
             g_app.wifi_channel,
             node_config.network_id);
    ESP_LOGI(TAG, "app proto example enabled: health_interval=%" PRIu32 "ms",
             g_app.health_interval_ms);

    if (APP_PROTO_ENABLE_UNICAST && g_app.has_target)
    {
        app_format_mac(g_app.target_mac, target_mac_text, sizeof(target_mac_text));
        ESP_LOGI(TAG, "unicast enabled target=%s text=\"%s\" cmd=%s args=\"%s\" interval=%" PRIu32 "ms",
                 target_mac_text,
                 g_app.unicast_text,
                 g_app.command_name,
                 g_app.command_args,
                 g_app.unicast_interval_ms);
    }
    else if (APP_PROTO_ENABLE_UNICAST)
    {
        ESP_LOGW(TAG, "unicast enabled but target MAC invalid or empty");
    }

    xTaskCreate(app_protocol_task, "aodv_en_proto_task", 9216, &g_app, 5, NULL);

    xTaskCreate(app_cli_task, "aodv_en_cli_task", 4096, &g_app, 4, NULL);
}
