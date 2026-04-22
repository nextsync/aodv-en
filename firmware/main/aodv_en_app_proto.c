#include "aodv_en_app_proto.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define AODV_EN_APP_PROTO_HEADER_LEN 8u
#define AODV_EN_APP_PROTO_TEXT_MAX 120u

typedef struct
{
    uint8_t type;
    uint16_t body_len;
    uint32_t request_id;
    const uint8_t *body;
} aodv_en_app_proto_decoded_t;

static uint16_t aodv_en_app_proto_read_u16_le(const uint8_t *buffer)
{
    return (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
}

static uint32_t aodv_en_app_proto_read_u32_le(const uint8_t *buffer)
{
    return (uint32_t)buffer[0] |
           ((uint32_t)buffer[1] << 8) |
           ((uint32_t)buffer[2] << 16) |
           ((uint32_t)buffer[3] << 24);
}

static void aodv_en_app_proto_write_u16_le(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFu);
    buffer[1] = (uint8_t)((value >> 8) & 0xFFu);
}

static void aodv_en_app_proto_write_u32_le(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFu);
    buffer[1] = (uint8_t)((value >> 8) & 0xFFu);
    buffer[2] = (uint8_t)((value >> 16) & 0xFFu);
    buffer[3] = (uint8_t)((value >> 24) & 0xFFu);
}

static uint32_t aodv_en_app_proto_next_request_id(aodv_en_app_proto_t *proto)
{
    uint32_t request_id;

    request_id = proto->next_request_id;
    proto->next_request_id++;
    if (proto->next_request_id == 0u)
    {
        proto->next_request_id = 1u;
    }

    return request_id;
}

static uint32_t aodv_en_app_proto_now_ms(const aodv_en_app_proto_t *proto)
{
    if (proto == NULL || proto->transport.now_ms == NULL)
    {
        return 0u;
    }

    return proto->transport.now_ms(proto->transport.user_ctx);
}

