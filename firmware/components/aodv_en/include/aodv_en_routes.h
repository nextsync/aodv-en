#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "aodv_en_status.h"
#include "aodv_en_tables.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void aodv_en_route_table_init(aodv_en_route_table_t *table);

    aodv_en_route_entry_t *aodv_en_route_find(
        aodv_en_route_table_t *table,
        const uint8_t destination[AODV_EN_MAC_ADDR_LEN]);

    const aodv_en_route_entry_t *aodv_en_route_find_const(
        const aodv_en_route_table_t *table,
        const uint8_t destination[AODV_EN_MAC_ADDR_LEN]);

    aodv_en_route_entry_t *aodv_en_route_find_valid(
        aodv_en_route_table_t *table,
        const uint8_t destination[AODV_EN_MAC_ADDR_LEN]);

    bool aodv_en_route_should_replace(
        const aodv_en_route_entry_t *existing,
        const aodv_en_route_entry_t *candidate);

    aodv_en_status_t aodv_en_route_upsert(
        aodv_en_route_table_t *table,
        const aodv_en_route_entry_t *candidate);

    aodv_en_status_t aodv_en_route_invalidate_destination(
        aodv_en_route_table_t *table,
        const uint8_t destination[AODV_EN_MAC_ADDR_LEN],
        uint32_t now_ms);

    size_t aodv_en_route_invalidate_by_next_hop(
        aodv_en_route_table_t *table,
        const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
        uint32_t now_ms);

    size_t aodv_en_route_expire(
        aodv_en_route_table_t *table,
        uint32_t now_ms);

#ifdef __cplusplus
}
#endif
