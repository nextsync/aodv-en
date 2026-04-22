#include "aodv_en.h"

#include <stdlib.h>
#include <string.h>

#include "aodv_en_node.h"

typedef struct
{
    aodv_en_node_t node;
    aodv_en_adapter_t adapter;
    aodv_en_app_callbacks_t app_callbacks;
} aodv_en_stack_impl_t;

static aodv_en_stack_impl_t *aodv_en_stack_impl(aodv_en_stack_t *stack)
{
    if (stack == NULL || stack->impl == NULL)
    {
        return NULL;
    }

    return (aodv_en_stack_impl_t *)stack->impl;
}

static const aodv_en_stack_impl_t *aodv_en_stack_impl_const(const aodv_en_stack_t *stack)
{
    if (stack == NULL || stack->impl == NULL)
    {
        return NULL;
    }

    return (const aodv_en_stack_impl_t *)stack->impl;
}

static void aodv_en_stats_from_node(
    const aodv_en_node_t *node,
    aodv_en_stack_stats_t *stats)
{
    if (node == NULL || stats == NULL)
    {
        return;
    }

    stats->rx_frames = node->stats.rx_frames;
    stats->tx_frames = node->stats.tx_frames;
    stats->forwarded_frames = node->stats.forwarded_frames;
    stats->delivered_frames = node->stats.delivered_frames;
    stats->ack_received = node->stats.ack_received;
    stats->route_discoveries = node->stats.route_discoveries;
    stats->route_repairs = node->stats.route_repairs;
    stats->duplicate_rreq_drops = node->stats.duplicate_rreq_drops;
    stats->pending_data_queued = node->stats.pending_data_queued;
    stats->pending_data_flushed = node->stats.pending_data_flushed;
    stats->pending_data_dropped = node->stats.pending_data_dropped;
    stats->route_discovery_retries = node->stats.route_discovery_retries;
    stats->ack_retry_sent = node->stats.ack_retry_sent;
    stats->ack_timeout_drops = node->stats.ack_timeout_drops;
    stats->link_fail_events = node->stats.link_fail_events;
    stats->route_invalidations_link_fail = node->stats.route_invalidations_link_fail;
}

