#pragma once

#include <stdint.h>

#include "aodv_en_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        AODV_EN_MSG_FLAG_NONE = 0x00,
        AODV_EN_MSG_FLAG_ACK_REQUIRED = 0x01,
        AODV_EN_MSG_FLAG_ROUTE_REPAIR = 0x02,
    };

    typedef struct __attribute__((packed))
    {
        uint8_t protocol_version;
        uint8_t message_type;
        uint8_t flags;
        uint8_t hop_count;
        uint32_t network_id;
        uint8_t sender_mac[AODV_EN_MAC_ADDR_LEN];
    } aodv_en_header_t;

    typedef struct __attribute__((packed))
    {
        aodv_en_header_t header;
        uint8_t node_mac[AODV_EN_MAC_ADDR_LEN];
        uint32_t node_seq_num;
        uint32_t timestamp_ms;
    } aodv_en_hello_msg_t;

    typedef struct __attribute__((packed))
    {
        aodv_en_header_t header;
        uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN];
        uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN];
        uint32_t originator_seq_num;
        uint32_t destination_seq_num;
        uint32_t rreq_id;
        uint8_t ttl;
    } aodv_en_rreq_msg_t;

    typedef struct __attribute__((packed))
    {
        aodv_en_header_t header;
        uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN];
        uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN];
        uint32_t destination_seq_num;
        uint32_t lifetime_ms;
    } aodv_en_rrep_msg_t;

    typedef struct __attribute__((packed))
    {
        aodv_en_header_t header;
        uint8_t unreachable_destination_mac[AODV_EN_MAC_ADDR_LEN];
        uint32_t unreachable_dest_seq_num;
    } aodv_en_rerr_msg_t;

    typedef struct __attribute__((packed))
    {
        aodv_en_header_t header;
        uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN];
        uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN];
        uint32_t ack_for_sequence;
    } aodv_en_ack_msg_t;

    typedef struct __attribute__((packed))
    {
        aodv_en_header_t header;
        uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN];
        uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN];
        uint32_t sequence_number;
        uint8_t ttl;
        uint16_t payload_length;
        uint8_t payload[];
    } aodv_en_data_msg_t;

#ifdef __cplusplus
}
#endif
