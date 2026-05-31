#include <string.h>

#include "flood_en.h"

#include "aodv_en_messages.h"

static const uint8_t FLOOD_EN_BROADCAST_MAC[AODV_EN_MAC_ADDR_LEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static bool flood_en_mac_equal(
    const uint8_t a[AODV_EN_MAC_ADDR_LEN],
    const uint8_t b[AODV_EN_MAC_ADDR_LEN])
{
    return memcmp(a, b, AODV_EN_MAC_ADDR_LEN) == 0;
}

static bool flood_en_seen_contains(
    const flood_en_seen_entry_t *table,
    const uint8_t origin[AODV_EN_MAC_ADDR_LEN],
    uint32_t seq)
{
    for (size_t index = 0; index < FLOOD_EN_SEEN_SIZE; index++)
    {
        if (table[index].used &&
            table[index].seq == seq &&
            flood_en_mac_equal(table[index].origin, origin))
        {
            return true;
        }
    }

    return false;
}

static void flood_en_seen_remember(
    flood_en_seen_entry_t *table,
    const uint8_t origin[AODV_EN_MAC_ADDR_LEN],
    uint32_t seq,
    uint32_t now_ms)
{
    size_t free_index = FLOOD_EN_SEEN_SIZE;
    size_t oldest_index = 0;
    uint32_t oldest_at = UINT32_MAX;

    for (size_t index = 0; index < FLOOD_EN_SEEN_SIZE; index++)
    {
        if (!table[index].used)
        {
            free_index = index;
            break;
        }

        if (table[index].inserted_at_ms <= oldest_at)
        {
            oldest_at = table[index].inserted_at_ms;
            oldest_index = index;
        }
    }

    size_t target = (free_index < FLOOD_EN_SEEN_SIZE) ? free_index : oldest_index;

    memcpy(table[target].origin, origin, AODV_EN_MAC_ADDR_LEN);
    table[target].seq = seq;
    table[target].inserted_at_ms = now_ms;
    table[target].used = true;
}

static aodv_en_status_t flood_en_emit(
    flood_en_node_t *node,
    const uint8_t *frame,
    size_t frame_len)
{
    if (node->callbacks.emit_frame == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    node->stats.tx_frames++;
    return node->callbacks.emit_frame(
        node->callbacks.user_ctx,
        FLOOD_EN_BROADCAST_MAC,
        frame,
        frame_len,
        true);
}

static void flood_en_send_ack(
    flood_en_node_t *node,
    const uint8_t data_originator[AODV_EN_MAC_ADDR_LEN],
    uint32_t sequence_number,
    uint32_t now_ms)
{
    aodv_en_ack_msg_t ack;

    memset(&ack, 0, sizeof(ack));
    ack.header.protocol_version = AODV_EN_PROTOCOL_VERSION;
    ack.header.message_type = AODV_EN_MSG_ACK;
    ack.header.flags = AODV_EN_MSG_FLAG_NONE;
    ack.header.hop_count = 0;
    ack.header.network_id = node->config.network_id;
    memcpy(ack.header.sender_mac, node->self_mac, AODV_EN_MAC_ADDR_LEN);
    memcpy(ack.originator_mac, node->self_mac, AODV_EN_MAC_ADDR_LEN);
    memcpy(ack.destination_mac, data_originator, AODV_EN_MAC_ADDR_LEN);
    ack.ack_for_sequence = sequence_number;

    flood_en_seen_remember(node->seen_ack, node->self_mac, sequence_number, now_ms);
    (void)flood_en_emit(node, (const uint8_t *)&ack, sizeof(ack));
}

aodv_en_status_t flood_en_node_init(
    flood_en_node_t *node,
    const aodv_en_config_t *config,
    const uint8_t self_mac[AODV_EN_MAC_ADDR_LEN])
{
    if (node == NULL || config == NULL || self_mac == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    memset(node, 0, sizeof(*node));
    node->config = *config;
    memcpy(node->self_mac, self_mac, AODV_EN_MAC_ADDR_LEN);
    node->next_seq = 0;

    return AODV_EN_OK;
}

void flood_en_node_set_callbacks(
    flood_en_node_t *node,
    const aodv_en_node_callbacks_t *callbacks)
{
    if (node == NULL || callbacks == NULL)
    {
        return;
    }

    node->callbacks = *callbacks;
}

void flood_en_node_tick(
    flood_en_node_t *node,
    uint32_t now_ms)
{
    (void)node;
    (void)now_ms;
}

aodv_en_status_t flood_en_node_send_data(
    flood_en_node_t *node,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len,
    bool ack_required,
    uint32_t now_ms)
{
    uint8_t buffer[sizeof(aodv_en_data_msg_t) + AODV_EN_DATA_PAYLOAD_MAX];
    aodv_en_data_msg_t *msg = (aodv_en_data_msg_t *)buffer;
    uint32_t seq;

    if (node == NULL || destination_mac == NULL ||
        (payload == NULL && payload_len > 0))
    {
        return AODV_EN_ERR_ARG;
    }

    if (payload_len > node->config.data_payload_max ||
        payload_len > AODV_EN_DATA_PAYLOAD_MAX)
    {
        return AODV_EN_ERR_SIZE;
    }

    seq = ++node->next_seq;
    flood_en_seen_remember(node->seen_data, node->self_mac, seq, now_ms);

    memset(msg, 0, sizeof(*msg));
    msg->header.protocol_version = AODV_EN_PROTOCOL_VERSION;
    msg->header.message_type = AODV_EN_MSG_DATA;
    msg->header.flags = ack_required ? AODV_EN_MSG_FLAG_ACK_REQUIRED : AODV_EN_MSG_FLAG_NONE;
    msg->header.hop_count = 0;
    msg->header.network_id = node->config.network_id;
    memcpy(msg->header.sender_mac, node->self_mac, AODV_EN_MAC_ADDR_LEN);
    memcpy(msg->originator_mac, node->self_mac, AODV_EN_MAC_ADDR_LEN);
    memcpy(msg->destination_mac, destination_mac, AODV_EN_MAC_ADDR_LEN);
    msg->sequence_number = seq;
    msg->ttl = node->config.ttl_default;
    msg->payload_length = payload_len;
    if (payload_len > 0)
    {
        memcpy(msg->payload, payload, payload_len);
    }

    return flood_en_emit(node, buffer, sizeof(aodv_en_data_msg_t) + payload_len);
}

static aodv_en_status_t flood_en_handle_data(
    flood_en_node_t *node,
    const uint8_t *frame,
    size_t frame_len,
    uint32_t now_ms)
{
    const aodv_en_data_msg_t *msg = (const aodv_en_data_msg_t *)frame;
    uint16_t payload_len;

    if (frame_len < sizeof(aodv_en_data_msg_t))
    {
        return AODV_EN_ERR_SIZE;
    }

    payload_len = msg->payload_length;
    if (frame_len < sizeof(aodv_en_data_msg_t) + payload_len)
    {
        return AODV_EN_ERR_SIZE;
    }

    if (flood_en_seen_contains(node->seen_data, msg->originator_mac, msg->sequence_number))
    {
        node->stats.duplicate_drops++;
        return AODV_EN_OK;
    }

    flood_en_seen_remember(node->seen_data, msg->originator_mac, msg->sequence_number, now_ms);

    if (flood_en_mac_equal(msg->destination_mac, node->self_mac))
    {
        node->stats.delivered_frames++;
        if (node->callbacks.deliver_data != NULL)
        {
            node->callbacks.deliver_data(
                node->callbacks.user_ctx,
                msg->originator_mac,
                msg->payload,
                payload_len);
        }

        if ((msg->header.flags & AODV_EN_MSG_FLAG_ACK_REQUIRED) != 0)
        {
            flood_en_send_ack(node, msg->originator_mac, msg->sequence_number, now_ms);
        }

        return AODV_EN_OK;
    }

    if (msg->ttl <= 1u)
    {
        node->stats.ttl_drops++;
        return AODV_EN_OK;
    }

    {
        uint8_t buffer[sizeof(aodv_en_data_msg_t) + AODV_EN_DATA_PAYLOAD_MAX];
        aodv_en_data_msg_t *fwd = (aodv_en_data_msg_t *)buffer;
        size_t fwd_len = sizeof(aodv_en_data_msg_t) + payload_len;

        memcpy(buffer, frame, fwd_len);
        fwd->header.hop_count++;
        memcpy(fwd->header.sender_mac, node->self_mac, AODV_EN_MAC_ADDR_LEN);
        fwd->ttl = (uint8_t)(msg->ttl - 1u);

        node->stats.rebroadcast_frames++;
        return flood_en_emit(node, buffer, fwd_len);
    }
}

static aodv_en_status_t flood_en_handle_ack(
    flood_en_node_t *node,
    const uint8_t *frame,
    size_t frame_len,
    uint32_t now_ms)
{
    const aodv_en_ack_msg_t *msg = (const aodv_en_ack_msg_t *)frame;

    if (frame_len < sizeof(aodv_en_ack_msg_t))
    {
        return AODV_EN_ERR_SIZE;
    }

    if (flood_en_seen_contains(node->seen_ack, msg->originator_mac, msg->ack_for_sequence))
    {
        node->stats.duplicate_drops++;
        return AODV_EN_OK;
    }

    flood_en_seen_remember(node->seen_ack, msg->originator_mac, msg->ack_for_sequence, now_ms);

    if (flood_en_mac_equal(msg->destination_mac, node->self_mac))
    {
        node->stats.ack_received++;
        if (node->callbacks.ack_received != NULL)
        {
            node->callbacks.ack_received(
                node->callbacks.user_ctx,
                msg->originator_mac,
                msg->ack_for_sequence);
        }

        return AODV_EN_OK;
    }

    if ((uint32_t)(msg->header.hop_count + 1u) >= node->config.max_hops)
    {
        node->stats.ttl_drops++;
        return AODV_EN_OK;
    }

    {
        aodv_en_ack_msg_t fwd = *msg;

        fwd.header.hop_count++;
        memcpy(fwd.header.sender_mac, node->self_mac, AODV_EN_MAC_ADDR_LEN);

        node->stats.rebroadcast_frames++;
        return flood_en_emit(node, (const uint8_t *)&fwd, sizeof(fwd));
    }
}

aodv_en_status_t flood_en_node_on_recv(
    flood_en_node_t *node,
    const uint8_t link_src_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    int8_t rssi,
    uint32_t now_ms)
{
    const aodv_en_header_t *header = (const aodv_en_header_t *)frame;

    (void)link_src_mac;
    (void)rssi;

    if (node == NULL || frame == NULL || frame_len < sizeof(aodv_en_header_t))
    {
        return AODV_EN_ERR_SIZE;
    }

    if (header->protocol_version != AODV_EN_PROTOCOL_VERSION ||
        header->network_id != node->config.network_id)
    {
        node->stats.foreign_drops++;
        return AODV_EN_OK;
    }

    node->stats.rx_frames++;

    switch ((aodv_en_message_type_t)header->message_type)
    {
    case AODV_EN_MSG_DATA:
        return flood_en_handle_data(node, frame, frame_len, now_ms);
    case AODV_EN_MSG_ACK:
        return flood_en_handle_ack(node, frame, frame_len, now_ms);
    default:
        return AODV_EN_OK;
    }
}
