#include "aodv_en_node.h"

#include <string.h>

#include "aodv_en_mac.h"

static const uint8_t AODV_EN_BROADCAST_MAC[AODV_EN_MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static bool aodv_en_node_is_self(
    const aodv_en_node_t *node,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    return node != NULL && aodv_en_mac_equal(node->self_mac, mac);
}

static bool aodv_en_route_is_usable(
    const aodv_en_route_entry_t *route,
    uint32_t now_ms)
{
    if (route == NULL)
    {
        return false;
    }

    if (route->state == AODV_EN_ROUTE_INVALID)
    {
        return false;
    }

    return route->expires_at_ms > now_ms;
}

static aodv_en_route_entry_t *aodv_en_node_find_usable_route(
    aodv_en_node_t *node,
    const uint8_t destination[AODV_EN_MAC_ADDR_LEN],
    uint32_t now_ms)
{
    aodv_en_route_entry_t *route = aodv_en_route_find(&node->routes, destination);

    if (!aodv_en_route_is_usable(route, now_ms))
    {
        return NULL;
    }

    return route;
}

static aodv_en_status_t aodv_en_node_emit(
    aodv_en_node_t *node,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    bool broadcast,
    uint32_t now_ms)
{
    aodv_en_status_t status;

    if (node == NULL || frame == NULL || frame_len == 0u)
    {
        return AODV_EN_ERR_ARG;
    }

    if (node->callbacks.emit_frame == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    if (!broadcast)
    {
        (void)aodv_en_peer_touch(&node->peer_cache, next_hop, now_ms);
        (void)aodv_en_neighbor_mark_used(&node->neighbors, next_hop, now_ms);
    }

    status = node->callbacks.emit_frame(
        node->callbacks.user_ctx,
        next_hop,
        frame,
        frame_len,
        broadcast);

    if (status == AODV_EN_OK)
    {
        node->stats.tx_frames++;
    }

    return status;
}

static void aodv_en_fill_header(
    aodv_en_node_t *node,
    aodv_en_header_t *header,
    aodv_en_message_type_t message_type,
    uint8_t flags,
    uint8_t hop_count)
{
    memset(header, 0, sizeof(*header));
    header->protocol_version = AODV_EN_PROTOCOL_VERSION;
    header->message_type = (uint8_t)message_type;
    header->flags = flags;
    header->hop_count = hop_count;
    header->network_id = node->config.network_id;
    aodv_en_mac_copy(header->sender_mac, node->self_mac);
}

static aodv_en_status_t aodv_en_node_send_rreq(
    aodv_en_node_t *node,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t now_ms)
{
    aodv_en_rreq_msg_t message;

    if (node == NULL || aodv_en_mac_is_zero(destination_mac))
    {
        return AODV_EN_ERR_ARG;
    }

    aodv_en_fill_header(node, &message.header, AODV_EN_MSG_RREQ, AODV_EN_MSG_FLAG_NONE, 0u);
    aodv_en_mac_copy(message.originator_mac, node->self_mac);
    aodv_en_mac_copy(message.destination_mac, destination_mac);
    message.originator_seq_num = ++node->self_seq_num;
    message.destination_seq_num = 0u;
    message.rreq_id = ++node->next_rreq_id;
    message.ttl = node->config.ttl_default;

    node->stats.route_discoveries++;
    (void)aodv_en_rreq_cache_remember(
        &node->rreq_cache,
        node->self_mac,
        message.rreq_id,
        0u,
        now_ms);

    return aodv_en_node_emit(node, AODV_EN_BROADCAST_MAC, (const uint8_t *)&message, sizeof(message), true, now_ms);
}

static aodv_en_status_t aodv_en_node_send_rrep(
    aodv_en_node_t *node,
    const uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t now_ms)
{
    aodv_en_route_entry_t *reverse_route;
    aodv_en_rrep_msg_t message;

    reverse_route = aodv_en_node_find_usable_route(node, originator_mac, now_ms);
    if (reverse_route == NULL)
    {
        return AODV_EN_ERR_NO_ROUTE;
    }

    aodv_en_fill_header(node, &message.header, AODV_EN_MSG_RREP, AODV_EN_MSG_FLAG_NONE, 0u);
    aodv_en_mac_copy(message.originator_mac, originator_mac);
    aodv_en_mac_copy(message.destination_mac, destination_mac);
    message.destination_seq_num = ++node->self_seq_num;
    message.lifetime_ms = node->config.route_lifetime_ms;

    return aodv_en_node_emit(node, reverse_route->next_hop, (const uint8_t *)&message, sizeof(message), false, now_ms);
}

static aodv_en_status_t aodv_en_node_send_rerr(
    aodv_en_node_t *node,
    const uint8_t unreachable_destination_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t unreachable_dest_seq_num,
    uint32_t now_ms)
{
    aodv_en_rerr_msg_t message;

    if (node == NULL || aodv_en_mac_is_zero(unreachable_destination_mac))
    {
        return AODV_EN_ERR_ARG;
    }

    aodv_en_fill_header(node, &message.header, AODV_EN_MSG_RERR, AODV_EN_MSG_FLAG_ROUTE_REPAIR, 0u);
    aodv_en_mac_copy(message.unreachable_destination_mac, unreachable_destination_mac);
    message.unreachable_dest_seq_num = unreachable_dest_seq_num;

    node->stats.route_repairs++;
    return aodv_en_node_emit(node, AODV_EN_BROADCAST_MAC, (const uint8_t *)&message, sizeof(message), true, now_ms);
}

static aodv_en_status_t aodv_en_node_send_ack(
    aodv_en_node_t *node,
    const uint8_t ack_target_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t ack_for_sequence,
    uint32_t now_ms)
{
    aodv_en_route_entry_t *route;
    aodv_en_ack_msg_t message;

    route = aodv_en_node_find_usable_route(node, ack_target_mac, now_ms);
    if (route == NULL)
    {
        return AODV_EN_ERR_NO_ROUTE;
    }

    aodv_en_fill_header(node, &message.header, AODV_EN_MSG_ACK, AODV_EN_MSG_FLAG_NONE, 0u);
    aodv_en_mac_copy(message.originator_mac, node->self_mac);
    aodv_en_mac_copy(message.destination_mac, ack_target_mac);
    message.ack_for_sequence = ack_for_sequence;

    return aodv_en_node_emit(node, route->next_hop, (const uint8_t *)&message, sizeof(message), false, now_ms);
}

static aodv_en_status_t aodv_en_node_forward_rreq(
    aodv_en_node_t *node,
    const aodv_en_rreq_msg_t *incoming,
    uint32_t now_ms)
{
    aodv_en_rreq_msg_t message = *incoming;

    if (incoming->ttl <= 1u)
    {
        return AODV_EN_NOOP;
    }

    message.header.hop_count++;
    message.ttl--;
    aodv_en_mac_copy(message.header.sender_mac, node->self_mac);

    return aodv_en_node_emit(node, AODV_EN_BROADCAST_MAC, (const uint8_t *)&message, sizeof(message), true, now_ms);
}

static aodv_en_status_t aodv_en_node_forward_rrep(
    aodv_en_node_t *node,
    const aodv_en_rrep_msg_t *incoming,
    uint32_t now_ms)
{
    aodv_en_route_entry_t *reverse_route;
    aodv_en_rrep_msg_t message = *incoming;

    reverse_route = aodv_en_node_find_usable_route(node, incoming->originator_mac, now_ms);
    if (reverse_route == NULL)
    {
        return AODV_EN_ERR_NO_ROUTE;
    }

    message.header.hop_count++;
    aodv_en_mac_copy(message.header.sender_mac, node->self_mac);

    return aodv_en_node_emit(node, reverse_route->next_hop, (const uint8_t *)&message, sizeof(message), false, now_ms);
}

static aodv_en_status_t aodv_en_node_forward_data(
    aodv_en_node_t *node,
    const aodv_en_data_msg_t *incoming,
    size_t frame_len,
    uint32_t now_ms)
{
    aodv_en_route_entry_t *route;
    uint8_t frame_buffer[sizeof(aodv_en_data_msg_t) + AODV_EN_DATA_PAYLOAD_MAX];
    aodv_en_data_msg_t *message = (aodv_en_data_msg_t *)frame_buffer;

    if (incoming->ttl <= 1u)
    {
        (void)aodv_en_node_send_rerr(node, incoming->destination_mac, 0u, now_ms);
        return AODV_EN_ERR_NO_ROUTE;
    }

    route = aodv_en_route_find_valid(&node->routes, incoming->destination_mac);
    if (route == NULL || !aodv_en_route_is_usable(route, now_ms))
    {
        (void)aodv_en_node_send_rerr(node, incoming->destination_mac, 0u, now_ms);
        return AODV_EN_ERR_NO_ROUTE;
    }

    memcpy(frame_buffer, incoming, frame_len);
    message->header.hop_count++;
    message->ttl--;
    aodv_en_mac_copy(message->header.sender_mac, node->self_mac);

    node->stats.forwarded_frames++;
    return aodv_en_node_emit(node, route->next_hop, frame_buffer, frame_len, false, now_ms);
}

static aodv_en_status_t aodv_en_node_handle_hello(
    aodv_en_node_t *node,
    const aodv_en_hello_msg_t *message,
    const uint8_t link_src_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t now_ms)
{
    aodv_en_route_entry_t route;

    memset(&route, 0, sizeof(route));
    aodv_en_mac_copy(route.destination, message->node_mac);
    aodv_en_mac_copy(route.next_hop, link_src_mac);
    route.dest_seq_num = message->node_seq_num;
    route.expires_at_ms = now_ms + node->config.route_lifetime_ms;
    route.metric = 1u;
    route.hop_count = 1u;
    route.state = AODV_EN_ROUTE_VALID;

    return aodv_en_route_upsert(&node->routes, &route);
}

static aodv_en_status_t aodv_en_node_handle_rreq(
    aodv_en_node_t *node,
    const aodv_en_rreq_msg_t *message,
    const uint8_t link_src_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t now_ms)
{
    aodv_en_route_entry_t reverse_route;

    if (aodv_en_rreq_cache_contains(&node->rreq_cache, message->originator_mac, message->rreq_id))
    {
        node->stats.duplicate_rreq_drops++;
        return AODV_EN_NOOP;
    }

    (void)aodv_en_rreq_cache_remember(
        &node->rreq_cache,
        message->originator_mac,
        message->rreq_id,
        message->header.hop_count,
        now_ms);

    memset(&reverse_route, 0, sizeof(reverse_route));
    aodv_en_mac_copy(reverse_route.destination, message->originator_mac);
    aodv_en_mac_copy(reverse_route.next_hop, link_src_mac);
    reverse_route.dest_seq_num = message->originator_seq_num;
    reverse_route.expires_at_ms = now_ms + node->config.route_lifetime_ms;
    reverse_route.metric = (uint16_t)(message->header.hop_count + 1u);
    reverse_route.hop_count = (uint8_t)(message->header.hop_count + 1u);
    reverse_route.state = AODV_EN_ROUTE_REVERSE;
    (void)aodv_en_route_upsert(&node->routes, &reverse_route);

    if (aodv_en_node_is_self(node, message->destination_mac))
    {
        return aodv_en_node_send_rrep(node, message->originator_mac, node->self_mac, now_ms);
    }

    return aodv_en_node_forward_rreq(node, message, now_ms);
}

static aodv_en_status_t aodv_en_node_handle_rrep(
    aodv_en_node_t *node,
    const aodv_en_rrep_msg_t *message,
    const uint8_t link_src_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t now_ms)
{
    aodv_en_route_entry_t route;

    memset(&route, 0, sizeof(route));
    aodv_en_mac_copy(route.destination, message->destination_mac);
    aodv_en_mac_copy(route.next_hop, link_src_mac);
    route.dest_seq_num = message->destination_seq_num;
    route.expires_at_ms = now_ms + message->lifetime_ms;
    route.metric = (uint16_t)(message->header.hop_count + 1u);
    route.hop_count = (uint8_t)(message->header.hop_count + 1u);
    route.state = AODV_EN_ROUTE_VALID;
    (void)aodv_en_route_upsert(&node->routes, &route);

    if (aodv_en_node_is_self(node, message->originator_mac))
    {
        return AODV_EN_OK;
    }

    node->stats.forwarded_frames++;
    return aodv_en_node_forward_rrep(node, message, now_ms);
}

static aodv_en_status_t aodv_en_node_handle_rerr(
    aodv_en_node_t *node,
    const aodv_en_rerr_msg_t *message,
    uint32_t now_ms)
{
    return (aodv_en_route_invalidate_destination(
                &node->routes,
                message->unreachable_destination_mac,
                now_ms) == AODV_EN_OK)
               ? AODV_EN_OK
               : AODV_EN_NOOP;
}

static aodv_en_status_t aodv_en_node_handle_ack(
    aodv_en_node_t *node,
    const aodv_en_ack_msg_t *message,
    uint32_t now_ms)
{
    aodv_en_route_entry_t *route;
    aodv_en_ack_msg_t forward = *message;

    if (aodv_en_node_is_self(node, message->destination_mac))
    {
        node->stats.ack_received++;
        if (node->callbacks.ack_received != NULL)
        {
            node->callbacks.ack_received(
                node->callbacks.user_ctx,
                message->originator_mac,
                message->ack_for_sequence);
        }
        return AODV_EN_OK;
    }

    route = aodv_en_node_find_usable_route(node, message->destination_mac, now_ms);
    if (route == NULL)
    {
        return AODV_EN_ERR_NO_ROUTE;
    }

    forward.header.hop_count++;
    aodv_en_mac_copy(forward.header.sender_mac, node->self_mac);
    node->stats.forwarded_frames++;
    return aodv_en_node_emit(node, route->next_hop, (const uint8_t *)&forward, sizeof(forward), false, now_ms);
}

static aodv_en_status_t aodv_en_node_handle_data(
    aodv_en_node_t *node,
    const aodv_en_data_msg_t *message,
    size_t frame_len,
    uint32_t now_ms)
{
    size_t expected_len = sizeof(aodv_en_data_msg_t) + message->payload_length;

    if (expected_len != frame_len)
    {
        return AODV_EN_ERR_PARSE;
    }

    if (aodv_en_node_is_self(node, message->destination_mac))
    {
        node->stats.delivered_frames++;
        if (node->callbacks.deliver_data != NULL)
        {
            node->callbacks.deliver_data(
                node->callbacks.user_ctx,
                message->originator_mac,
                message->payload,
                message->payload_length);
        }

        if ((message->header.flags & AODV_EN_MSG_FLAG_ACK_REQUIRED) != 0u)
        {
            (void)aodv_en_node_send_ack(node, message->originator_mac, message->sequence_number, now_ms);
        }

        return AODV_EN_OK;
    }

    return aodv_en_node_forward_data(node, message, frame_len, now_ms);
}

static bool aodv_en_validate_header(
    const aodv_en_node_t *node,
    const uint8_t *frame,
    size_t frame_len)
{
    const aodv_en_header_t *header;

    if (node == NULL || frame == NULL || frame_len < sizeof(aodv_en_header_t))
    {
        return false;
    }

    header = (const aodv_en_header_t *)frame;
    return header->protocol_version == AODV_EN_PROTOCOL_VERSION &&
           header->network_id == node->config.network_id;
}

static bool aodv_en_validate_message_size(
    aodv_en_message_type_t type,
    const uint8_t *frame,
    size_t frame_len)
{
    switch (type)
    {
        case AODV_EN_MSG_HELLO:
            return frame_len == sizeof(aodv_en_hello_msg_t);
        case AODV_EN_MSG_RREQ:
            return frame_len == sizeof(aodv_en_rreq_msg_t);
        case AODV_EN_MSG_RREP:
            return frame_len == sizeof(aodv_en_rrep_msg_t);
        case AODV_EN_MSG_RERR:
            return frame_len == sizeof(aodv_en_rerr_msg_t);
        case AODV_EN_MSG_ACK:
            return frame_len == sizeof(aodv_en_ack_msg_t);
        case AODV_EN_MSG_DATA:
            if (frame_len < sizeof(aodv_en_data_msg_t))
            {
                return false;
            }

            return (((const aodv_en_data_msg_t *)frame)->payload_length + sizeof(aodv_en_data_msg_t)) == frame_len;
        default:
            return false;
    }
}

void aodv_en_config_set_defaults(aodv_en_config_t *config)
{
    if (config == NULL)
    {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->neighbor_timeout_ms = AODV_EN_NEIGHBOR_TIMEOUT_MS_DEFAULT;
    config->route_lifetime_ms = AODV_EN_ROUTE_LIFETIME_MS_DEFAULT;
    config->rreq_cache_timeout_ms = AODV_EN_RREQ_CACHE_TIMEOUT_MS_DEFAULT;
    config->ack_timeout_ms = AODV_EN_ACK_TIMEOUT_MS_DEFAULT;
    config->neighbor_table_size = AODV_EN_NEIGHBOR_TABLE_SIZE;
    config->route_table_size = AODV_EN_ROUTE_TABLE_SIZE;
    config->rreq_cache_size = AODV_EN_RREQ_CACHE_SIZE;
    config->peer_cache_size = AODV_EN_PEER_CACHE_SIZE;
    config->control_payload_max = AODV_EN_CONTROL_PAYLOAD_MAX;
    config->data_payload_max = AODV_EN_DATA_PAYLOAD_MAX;
    config->max_hops = AODV_EN_MAX_HOPS_DEFAULT;
    config->ttl_default = AODV_EN_TTL_DEFAULT;
    config->rreq_retry_count = AODV_EN_RREQ_RETRY_COUNT_DEFAULT;
}

aodv_en_status_t aodv_en_node_init(
    aodv_en_node_t *node,
    const aodv_en_config_t *config,
    const uint8_t self_mac[AODV_EN_MAC_ADDR_LEN])
{
    if (node == NULL || aodv_en_mac_is_zero(self_mac))
    {
        return AODV_EN_ERR_ARG;
    }

    memset(node, 0, sizeof(*node));
    if (config != NULL)
    {
        node->config = *config;
    }
    else
    {
        aodv_en_config_set_defaults(&node->config);
    }

    aodv_en_mac_copy(node->self_mac, self_mac);
    aodv_en_neighbor_table_init(&node->neighbors);
    aodv_en_route_table_init(&node->routes);
    aodv_en_rreq_cache_init(&node->rreq_cache);
    aodv_en_peer_cache_init(&node->peer_cache);

    return AODV_EN_OK;
}

void aodv_en_node_set_callbacks(
    aodv_en_node_t *node,
    const aodv_en_node_callbacks_t *callbacks)
{
    if (node == NULL)
    {
        return;
    }

    if (callbacks == NULL)
    {
        memset(&node->callbacks, 0, sizeof(node->callbacks));
        return;
    }

    node->callbacks = *callbacks;
}

void aodv_en_node_tick(
    aodv_en_node_t *node,
    uint32_t now_ms)
{
    if (node == NULL)
    {
        return;
    }

    (void)aodv_en_neighbor_expire(&node->neighbors, now_ms, node->config.neighbor_timeout_ms);
    (void)aodv_en_route_expire(&node->routes, now_ms);
    (void)aodv_en_rreq_cache_expire(&node->rreq_cache, now_ms, node->config.rreq_cache_timeout_ms);
}

aodv_en_status_t aodv_en_node_send_hello(
    aodv_en_node_t *node,
    uint32_t now_ms)
{
    aodv_en_hello_msg_t message;

    if (node == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    aodv_en_fill_header(node, &message.header, AODV_EN_MSG_HELLO, AODV_EN_MSG_FLAG_NONE, 0u);
    aodv_en_mac_copy(message.node_mac, node->self_mac);
    message.node_seq_num = ++node->self_seq_num;
    message.timestamp_ms = now_ms;

    return aodv_en_node_emit(node, AODV_EN_BROADCAST_MAC, (const uint8_t *)&message, sizeof(message), true, now_ms);
}

aodv_en_status_t aodv_en_node_send_data(
    aodv_en_node_t *node,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len,
    bool ack_required,
    uint32_t now_ms)
{
    aodv_en_route_entry_t *route;
    uint8_t frame_buffer[sizeof(aodv_en_data_msg_t) + AODV_EN_DATA_PAYLOAD_MAX];
    aodv_en_data_msg_t *message = (aodv_en_data_msg_t *)frame_buffer;
    size_t frame_len;

    if (node == NULL || payload == NULL || payload_len == 0u ||
        aodv_en_mac_is_zero(destination_mac))
    {
        return AODV_EN_ERR_ARG;
    }

    if (payload_len > node->config.data_payload_max || payload_len > AODV_EN_DATA_PAYLOAD_MAX)
    {
        return AODV_EN_ERR_SIZE;
    }

    route = aodv_en_route_find_valid(&node->routes, destination_mac);
    if (route == NULL || !aodv_en_route_is_usable(route, now_ms))
    {
        (void)aodv_en_node_send_rreq(node, destination_mac, now_ms);
        return AODV_EN_ERR_NO_ROUTE;
    }

    aodv_en_fill_header(
        node,
        &message->header,
        AODV_EN_MSG_DATA,
        ack_required ? AODV_EN_MSG_FLAG_ACK_REQUIRED : AODV_EN_MSG_FLAG_NONE,
        0u);
    aodv_en_mac_copy(message->originator_mac, node->self_mac);
    aodv_en_mac_copy(message->destination_mac, destination_mac);
    message->sequence_number = ++node->next_data_seq;
    message->ttl = node->config.ttl_default;
    message->payload_length = payload_len;
    memcpy(message->payload, payload, payload_len);

    frame_len = sizeof(aodv_en_data_msg_t) + payload_len;
    return aodv_en_node_emit(node, route->next_hop, frame_buffer, frame_len, false, now_ms);
}

aodv_en_status_t aodv_en_node_on_recv(
    aodv_en_node_t *node,
    const uint8_t link_src_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    int8_t rssi,
    uint32_t now_ms)
{
    const aodv_en_header_t *header;

    if (node == NULL || aodv_en_mac_is_zero(link_src_mac) || frame == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    if (!aodv_en_validate_header(node, frame, frame_len))
    {
        return AODV_EN_ERR_PARSE;
    }

    header = (const aodv_en_header_t *)frame;
    if (!aodv_en_validate_message_size((aodv_en_message_type_t)header->message_type, frame, frame_len))
    {
        return AODV_EN_ERR_SIZE;
    }

    node->stats.rx_frames++;
    (void)aodv_en_neighbor_touch(&node->neighbors, link_src_mac, rssi, now_ms);

    switch ((aodv_en_message_type_t)header->message_type)
    {
        case AODV_EN_MSG_HELLO:
            return aodv_en_node_handle_hello(node, (const aodv_en_hello_msg_t *)frame, link_src_mac, now_ms);
        case AODV_EN_MSG_RREQ:
            return aodv_en_node_handle_rreq(node, (const aodv_en_rreq_msg_t *)frame, link_src_mac, now_ms);
        case AODV_EN_MSG_RREP:
            return aodv_en_node_handle_rrep(node, (const aodv_en_rrep_msg_t *)frame, link_src_mac, now_ms);
        case AODV_EN_MSG_RERR:
            return aodv_en_node_handle_rerr(node, (const aodv_en_rerr_msg_t *)frame, now_ms);
        case AODV_EN_MSG_DATA:
            return aodv_en_node_handle_data(node, (const aodv_en_data_msg_t *)frame, frame_len, now_ms);
        case AODV_EN_MSG_ACK:
            return aodv_en_node_handle_ack(node, (const aodv_en_ack_msg_t *)frame, now_ms);
        default:
            return AODV_EN_ERR_PARSE;
    }
}
