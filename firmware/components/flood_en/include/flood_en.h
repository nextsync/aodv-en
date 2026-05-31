#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define FLOOD_EN_MAC_ADDR_LEN 6u
#define FLOOD_EN_PROTOCOL_VERSION 1u

#ifndef FLOOD_EN_DATA_PAYLOAD_MAX
#define FLOOD_EN_DATA_PAYLOAD_MAX 1024u
#endif

#ifndef FLOOD_EN_SEEN_SIZE
#define FLOOD_EN_SEEN_SIZE 100u
#endif

#ifndef FLOOD_EN_TTL_DEFAULT
#define FLOOD_EN_TTL_DEFAULT 5u
#endif

#ifndef FLOOD_EN_MAX_HOPS_DEFAULT
#define FLOOD_EN_MAX_HOPS_DEFAULT 5u
#endif

    typedef enum
    {
        FLOOD_EN_OK = 0,
        FLOOD_EN_ERR_ARG = -1,
        FLOOD_EN_ERR_SIZE = -6,
        FLOOD_EN_ERR_STATE = -8,
    } flood_en_status_t;

    typedef enum
    {
        FLOOD_EN_MSG_DATA = 4,
        FLOOD_EN_MSG_ACK = 5,
    } flood_en_message_type_t;

    enum
    {
        FLOOD_EN_FLAG_NONE = 0x00,
        FLOOD_EN_FLAG_ACK_REQUIRED = 0x01,
    };

    typedef struct __attribute__((packed))
    {
        uint8_t protocol_version;
        uint8_t message_type;
        uint8_t flags;
        uint8_t hop_count;
        uint32_t network_id;
        uint8_t sender_mac[FLOOD_EN_MAC_ADDR_LEN];
    } flood_en_header_t;

    typedef struct __attribute__((packed))
    {
        flood_en_header_t header;
        uint8_t originator_mac[FLOOD_EN_MAC_ADDR_LEN];
        uint8_t destination_mac[FLOOD_EN_MAC_ADDR_LEN];
        uint32_t sequence_number;
        uint8_t ttl;
        uint16_t payload_length;
        uint8_t payload[];
    } flood_en_data_msg_t;

    typedef struct __attribute__((packed))
    {
        flood_en_header_t header;
        uint8_t originator_mac[FLOOD_EN_MAC_ADDR_LEN];
        uint8_t destination_mac[FLOOD_EN_MAC_ADDR_LEN];
        uint32_t ack_for_sequence;
    } flood_en_ack_msg_t;

    typedef struct
    {
        uint32_t network_id;
        uint16_t data_payload_max;
        uint8_t ttl_default;
        uint8_t max_hops;
    } flood_en_config_t;

    typedef flood_en_status_t (*flood_en_tx_frame_fn)(
        void *user_ctx,
        const uint8_t next_hop[FLOOD_EN_MAC_ADDR_LEN],
        const uint8_t *frame,
        size_t frame_len,
        bool broadcast);

    typedef void (*flood_en_deliver_data_fn)(
        void *user_ctx,
        const uint8_t originator_mac[FLOOD_EN_MAC_ADDR_LEN],
        const uint8_t *payload,
        uint16_t payload_len);

    typedef void (*flood_en_ack_received_fn)(
        void *user_ctx,
        const uint8_t ack_sender_mac[FLOOD_EN_MAC_ADDR_LEN],
        uint32_t sequence_number);

    typedef struct
    {
        flood_en_tx_frame_fn tx_frame;
        flood_en_deliver_data_fn deliver_data;
        flood_en_ack_received_fn ack_received;
        void *user_ctx;
    } flood_en_callbacks_t;

    typedef struct
    {
        uint8_t origin[FLOOD_EN_MAC_ADDR_LEN];
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
        flood_en_config_t config;
        uint8_t self_mac[FLOOD_EN_MAC_ADDR_LEN];
        uint32_t next_seq;
        flood_en_callbacks_t callbacks;
        flood_en_seen_entry_t seen_data[FLOOD_EN_SEEN_SIZE];
        flood_en_seen_entry_t seen_ack[FLOOD_EN_SEEN_SIZE];
        flood_en_stats_t stats;
    } flood_en_node_t;

    void flood_en_config_set_defaults(
        flood_en_config_t *config);

    flood_en_status_t flood_en_node_init(
        flood_en_node_t *node,
        const flood_en_config_t *config,
        const uint8_t self_mac[FLOOD_EN_MAC_ADDR_LEN]);

    void flood_en_node_set_callbacks(
        flood_en_node_t *node,
        const flood_en_callbacks_t *callbacks);

    void flood_en_node_tick(
        flood_en_node_t *node,
        uint32_t now_ms);

    flood_en_status_t flood_en_node_send_data(
        flood_en_node_t *node,
        const uint8_t destination_mac[FLOOD_EN_MAC_ADDR_LEN],
        const uint8_t *payload,
        uint16_t payload_len,
        bool ack_required,
        uint32_t now_ms);

    flood_en_status_t flood_en_node_on_recv(
        flood_en_node_t *node,
        const uint8_t link_src_mac[FLOOD_EN_MAC_ADDR_LEN],
        const uint8_t *frame,
        size_t frame_len,
        int8_t rssi,
        uint32_t now_ms);

#ifdef __cplusplus
}
#endif
