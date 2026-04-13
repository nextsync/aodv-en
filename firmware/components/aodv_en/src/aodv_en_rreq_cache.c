#include "aodv_en_rreq_cache.h"

#include <string.h>

#include "aodv_en_mac.h"

static int aodv_en_rreq_cache_find_index(
    const aodv_en_rreq_cache_t *cache,
    const uint8_t originator[AODV_EN_MAC_ADDR_LEN],
    uint32_t rreq_id)
{
    uint16_t index;

    if (cache == NULL || aodv_en_mac_is_zero(originator))
    {
        return -1;
    }

    for (index = 0; index < cache->count; index++)
    {
        if (cache->entries[index].rreq_id == rreq_id &&
            aodv_en_mac_equal(cache->entries[index].originator, originator))
        {
            return (int)index;
        }
    }

    return -1;
}

static uint16_t aodv_en_rreq_cache_oldest_index(const aodv_en_rreq_cache_t *cache)
{
    uint16_t oldest_index = 0;
    uint16_t index;

    for (index = 1; index < cache->count; index++)
    {
        if (cache->entries[index].created_at_ms < cache->entries[oldest_index].created_at_ms)
        {
            oldest_index = index;
        }
    }

    return oldest_index;
}

static void aodv_en_rreq_cache_remove_at(aodv_en_rreq_cache_t *cache, uint16_t index)
{
    if (cache == NULL || index >= cache->count)
    {
        return;
    }

    if ((index + 1u) < cache->count)
    {
        memmove(&cache->entries[index],
                &cache->entries[index + 1u],
                (size_t)(cache->count - index - 1u) * sizeof(cache->entries[0]));
    }

    memset(&cache->entries[cache->count - 1u], 0, sizeof(cache->entries[0]));
    cache->count--;
}

void aodv_en_rreq_cache_init(aodv_en_rreq_cache_t *cache)
{
    if (cache == NULL)
    {
        return;
    }

    memset(cache, 0, sizeof(*cache));
}

bool aodv_en_rreq_cache_contains(
    const aodv_en_rreq_cache_t *cache,
    const uint8_t originator[AODV_EN_MAC_ADDR_LEN],
    uint32_t rreq_id)
{
    return aodv_en_rreq_cache_find_index(cache, originator, rreq_id) >= 0;
}

aodv_en_status_t aodv_en_rreq_cache_remember(
    aodv_en_rreq_cache_t *cache,
    const uint8_t originator[AODV_EN_MAC_ADDR_LEN],
    uint32_t rreq_id,
    uint8_t hop_count,
    uint32_t now_ms)
{
    aodv_en_rreq_cache_entry_t *entry;
    int existing_index;

    if (cache == NULL || aodv_en_mac_is_zero(originator))
    {
        return AODV_EN_ERR_ARG;
    }

    existing_index = aodv_en_rreq_cache_find_index(cache, originator, rreq_id);
    if (existing_index >= 0)
    {
        return AODV_EN_NOOP;
    }

    if (cache->count < AODV_EN_RREQ_CACHE_SIZE)
    {
        entry = &cache->entries[cache->count++];
    }
    else
    {
        entry = &cache->entries[aodv_en_rreq_cache_oldest_index(cache)];
    }

    memset(entry, 0, sizeof(*entry));
    aodv_en_mac_copy(entry->originator, originator);
    entry->rreq_id = rreq_id;
    entry->created_at_ms = now_ms;
    entry->hop_count = hop_count;
    entry->used = 1u;

    return AODV_EN_OK;
}

size_t aodv_en_rreq_cache_expire(
    aodv_en_rreq_cache_t *cache,
    uint32_t now_ms,
    uint32_t timeout_ms)
{
    size_t removed = 0;
    uint16_t index = 0;

    if (cache == NULL)
    {
        return 0;
    }

    while (index < cache->count)
    {
        uint32_t age_ms = now_ms - cache->entries[index].created_at_ms;
        if (age_ms > timeout_ms)
        {
            aodv_en_rreq_cache_remove_at(cache, index);
            removed++;
            continue;
        }

        index++;
    }

    return removed;
}