static aodv_en_status_t aodv_en_stack_emit_frame(
    void *user_ctx,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    bool broadcast)
{
    aodv_en_stack_impl_t *impl = (aodv_en_stack_impl_t *)user_ctx;

    if (impl == NULL || impl->adapter.tx_frame == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    return impl->adapter.tx_frame(
        impl->adapter.user_ctx,
        next_hop,
        frame,
        frame_len,
        broadcast);
}

static void aodv_en_stack_deliver_data(
    void *user_ctx,
    const uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len)
{
    aodv_en_stack_impl_t *impl = (aodv_en_stack_impl_t *)user_ctx;

    if (impl == NULL || impl->app_callbacks.on_data == NULL)
    {
        return;
    }

    impl->app_callbacks.on_data(
        impl->app_callbacks.user_ctx,
        originator_mac,
        payload,
        payload_len);
}

static void aodv_en_stack_ack_received(
    void *user_ctx,
    const uint8_t ack_sender_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t sequence_number)
{
    aodv_en_stack_impl_t *impl = (aodv_en_stack_impl_t *)user_ctx;

    if (impl == NULL || impl->app_callbacks.on_ack == NULL)
    {
        return;
    }

    impl->app_callbacks.on_ack(
        impl->app_callbacks.user_ctx,
        ack_sender_mac,
        sequence_number);
}

static aodv_en_status_t aodv_en_stack_now_ms(
    aodv_en_stack_impl_t *impl,
    uint32_t *now_ms)
{
    if (impl == NULL || now_ms == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    if (impl->adapter.now_ms == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    *now_ms = impl->adapter.now_ms(impl->adapter.user_ctx);
    return AODV_EN_OK;
}

aodv_en_status_t aodv_en_stack_init(
    aodv_en_stack_t *stack,
    const aodv_en_config_t *config,
    const uint8_t self_mac[AODV_EN_MAC_ADDR_LEN],
    const aodv_en_adapter_t *adapter,
    const aodv_en_app_callbacks_t *app_callbacks)
{
    aodv_en_stack_impl_t *impl;
    aodv_en_node_callbacks_t callbacks;
    aodv_en_status_t status;

    if (stack == NULL || adapter == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    if (adapter->tx_frame == NULL || adapter->now_ms == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    if (stack->impl != NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    impl = (aodv_en_stack_impl_t *)calloc(1u, sizeof(*impl));
    if (impl == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    impl->adapter = *adapter;
    if (app_callbacks != NULL)
    {
        impl->app_callbacks = *app_callbacks;
    }

    status = aodv_en_node_init(&impl->node, config, self_mac);
    if (status != AODV_EN_OK)
    {
        free(impl);
        return status;
    }

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.emit_frame = aodv_en_stack_emit_frame;
    callbacks.deliver_data = aodv_en_stack_deliver_data;
    callbacks.ack_received = aodv_en_stack_ack_received;
    callbacks.user_ctx = impl;
    aodv_en_node_set_callbacks(&impl->node, &callbacks);

    stack->impl = impl;
    return AODV_EN_OK;
}

void aodv_en_stack_deinit(
    aodv_en_stack_t *stack)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);

    if (impl == NULL)
    {
        return;
    }

    free(impl);
    stack->impl = NULL;
}

void aodv_en_stack_set_app_callbacks(
    aodv_en_stack_t *stack,
    const aodv_en_app_callbacks_t *app_callbacks)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);

    if (impl == NULL)
    {
        return;
    }

    if (app_callbacks == NULL)
    {
        memset(&impl->app_callbacks, 0, sizeof(impl->app_callbacks));
        return;
    }

    impl->app_callbacks = *app_callbacks;
}

void aodv_en_stack_tick_at(
    aodv_en_stack_t *stack,
    uint32_t now_ms)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);

    if (impl == NULL)
    {
        return;
    }

    aodv_en_node_tick(&impl->node, now_ms);
}

aodv_en_status_t aodv_en_stack_tick(
    aodv_en_stack_t *stack)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);
    uint32_t now_ms = 0u;
    aodv_en_status_t status;

    if (impl == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    status = aodv_en_stack_now_ms(impl, &now_ms);
    if (status != AODV_EN_OK)
    {
        return status;
    }

    aodv_en_node_tick(&impl->node, now_ms);
    return AODV_EN_OK;
}

aodv_en_status_t aodv_en_stack_send_hello_at(
    aodv_en_stack_t *stack,
    uint32_t now_ms)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);

    if (impl == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    return aodv_en_node_send_hello(&impl->node, now_ms);
}

aodv_en_status_t aodv_en_stack_send_hello(
    aodv_en_stack_t *stack)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);
    uint32_t now_ms = 0u;
    aodv_en_status_t status;

    if (impl == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    status = aodv_en_stack_now_ms(impl, &now_ms);
    if (status != AODV_EN_OK)
    {
        return status;
    }

    return aodv_en_node_send_hello(&impl->node, now_ms);
}

aodv_en_status_t aodv_en_stack_send_data_at(
    aodv_en_stack_t *stack,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len,
    bool ack_required,
    uint32_t now_ms)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);

    if (impl == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    return aodv_en_node_send_data(
        &impl->node,
        destination_mac,
        payload,
        payload_len,
        ack_required,
        now_ms);
}

aodv_en_status_t aodv_en_stack_send_data(
    aodv_en_stack_t *stack,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len,
    bool ack_required)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);
    uint32_t now_ms = 0u;
    aodv_en_status_t status;

    if (impl == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    status = aodv_en_stack_now_ms(impl, &now_ms);
    if (status != AODV_EN_OK)
    {
        return status;
    }

    return aodv_en_node_send_data(
        &impl->node,
        destination_mac,
        payload,
        payload_len,
        ack_required,
        now_ms);
}

aodv_en_status_t aodv_en_stack_on_recv_at(
    aodv_en_stack_t *stack,
    const uint8_t link_src_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    int8_t rssi,
    uint32_t now_ms)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);

    if (impl == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    return aodv_en_node_on_recv(
        &impl->node,
        link_src_mac,
        frame,
        frame_len,
        rssi,
        now_ms);
}

