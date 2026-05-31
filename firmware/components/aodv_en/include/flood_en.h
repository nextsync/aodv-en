#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "aodv_en_limits.h"
#include "aodv_en_node.h"
#include "aodv_en_status.h"
#include "aodv_en_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef FLOOD_EN_SEEN_SIZE
#define FLOOD_EN_SEEN_SIZE 64u
#endif

    typedef struct
    {
        uint8_t origin[AODV_EN_MAC_ADDR_LEN];
        uint32_t seq;
        uint32_t inserted_at_ms;
        bool used;
    } flood_en_seen_entry_t;

    typedef struct
    {
        uint32_t rx_frames;
        uint32_t tx_frames;
        uint32_t rebroadcast_frames;
        uint32_t delivered_frames;
        uint32_t ack_received;
        uint32_t duplicate_drops;
        uint32_t ttl_drops;
        uint32_t foreign_drops;
    } flood_en_stats_t;

    typedef struct
    {
        aodv_en_config_t config;
        uint8_t self_mac[AODV_EN_MAC_ADDR_LEN];
        uint32_t next_seq;
        aodv_en_node_callbacks_t callbacks;
        flood_en_seen_entry_t seen_data[FLOOD_EN_SEEN_SIZE];
        flood_en_seen_entry_t seen_ack[FLOOD_EN_SEEN_SIZE];
        flood_en_stats_t stats;
    } flood_en_node_t;

    aodv_en_status_t flood_en_node_init(
        flood_en_node_t *node,
        const aodv_en_config_t *config,
        const uint8_t self_mac[AODV_EN_MAC_ADDR_LEN]);

    void flood_en_node_set_callbacks(
        flood_en_node_t *node,
        const aodv_en_node_callbacks_t *callbacks);

    void flood_en_node_tick(
        flood_en_node_t *node,
        uint32_t now_ms);

    aodv_en_status_t flood_en_node_send_data(
        flood_en_node_t *node,
        const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *payload,
        uint16_t payload_len,
        bool ack_required,
        uint32_t now_ms);

    aodv_en_status_t flood_en_node_on_recv(
        flood_en_node_t *node,
        const uint8_t link_src_mac[AODV_EN_MAC_ADDR_LEN],
        const uint8_t *frame,
        size_t frame_len,
        int8_t rssi,
        uint32_t now_ms);

#ifdef __cplusplus
}
#endif
