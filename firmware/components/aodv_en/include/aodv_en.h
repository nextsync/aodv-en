#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "aodv_en_status.h"
#include "aodv_en_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef uint32_t (*aodv_en_now_ms_fn)(void *user_ctx);

    typedef aodv_en_status_t (*aodv_en_adapter_tx_frame_fn)(
        void *user_ctx,
        const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *frame,
        size_t frame_len,
        bool broadcast);

    typedef struct
    {
        void *user_ctx;
        aodv_en_now_ms_fn now_ms;
        aodv_en_adapter_tx_frame_fn tx_frame;
    } aodv_en_adapter_t;

    typedef void (*aodv_en_on_data_fn)(
        void *user_ctx,
        const uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *payload,
        uint16_t payload_len);

    typedef void (*aodv_en_on_ack_fn)(
        void *user_ctx,
        const uint8_t ack_sender_mac[AODV_EN_MAC_ADDR_LEN],
        uint32_t sequence_number);

    typedef struct
    {
        void *user_ctx;
        aodv_en_on_data_fn on_data;
        aodv_en_on_ack_fn on_ack;
    } aodv_en_app_callbacks_t;

    typedef struct
    {
        uint32_t rx_frames;
        uint32_t tx_frames;
        uint32_t forwarded_frames;
        uint32_t delivered_frames;
        uint32_t ack_received;
        uint32_t route_discoveries;
        uint32_t route_repairs;
        uint32_t duplicate_rreq_drops;
        uint32_t pending_data_queued;
        uint32_t pending_data_flushed;
        uint32_t pending_data_dropped;
        uint32_t route_discovery_retries;
        uint32_t ack_retry_sent;
        uint32_t ack_timeout_drops;
        uint32_t link_fail_events;
        uint32_t route_invalidations_link_fail;
    } aodv_en_stack_stats_t;

    typedef struct
    {
        uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN];
        uint8_t next_hop_mac[AODV_EN_MAC_ADDR_LEN];
        uint32_t dest_seq_num;
        uint32_t expires_at_ms;
        uint16_t metric;
        uint8_t hop_count;
        uint8_t state;
    } aodv_en_route_snapshot_t;

    typedef struct
    {
        uint16_t routes_count;
        uint16_t neighbors_count;
        aodv_en_stack_stats_t stats;
    } aodv_en_overview_t;

    typedef struct
    {
        void *impl;
    } aodv_en_stack_t;

    void aodv_en_config_set_defaults(
        aodv_en_config_t *config);

    aodv_en_status_t aodv_en_stack_init(
        aodv_en_stack_t *stack,
        const aodv_en_config_t *config,
        const uint8_t self_mac[AODV_EN_MAC_ADDR_LEN],
        const aodv_en_adapter_t *adapter,
        const aodv_en_app_callbacks_t *app_callbacks);

    void aodv_en_stack_deinit(
        aodv_en_stack_t *stack);

    void aodv_en_stack_set_app_callbacks(
        aodv_en_stack_t *stack,
        const aodv_en_app_callbacks_t *app_callbacks);

    void aodv_en_stack_tick_at(
        aodv_en_stack_t *stack,
        uint32_t now_ms);

    aodv_en_status_t aodv_en_stack_tick(
        aodv_en_stack_t *stack);

    aodv_en_status_t aodv_en_stack_send_hello_at(
        aodv_en_stack_t *stack,
        uint32_t now_ms);

    aodv_en_status_t aodv_en_stack_send_hello(
        aodv_en_stack_t *stack);

    aodv_en_status_t aodv_en_stack_send_data_at(
        aodv_en_stack_t *stack,
        const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *payload,
        uint16_t payload_len,
        bool ack_required,
        uint32_t now_ms);

    aodv_en_status_t aodv_en_stack_send_data(
        aodv_en_stack_t *stack,
        const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *payload,
        uint16_t payload_len,
        bool ack_required);

    aodv_en_status_t aodv_en_stack_on_recv_at(
        aodv_en_stack_t *stack,
        const uint8_t link_src_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *frame,
        size_t frame_len,
        int8_t rssi,
        uint32_t now_ms);

    aodv_en_status_t aodv_en_stack_on_recv(
        aodv_en_stack_t *stack,
        const uint8_t link_src_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *frame,
        size_t frame_len,
        int8_t rssi);

    aodv_en_status_t aodv_en_stack_on_link_tx_result_at(
        aodv_en_stack_t *stack,
        const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
        bool success,
        uint32_t now_ms,
        size_t *invalidated_routes);

    aodv_en_status_t aodv_en_stack_on_link_tx_result(
        aodv_en_stack_t *stack,
        const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
        bool success,
        size_t *invalidated_routes);

    aodv_en_status_t aodv_en_stack_get_overview(
        const aodv_en_stack_t *stack,
        aodv_en_overview_t *overview);

    size_t aodv_en_stack_get_route_count(
        const aodv_en_stack_t *stack);

    aodv_en_status_t aodv_en_stack_get_route_at(
        const aodv_en_stack_t *stack,
        size_t route_index,
        aodv_en_route_snapshot_t *route);

    aodv_en_status_t aodv_en_stack_get_stats(
        const aodv_en_stack_t *stack,
        aodv_en_stack_stats_t *stats);

#ifdef __cplusplus
}
#endif