static aodv_en_status_t aodv_en_app_proto_send_frame(
    aodv_en_app_proto_t *proto,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    uint8_t msg_type,
    uint32_t request_id,
    const uint8_t *body,
    uint16_t body_len,
    bool ack_required)
{
    uint8_t frame[AODV_EN_DATA_PAYLOAD_MAX];
    size_t frame_len;

    if (proto == NULL || destination_mac == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    if (proto->transport.send_data == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    if ((size_t)body_len + AODV_EN_APP_PROTO_HEADER_LEN > sizeof(frame))
    {
        return AODV_EN_ERR_SIZE;
    }

    frame[0] = AODV_EN_APP_PROTO_VERSION;
    frame[1] = msg_type;
    aodv_en_app_proto_write_u16_le(&frame[2], body_len);
    aodv_en_app_proto_write_u32_le(&frame[4], request_id);

    if (body_len > 0u && body != NULL)
    {
        memcpy(&frame[AODV_EN_APP_PROTO_HEADER_LEN], body, body_len);
    }

    frame_len = AODV_EN_APP_PROTO_HEADER_LEN + (size_t)body_len;
    return proto->transport.send_data(
        proto->transport.user_ctx,
        destination_mac,
        frame,
        (uint16_t)frame_len,
        ack_required);
}

static aodv_en_status_t aodv_en_app_proto_decode(
    const uint8_t *payload,
    uint16_t payload_len,
    aodv_en_app_proto_decoded_t *decoded)
{
    uint16_t body_len;

    if (payload == NULL || decoded == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    if (payload_len < AODV_EN_APP_PROTO_HEADER_LEN)
    {
        return AODV_EN_ERR_PARSE;
    }

    if (payload[0] != AODV_EN_APP_PROTO_VERSION)
    {
        return AODV_EN_ERR_PARSE;
    }

    body_len = aodv_en_app_proto_read_u16_le(&payload[2]);
    if ((uint16_t)(body_len + AODV_EN_APP_PROTO_HEADER_LEN) != payload_len)
    {
        return AODV_EN_ERR_PARSE;
    }

    memset(decoded, 0, sizeof(*decoded));
    decoded->type = payload[1];
    decoded->body_len = body_len;
    decoded->request_id = aodv_en_app_proto_read_u32_le(&payload[4]);
    decoded->body = &payload[AODV_EN_APP_PROTO_HEADER_LEN];
    return AODV_EN_OK;
}

static void aodv_en_app_proto_copy_text(
    const uint8_t *raw,
    uint16_t raw_len,
    char *text_out,
    size_t text_out_size)
{
    size_t copy_len;

    if (text_out == NULL || text_out_size == 0u)
    {
        return;
    }

    if (raw == NULL || raw_len == 0u)
    {
        text_out[0] = '\0';
        return;
    }

    copy_len = raw_len;
    if (copy_len >= text_out_size)
    {
        copy_len = text_out_size - 1u;
    }

    memcpy(text_out, raw, copy_len);
    text_out[copy_len] = '\0';
}

static aodv_en_app_proto_command_entry_t *aodv_en_app_proto_find_command(
    aodv_en_app_proto_t *proto,
    const char *command_name)
{
    size_t index;

    if (proto == NULL || command_name == NULL || command_name[0] == '\0')
    {
        return NULL;
    }

    for (index = 0u; index < AODV_EN_APP_PROTO_MAX_COMMANDS; index++)
    {
        if (!proto->commands[index].used)
        {
            continue;
        }

        if (strncmp(proto->commands[index].name, command_name, AODV_EN_APP_PROTO_COMMAND_NAME_MAX) == 0)
        {
            return &proto->commands[index];
        }
    }

    return NULL;
}

static aodv_en_status_t aodv_en_app_proto_send_health_rsp(
    aodv_en_app_proto_t *proto,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t request_id)
{
    char text[AODV_EN_APP_PROTO_TEXT_MAX];
    uint32_t now_ms;
    int written;

    now_ms = aodv_en_app_proto_now_ms(proto);
    written = snprintf(
        text,
        sizeof(text),
        "node=%s uptime_ms=%" PRIu32,
        proto->node_name,
        now_ms);

    if (written < 0)
    {
        return AODV_EN_ERR_STATE;
    }

    return aodv_en_app_proto_send_frame(
        proto,
        destination_mac,
        AODV_EN_APP_PROTO_MSG_HEALTH_RSP,
        request_id,
        (const uint8_t *)text,
        (uint16_t)strlen(text),
        true);
}

static aodv_en_status_t aodv_en_app_proto_handle_cmd_req(
    aodv_en_app_proto_t *proto,
    const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
    const aodv_en_app_proto_decoded_t *decoded)
{
    char command[AODV_EN_APP_PROTO_COMMAND_NAME_MAX];
    char args[AODV_EN_APP_PROTO_TEXT_MAX];
    char response[AODV_EN_APP_PROTO_RESPONSE_TEXT_MAX];
    uint8_t body[AODV_EN_DATA_PAYLOAD_MAX];
    aodv_en_app_proto_command_entry_t *entry;
    aodv_en_status_t handler_status;
    uint8_t cmd_len;
    uint8_t args_len;
    size_t response_len;
    size_t body_len;
    int32_t status_code;

    if (decoded->body_len < 2u)
    {
        return AODV_EN_ERR_PARSE;
    }

    cmd_len = decoded->body[0];
    if (cmd_len == 0u || (size_t)(1u + cmd_len + 1u) > decoded->body_len)
    {
        return AODV_EN_ERR_PARSE;
    }

    args_len = decoded->body[1u + cmd_len];
    if ((size_t)(1u + cmd_len + 1u + args_len) != decoded->body_len)
    {
        return AODV_EN_ERR_PARSE;
    }

    aodv_en_app_proto_copy_text(&decoded->body[1], cmd_len, command, sizeof(command));
    aodv_en_app_proto_copy_text(&decoded->body[2u + cmd_len], args_len, args, sizeof(args));

    if (proto->callbacks.on_cmd_req != NULL)
    {
        proto->callbacks.on_cmd_req(
            proto->callbacks.user_ctx,
            source_mac,
            decoded->request_id,
            command,
            args);
    }

    status_code = -404;
    snprintf(response, sizeof(response), "unknown command: %s", command);
    entry = aodv_en_app_proto_find_command(proto, command);
    if (entry != NULL && entry->handler != NULL)
    {
        handler_status = entry->handler(
            proto,
            entry->handler_user_ctx,
            source_mac,
            args,
            response,
            sizeof(response),
            &status_code);
        if (handler_status != AODV_EN_OK && status_code == 0)
        {
            status_code = -500;
        }
    }

    response_len = strlen(response);
    if (response_len > 255u)
    {
        response_len = 255u;
        response[response_len] = '\0';
    }

    body_len = 0u;
    body[body_len++] = (uint8_t)strlen(command);
    memcpy(&body[body_len], command, (size_t)body[0]);
    body_len += (size_t)body[0];
    aodv_en_app_proto_write_u32_le(&body[body_len], (uint32_t)status_code);
    body_len += 4u;
    body[body_len++] = (uint8_t)response_len;
    memcpy(&body[body_len], response, response_len);
    body_len += response_len;

    return aodv_en_app_proto_send_frame(
        proto,
        source_mac,
        AODV_EN_APP_PROTO_MSG_CMD_RSP,
        decoded->request_id,
        body,
        (uint16_t)body_len,
        true);
}

void aodv_en_app_proto_init(
    aodv_en_app_proto_t *proto,
    const char *node_name,
    const aodv_en_app_proto_transport_t *transport,
    const aodv_en_app_proto_callbacks_t *callbacks)
{
    if (proto == NULL)
    {
        return;
    }

    memset(proto, 0, sizeof(*proto));
    proto->next_request_id = 1u;

    if (node_name != NULL && node_name[0] != '\0')
    {
        strncpy(proto->node_name, node_name, sizeof(proto->node_name) - 1u);
    }
    else
    {
        strncpy(proto->node_name, "NODE", sizeof(proto->node_name) - 1u);
    }

    if (transport != NULL)
    {
        proto->transport = *transport;
    }

    if (callbacks != NULL)
    {
        proto->callbacks = *callbacks;
    }
}

void aodv_en_app_proto_set_callbacks(
    aodv_en_app_proto_t *proto,
    const aodv_en_app_proto_callbacks_t *callbacks)
{
    if (proto == NULL)
    {
        return;
    }

    memset(&proto->callbacks, 0, sizeof(proto->callbacks));
    if (callbacks != NULL)
    {
        proto->callbacks = *callbacks;
    }
}

aodv_en_status_t aodv_en_app_proto_register_command(
    aodv_en_app_proto_t *proto,
    const char *command_name,
    aodv_en_app_proto_command_handler_fn handler,
    void *handler_user_ctx)
{
    size_t index;
    size_t command_len;

    if (proto == NULL || command_name == NULL || handler == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    command_len = strlen(command_name);
    if (command_len == 0u)
    {
        return AODV_EN_ERR_ARG;
    }

    if (command_len >= AODV_EN_APP_PROTO_COMMAND_NAME_MAX)
    {
        return AODV_EN_ERR_SIZE;
    }

    if (aodv_en_app_proto_find_command(proto, command_name) != NULL)
    {
        return AODV_EN_ERR_EXISTS;
    }

    for (index = 0u; index < AODV_EN_APP_PROTO_MAX_COMMANDS; index++)
    {
        if (proto->commands[index].used)
        {
            continue;
        }

        memset(&proto->commands[index], 0, sizeof(proto->commands[index]));
        proto->commands[index].used = true;
        proto->commands[index].handler = handler;
        proto->commands[index].handler_user_ctx = handler_user_ctx;
        strncpy(
            proto->commands[index].name,
            command_name,
            sizeof(proto->commands[index].name) - 1u);
        return AODV_EN_OK;
    }

    return AODV_EN_ERR_FULL;
}

aodv_en_status_t aodv_en_app_proto_send_health_req(
    aodv_en_app_proto_t *proto,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    bool is_broadcast,
    uint32_t *request_id_out)
{
    uint32_t request_id;
    aodv_en_status_t status;

    if (proto == NULL || destination_mac == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    request_id = aodv_en_app_proto_next_request_id(proto);
    status = aodv_en_app_proto_send_frame(
        proto,
        destination_mac,
        AODV_EN_APP_PROTO_MSG_HEALTH_REQ,
        request_id,
        NULL,
        0u,
        !is_broadcast);

    if (request_id_out != NULL)
    {
        *request_id_out = request_id;
    }

    return status;
}

aodv_en_status_t aodv_en_app_proto_send_text(
    aodv_en_app_proto_t *proto,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    const char *text,
    bool ack_required,
    uint32_t *request_id_out)
{
    uint32_t request_id;
    size_t text_len;

    if (proto == NULL || destination_mac == NULL || text == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    text_len = strlen(text);
    if (text_len == 0u)
    {
        return AODV_EN_ERR_ARG;
    }

    request_id = aodv_en_app_proto_next_request_id(proto);
    if (request_id_out != NULL)
    {
        *request_id_out = request_id;
    }

    return aodv_en_app_proto_send_frame(
        proto,
        destination_mac,
        AODV_EN_APP_PROTO_MSG_TEXT,
        request_id,
        (const uint8_t *)text,
        (uint16_t)text_len,
        ack_required);
}

aodv_en_status_t aodv_en_app_proto_send_command(
    aodv_en_app_proto_t *proto,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    const char *command_name,
    const char *args,
    bool ack_required,
    uint32_t *request_id_out)
{
    uint8_t body[AODV_EN_DATA_PAYLOAD_MAX];
    uint32_t request_id;
    size_t command_len;
    size_t args_len;
    size_t body_len;

    if (proto == NULL || destination_mac == NULL || command_name == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    if (args == NULL)
    {
        args = "";
    }

    command_len = strlen(command_name);
    args_len = strlen(args);
    if (command_len == 0u)
    {
        return AODV_EN_ERR_ARG;
    }

    if (command_len > 255u || args_len > 255u)
    {
        return AODV_EN_ERR_SIZE;
    }

    body_len = 1u + command_len + 1u + args_len;
    if (body_len + AODV_EN_APP_PROTO_HEADER_LEN > sizeof(body))
    {
        return AODV_EN_ERR_SIZE;
    }

    body[0] = (uint8_t)command_len;
    memcpy(&body[1], command_name, command_len);
    body[1u + command_len] = (uint8_t)args_len;
    memcpy(&body[2u + command_len], args, args_len);

    request_id = aodv_en_app_proto_next_request_id(proto);
    if (request_id_out != NULL)
    {
        *request_id_out = request_id;
    }

    return aodv_en_app_proto_send_frame(
        proto,
        destination_mac,
        AODV_EN_APP_PROTO_MSG_CMD_REQ,
        request_id,
        body,
        (uint16_t)body_len,
        ack_required);
}

aodv_en_status_t aodv_en_app_proto_on_mesh_payload(
    aodv_en_app_proto_t *proto,
    const uint8_t source_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len)
{
    aodv_en_app_proto_decoded_t decoded;
    char text[AODV_EN_APP_PROTO_TEXT_MAX];
    char command[AODV_EN_APP_PROTO_COMMAND_NAME_MAX];
    uint8_t cmd_len;
    uint8_t msg_len;
    int32_t status_code;
    aodv_en_status_t status;

    if (proto == NULL || source_mac == NULL || payload == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    status = aodv_en_app_proto_decode(payload, payload_len, &decoded);
    if (status != AODV_EN_OK)
    {
        return status;
    }

    switch (decoded.type)
    {
    case AODV_EN_APP_PROTO_MSG_HEALTH_REQ:
        if (proto->callbacks.on_health_req != NULL)
        {
            proto->callbacks.on_health_req(
                proto->callbacks.user_ctx,
                source_mac,
                decoded.request_id);
        }
        return aodv_en_app_proto_send_health_rsp(proto, source_mac, decoded.request_id);

    case AODV_EN_APP_PROTO_MSG_HEALTH_RSP:
        aodv_en_app_proto_copy_text(decoded.body, decoded.body_len, text, sizeof(text));
        if (proto->callbacks.on_health_rsp != NULL)
        {
            proto->callbacks.on_health_rsp(
                proto->callbacks.user_ctx,
                source_mac,
                decoded.request_id,
                text);
        }
        return AODV_EN_OK;

    case AODV_EN_APP_PROTO_MSG_TEXT:
        aodv_en_app_proto_copy_text(decoded.body, decoded.body_len, text, sizeof(text));
        if (proto->callbacks.on_text != NULL)
        {
            proto->callbacks.on_text(
                proto->callbacks.user_ctx,
                source_mac,
                decoded.request_id,
                text);
        }
        return AODV_EN_OK;

    case AODV_EN_APP_PROTO_MSG_CMD_REQ:
        return aodv_en_app_proto_handle_cmd_req(proto, source_mac, &decoded);

    case AODV_EN_APP_PROTO_MSG_CMD_RSP:
        if (decoded.body_len < (uint16_t)(1u + 4u + 1u))
        {
            return AODV_EN_ERR_PARSE;
        }

        cmd_len = decoded.body[0];
        if ((size_t)(1u + cmd_len + 4u + 1u) > decoded.body_len || cmd_len == 0u)
        {
            return AODV_EN_ERR_PARSE;
        }

        msg_len = decoded.body[1u + cmd_len + 4u];
        if ((size_t)(1u + cmd_len + 4u + 1u + msg_len) != decoded.body_len)
        {
            return AODV_EN_ERR_PARSE;
        }

        aodv_en_app_proto_copy_text(&decoded.body[1], cmd_len, command, sizeof(command));
        status_code = (int32_t)aodv_en_app_proto_read_u32_le(&decoded.body[1u + cmd_len]);
        aodv_en_app_proto_copy_text(
            &decoded.body[1u + cmd_len + 4u + 1u],
            msg_len,
            text,
            sizeof(text));

        if (proto->callbacks.on_cmd_rsp != NULL)
        {
            proto->callbacks.on_cmd_rsp(
                proto->callbacks.user_ctx,
                source_mac,
                decoded.request_id,
                command,
                status_code,
                text);
        }
        return AODV_EN_OK;

    default:
        return AODV_EN_ERR_PARSE;
    }
}
