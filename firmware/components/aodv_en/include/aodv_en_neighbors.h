#pragma once

#include <stddef.h>
#include <stdint.h>

#include "aodv_en_status.h"
#include "aodv_en_tables.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void aodv_en_neighbor_table_init(aodv_en_neighbor_table_t *table);

    aodv_en_neighbor_entry_t *aodv_en_neighbor_find(
        aodv_en_neighbor_table_t *table,
        const uint8_t mac[AODV_EN_MAC_ADDR_LEN]);

    const aodv_en_neighbor_entry_t *aodv_en_neighbor_find_const(
        const aodv_en_neighbor_table_t *table,
        const uint8_t mac[AODV_EN_MAC_ADDR_LEN]);

    aodv_en_status_t aodv_en_neighbor_touch(
        aodv_en_neighbor_table_t *table,
        const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
        int8_t rssi,
        uint32_t now_ms);

    aodv_en_status_t aodv_en_neighbor_mark_used(
        aodv_en_neighbor_table_t *table,
        const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
        uint32_t now_ms);

    aodv_en_status_t aodv_en_neighbor_note_link_failure(
        aodv_en_neighbor_table_t *table,
        const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
        uint8_t fail_threshold);

    size_t aodv_en_neighbor_expire(
        aodv_en_neighbor_table_t *table,
        uint32_t now_ms,
        uint32_t timeout_ms);

    size_t aodv_en_neighbor_count_active(
        const aodv_en_neighbor_table_t *table);

#ifdef __cplusplus
}
#endif
