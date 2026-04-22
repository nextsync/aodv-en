#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "aodv_en_messages.h"
#include "aodv_en_neighbors.h"
#include "aodv_en_peers.h"
#include "aodv_en_rreq_cache.h"
#include "aodv_en_routes.h"
#include "aodv_en_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
    } aodv_en_stats_t;

    typedef struct
    {
        uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN];
        uint16_t payload_len;
        bool ack_required;
        bool used;
        uint32_t enqueued_at_ms;
        uint32_t last_rreq_at_ms;
        uint8_t discovery_attempts;
        uint8_t payload[AODV_EN_DATA_PAYLOAD_MAX];
    } aodv_en_pending_data_entry_t;

    typedef struct
    {
        uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN];
        uint16_t payload_len;
        bool used;
        uint8_t retries_left;
        uint32_t sequence_number;
        uint32_t last_sent_at_ms;
        uint8_t payload[AODV_EN_DATA_PAYLOAD_MAX];
    } aodv_en_pending_ack_entry_t;

    typedef aodv_en_status_t (*aodv_en_emit_frame_fn)(
        void *user_ctx,
        const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *frame,
        size_t frame_len,
        bool broadcast);

    typedef void (*aodv_en_deliver_data_fn)(
        void *user_ctx,
        const uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *payload,
        uint16_t payload_len);

    typedef void (*aodv_en_ack_received_fn)(
        void *user_ctx,
        const uint8_t ack_sender_mac[AODV_EN_MAC_ADDR_LEN],
        uint32_t sequence_number);

    typedef struct
    {
        aodv_en_emit_frame_fn emit_frame;
        aodv_en_deliver_data_fn deliver_data;
        aodv_en_ack_received_fn ack_received;
        void *user_ctx;
    } aodv_en_node_callbacks_t;

    typedef struct
    {
        aodv_en_config_t config;
        uint8_t self_mac[AODV_EN_MAC_ADDR_LEN];
        uint32_t self_seq_num;
        uint32_t next_rreq_id;
        uint32_t next_data_seq;
        aodv_en_neighbor_table_t neighbors;
        aodv_en_route_table_t routes;
        aodv_en_rreq_cache_t rreq_cache;
        aodv_en_peer_cache_t peer_cache;
        aodv_en_pending_data_entry_t pending_data[AODV_EN_PENDING_DATA_QUEUE_SIZE];
        uint16_t pending_data_count;
        aodv_en_pending_ack_entry_t pending_ack[AODV_EN_PENDING_DATA_QUEUE_SIZE];
        uint16_t pending_ack_count;
        aodv_en_node_callbacks_t callbacks;
        aodv_en_stats_t stats;
    } aodv_en_node_t;

    void aodv_en_config_set_defaults(aodv_en_config_t *config);

    aodv_en_status_t aodv_en_node_init(
        aodv_en_node_t *node,
        const aodv_en_config_t *config,
        const uint8_t self_mac[AODV_EN_MAC_ADDR_LEN]);

    void aodv_en_node_set_callbacks(
        aodv_en_node_t *node,
        const aodv_en_node_callbacks_t *callbacks);

    void aodv_en_node_tick(
        aodv_en_node_t *node,
        uint32_t now_ms);

    aodv_en_status_t aodv_en_node_send_hello(
        aodv_en_node_t *node,
        uint32_t now_ms);

    aodv_en_status_t aodv_en_node_send_data(
        aodv_en_node_t *node,
        const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *payload,
        uint16_t payload_len,
        bool ack_required,
        uint32_t now_ms);

    aodv_en_status_t aodv_en_node_on_recv(
        aodv_en_node_t *node,
        const uint8_t link_src_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *frame,
        size_t frame_len,
        int8_t rssi,
        uint32_t now_ms);

    aodv_en_status_t aodv_en_node_on_link_tx_result(
        aodv_en_node_t *node,
        const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
        bool success,
        uint32_t now_ms,
        size_t *invalidated_routes);

#ifdef __cplusplus
}
#endif
