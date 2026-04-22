#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "aodv_en.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AODV_EN_APP_PROTO_VERSION 1u
#define AODV_EN_APP_PROTO_MAX_COMMANDS 8u
#define AODV_EN_APP_PROTO_NODE_NAME_MAX 24u
#define AODV_EN_APP_PROTO_COMMAND_NAME_MAX 16u
#define AODV_EN_APP_PROTO_RESPONSE_TEXT_MAX 96u

    typedef enum
    {
        AODV_EN_APP_PROTO_MSG_HEALTH_REQ = 1,
        AODV_EN_APP_PROTO_MSG_HEALTH_RSP = 2,
        AODV_EN_APP_PROTO_MSG_TEXT = 3,
        AODV_EN_APP_PROTO_MSG_CMD_REQ = 4,
        AODV_EN_APP_PROTO_MSG_CMD_RSP = 5,
    } aodv_en_app_proto_msg_type_t;

    typedef struct aodv_en_app_proto aodv_en_app_proto_t;

    typedef aodv_en_status_t (*aodv_en_app_proto_send_fn)(
        void *user_ctx,
        const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *payload,
        uint16_t payload_len,
        bool ack_required);

    typedef uint32_t (*aodv_en_app_proto_now_ms_fn)(
        void *user_ctx);

    typedef struct
    {
        void *user_ctx;
        aodv_en_app_proto_send_fn send_data;
        aodv_en_app_proto_now_ms_fn now_ms;
    } aodv_en_app_proto_transport_t;

    typedef aodv_en_status_t (*aodv_en_app_proto_command_handler_fn)(
        aodv_en_app_proto_t *proto,
        void *user_ctx,
        const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
        const char *args,
        char *response,
        size_t response_size,
        int32_t *status_code);

    typedef void (*aodv_en_app_proto_on_health_req_fn)(
        void *user_ctx,
        const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
        uint32_t request_id);

    typedef void (*aodv_en_app_proto_on_health_rsp_fn)(
        void *user_ctx,
        const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
        uint32_t request_id,
        const char *status_text);

    typedef void (*aodv_en_app_proto_on_text_fn)(
        void *user_ctx,
        const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
        uint32_t request_id,
        const char *text);

    typedef void (*aodv_en_app_proto_on_cmd_req_fn)(
        void *user_ctx,
        const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
        uint32_t request_id,
        const char *command,
        const char *args);

    typedef void (*aodv_en_app_proto_on_cmd_rsp_fn)(
        void *user_ctx,
        const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
        uint32_t request_id,
        const char *command,
        int32_t status_code,
        const char *response_text);

    typedef struct
    {
        void *user_ctx;
        aodv_en_app_proto_on_health_req_fn on_health_req;
        aodv_en_app_proto_on_health_rsp_fn on_health_rsp;
        aodv_en_app_proto_on_text_fn on_text;
        aodv_en_app_proto_on_cmd_req_fn on_cmd_req;
        aodv_en_app_proto_on_cmd_rsp_fn on_cmd_rsp;
    } aodv_en_app_proto_callbacks_t;

    typedef struct
    {
        bool used;
        char name[AODV_EN_APP_PROTO_COMMAND_NAME_MAX];
        aodv_en_app_proto_command_handler_fn handler;
        void *handler_user_ctx;
    } aodv_en_app_proto_command_entry_t;

    struct aodv_en_app_proto
    {
        char node_name[AODV_EN_APP_PROTO_NODE_NAME_MAX];
        uint32_t next_request_id;
        aodv_en_app_proto_transport_t transport;
        aodv_en_app_proto_callbacks_t callbacks;
        aodv_en_app_proto_command_entry_t commands[AODV_EN_APP_PROTO_MAX_COMMANDS];
    };

    void aodv_en_app_proto_init(
        aodv_en_app_proto_t *proto,
        const char *node_name,
        const aodv_en_app_proto_transport_t *transport,
        const aodv_en_app_proto_callbacks_t *callbacks);

    void aodv_en_app_proto_set_callbacks(
        aodv_en_app_proto_t *proto,
        const aodv_en_app_proto_callbacks_t *callbacks);

    aodv_en_status_t aodv_en_app_proto_register_command(
        aodv_en_app_proto_t *proto,
        const char *command_name,
        aodv_en_app_proto_command_handler_fn handler,
        void *handler_user_ctx);

    aodv_en_status_t aodv_en_app_proto_send_health_req(
        aodv_en_app_proto_t *proto,
        const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
        bool is_broadcast,
        uint32_t *request_id_out);

    aodv_en_status_t aodv_en_app_proto_send_text(
        aodv_en_app_proto_t *proto,
        const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
        const char *text,
        bool ack_required,
        uint32_t *request_id_out);

    aodv_en_status_t aodv_en_app_proto_send_command(
        aodv_en_app_proto_t *proto,
        const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
        const char *command_name,
        const char *args,
        bool ack_required,
        uint32_t *request_id_out);

    aodv_en_status_t aodv_en_app_proto_on_mesh_payload(
        aodv_en_app_proto_t *proto,
        const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *payload,
        uint16_t payload_len);

#ifdef __cplusplus
}
#endif