aodv_en_status_t aodv_en_stack_on_recv(
    aodv_en_stack_t *stack,
    const uint8_t link_src_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    int8_t rssi)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);
    uint32_t now_ms = 0u;
    aodv_en_status_t status;

    if (impl == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    status = aodv_en_stack_now_ms(impl, &now_ms);
    if (status != AODV_EN_OK)
    {
        return status;
    }

    return aodv_en_node_on_recv(
        &impl->node,
        link_src_mac,
        frame,
        frame_len,
        rssi,
        now_ms);
}

aodv_en_status_t aodv_en_stack_on_link_tx_result_at(
    aodv_en_stack_t *stack,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    bool success,
    uint32_t now_ms,
    size_t *invalidated_routes)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);

    if (impl == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    return aodv_en_node_on_link_tx_result(
        &impl->node,
        next_hop,
        success,
        now_ms,
        invalidated_routes);
}

aodv_en_status_t aodv_en_stack_on_link_tx_result(
    aodv_en_stack_t *stack,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    bool success,
    size_t *invalidated_routes)
{
    aodv_en_stack_impl_t *impl = aodv_en_stack_impl(stack);
    uint32_t now_ms = 0u;
    aodv_en_status_t status;

    if (impl == NULL)
    {
        return AODV_EN_ERR_STATE;
    }

    status = aodv_en_stack_now_ms(impl, &now_ms);
    if (status != AODV_EN_OK)
    {
        return status;
    }

    return aodv_en_node_on_link_tx_result(
        &impl->node,
        next_hop,
        success,
        now_ms,
        invalidated_routes);
}

aodv_en_status_t aodv_en_stack_get_overview(
    const aodv_en_stack_t *stack,
    aodv_en_overview_t *overview)
{
    const aodv_en_stack_impl_t *impl = aodv_en_stack_impl_const(stack);

    if (impl == NULL || overview == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    memset(overview, 0, sizeof(*overview));
    overview->routes_count = impl->node.routes.count;
    overview->neighbors_count = impl->node.neighbors.count;
    aodv_en_stats_from_node(&impl->node, &overview->stats);
    return AODV_EN_OK;
}

size_t aodv_en_stack_get_route_count(
    const aodv_en_stack_t *stack)
{
    const aodv_en_stack_impl_t *impl = aodv_en_stack_impl_const(stack);

    if (impl == NULL)
    {
        return 0u;
    }

    return (size_t)impl->node.routes.count;
}

aodv_en_status_t aodv_en_stack_get_route_at(
    const aodv_en_stack_t *stack,
    size_t route_index,
    aodv_en_route_snapshot_t *route)
{
    const aodv_en_stack_impl_t *impl = aodv_en_stack_impl_const(stack);
    const aodv_en_route_entry_t *entry;

    if (impl == NULL || route == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    if (route_index >= (size_t)impl->node.routes.count)
    {
        return AODV_EN_ERR_NOT_FOUND;
    }

    entry = &impl->node.routes.entries[route_index];
    memset(route, 0, sizeof(*route));
    memcpy(route->destination_mac, entry->destination, AODV_EN_MAC_ADDR_LEN);
    memcpy(route->next_hop_mac, entry->next_hop, AODV_EN_MAC_ADDR_LEN);
    route->dest_seq_num = entry->dest_seq_num;
    route->expires_at_ms = entry->expires_at_ms;
    route->metric = entry->metric;
    route->hop_count = entry->hop_count;
    route->state = entry->state;
    return AODV_EN_OK;
}

aodv_en_status_t aodv_en_stack_get_stats(
    const aodv_en_stack_t *stack,
    aodv_en_stack_stats_t *stats)
{
    const aodv_en_stack_impl_t *impl = aodv_en_stack_impl_const(stack);

    if (impl == NULL || stats == NULL)
    {
        return AODV_EN_ERR_ARG;
    }

    memset(stats, 0, sizeof(*stats));
    aodv_en_stats_from_node(&impl->node, stats);
    return AODV_EN_OK;
}
