#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "aodv_en_limits.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        AODV_EN_MSG_HELLO = 0,
        AODV_EN_MSG_RREQ = 1,
        AODV_EN_MSG_RREP = 2,
        AODV_EN_MSG_RERR = 3,
        AODV_EN_MSG_DATA = 4,
        AODV_EN_MSG_ACK = 5,
    } aodv_en_message_type_t;

    typedef enum
    {
        AODV_EN_NEIGHBOR_INACTIVE = 0,
        AODV_EN_NEIGHBOR_ACTIVE = 1,
    } aodv_en_neighbor_state_t;

    typedef enum
    {
        AODV_EN_ROUTE_INVALID = 0,
        AODV_EN_ROUTE_REVERSE = 1,
        AODV_EN_ROUTE_VALID = 2,
    } aodv_en_route_state_t;

    typedef enum
    {
        AODV_EN_PEER_FLAG_NONE = 0x00,
        AODV_EN_PEER_FLAG_PINNED = 0x01,
        AODV_EN_PEER_FLAG_REGISTERED = 0x02,
    } aodv_en_peer_flags_t;

    typedef struct
    {
        uint8_t bytes[AODV_EN_MAC_ADDR_LEN];
    } aodv_en_mac_addr_t;

    typedef struct
    {
        uint8_t mac[AODV_EN_MAC_ADDR_LEN];
        int8_t avg_rssi;
        int8_t last_rssi;
        uint8_t link_fail_count;
        uint8_t state;
        uint32_t last_seen_ms;
        uint32_t last_used_ms;
    } aodv_en_neighbor_entry_t;

    typedef struct
    {
        uint8_t destination[AODV_EN_MAC_ADDR_LEN];
        uint8_t next_hop[AODV_EN_MAC_ADDR_LEN];
        uint32_t dest_seq_num;
        uint32_t expires_at_ms;
        uint16_t metric;
        uint8_t hop_count;
        uint8_t state;
        uint8_t precursor_count;
        uint8_t precursors[AODV_EN_MAX_PRECURSORS][AODV_EN_MAC_ADDR_LEN];
    } aodv_en_route_entry_t;

    typedef struct
    {
        uint8_t originator[AODV_EN_MAC_ADDR_LEN];
        uint32_t rreq_id;
        uint32_t created_at_ms;
        uint8_t hop_count;
        uint8_t used;
    } aodv_en_rreq_cache_entry_t;

    typedef struct
    {
        uint8_t mac[AODV_EN_MAC_ADDR_LEN];
        uint32_t last_used_ms;
        uint8_t flags;
        uint8_t reserved;
    } aodv_en_peer_cache_entry_t;

    typedef struct
    {
        uint32_t network_id;
        uint32_t neighbor_timeout_ms;
        uint32_t route_lifetime_ms;
        uint32_t rreq_cache_timeout_ms;
        uint32_t ack_timeout_ms;
        uint16_t neighbor_table_size;
        uint16_t route_table_size;
        uint16_t rreq_cache_size;
        uint16_t peer_cache_size;
        uint16_t control_payload_max;
        uint16_t data_payload_max;
        uint8_t wifi_channel;
        uint8_t max_hops;
        uint8_t ttl_default;
        uint8_t rreq_retry_count;
        uint8_t link_fail_threshold;
    } aodv_en_config_t;

#ifdef __cplusplus
}
#endif
