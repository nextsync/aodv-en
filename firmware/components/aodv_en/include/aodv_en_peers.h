#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "aodv_en_status.h"
#include "aodv_en_tables.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void aodv_en_peer_cache_init(aodv_en_peer_cache_t *table);

    aodv_en_peer_cache_entry_t *aodv_en_peer_find(
        aodv_en_peer_cache_t *table,
        const uint8_t mac[AODV_EN_MAC_ADDR_LEN]);

    const aodv_en_peer_cache_entry_t *aodv_en_peer_find_const(
        const aodv_en_peer_cache_t *table,
        const uint8_t mac[AODV_EN_MAC_ADDR_LEN]);

    aodv_en_status_t aodv_en_peer_touch(
        aodv_en_peer_cache_t *table,
        const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
        uint32_t now_ms);

    aodv_en_status_t aodv_en_peer_set_registered(
        aodv_en_peer_cache_t *table,
        const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
        bool registered);

    aodv_en_status_t aodv_en_peer_set_pinned(
        aodv_en_peer_cache_t *table,
        const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
        bool pinned);

    aodv_en_status_t aodv_en_peer_remove(
        aodv_en_peer_cache_t *table,
        const uint8_t mac[AODV_EN_MAC_ADDR_LEN]);

#ifdef __cplusplus
}
#endif
