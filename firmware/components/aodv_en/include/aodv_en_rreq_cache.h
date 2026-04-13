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

    void aodv_en_rreq_cache_init(aodv_en_rreq_cache_t *cache);

    bool aodv_en_rreq_cache_contains(
        const aodv_en_rreq_cache_t *cache,
        const uint8_t originator[AODV_EN_MAC_ADDR_LEN],
        uint32_t rreq_id);

    aodv_en_status_t aodv_en_rreq_cache_remember(
        aodv_en_rreq_cache_t *cache,
        const uint8_t originator[AODV_EN_MAC_ADDR_LEN],
        uint32_t rreq_id,
        uint8_t hop_count,
        uint32_t now_ms);

    size_t aodv_en_rreq_cache_expire(
        aodv_en_rreq_cache_t *cache,
        uint32_t now_ms,
        uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
