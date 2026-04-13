#include "aodv_en_peers.h"

#include <string.h>

#include "aodv_en_mac.h"

static int aodv_en_peer_find_index(
    const aodv_en_peer_cache_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    uint16_t index;

    if (table == NULL || aodv_en_mac_is_zero(mac))
    {
        return -1;
    }

    for (index = 0; index < table->count; index++)
    {
        if (aodv_en_mac_equal(table->entries[index].mac, mac))
        {
            return (int)index;
        }
    }

    return -1;
}

static int aodv_en_peer_find_lru_index(const aodv_en_peer_cache_t *table)
{
    int candidate_index = -1;
    uint16_t index;

    if (table == NULL)
    {
        return -1;
    }

    for (index = 0; index < table->count; index++)
    {
        if ((table->entries[index].flags & AODV_EN_PEER_FLAG_PINNED) != 0u)
        {
            continue;
        }

        if (candidate_index < 0 ||
            table->entries[index].last_used_ms < table->entries[(uint16_t)candidate_index].last_used_ms)
        {
            candidate_index = (int)index;
        }
    }

    return candidate_index;
}

static void aodv_en_peer_remove_at(aodv_en_peer_cache_t *table, uint16_t index)
{
    if (table == NULL || index >= table->count)
    {
        return;
    }

    if ((index + 1u) < table->count)
    {
        memmove(&table->entries[index],
                &table->entries[index + 1u],
                (size_t)(table->count - index - 1u) * sizeof(table->entries[0]));
    }

    memset(&table->entries[table->count - 1u], 0, sizeof(table->entries[0]));
    table->count--;
}

void aodv_en_peer_cache_init(aodv_en_peer_cache_t *table)
{
    if (table == NULL)
    {
        return;
    }

    memset(table, 0, sizeof(*table));
}

aodv_en_peer_cache_entry_t *aodv_en_peer_find(
    aodv_en_peer_cache_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    int index = aodv_en_peer_find_index(table, mac);

    if (index < 0)
    {
        return NULL;
    }

    return &table->entries[(uint16_t)index];
}

const aodv_en_peer_cache_entry_t *aodv_en_peer_find_const(
    const aodv_en_peer_cache_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    return aodv_en_peer_find((aodv_en_peer_cache_t *)table, mac);
}

aodv_en_status_t aodv_en_peer_touch(
    aodv_en_peer_cache_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t now_ms)
{
    aodv_en_peer_cache_entry_t *entry;
    int lru_index;

    if (table == NULL || aodv_en_mac_is_zero(mac))
    {
        return AODV_EN_ERR_ARG;
    }

    entry = aodv_en_peer_find(table, mac);
    if (entry != NULL)
    {
        entry->last_used_ms = now_ms;
        return AODV_EN_OK;
    }

    if (table->count < AODV_EN_PEER_CACHE_SIZE)
    {
        entry = &table->entries[table->count++];
        memset(entry, 0, sizeof(*entry));
        aodv_en_mac_copy(entry->mac, mac);
        entry->last_used_ms = now_ms;
        return AODV_EN_OK;
    }

    lru_index = aodv_en_peer_find_lru_index(table);
    if (lru_index < 0)
    {
        return AODV_EN_ERR_FULL;
    }

    entry = &table->entries[(uint16_t)lru_index];
    memset(entry, 0, sizeof(*entry));
    aodv_en_mac_copy(entry->mac, mac);
    entry->last_used_ms = now_ms;

    return AODV_EN_OK;
}

aodv_en_status_t aodv_en_peer_set_registered(
    aodv_en_peer_cache_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
    bool registered)
{
    aodv_en_peer_cache_entry_t *entry = aodv_en_peer_find(table, mac);

    if (entry == NULL)
    {
        return AODV_EN_ERR_NOT_FOUND;
    }

    if (registered)
    {
        entry->flags |= AODV_EN_PEER_FLAG_REGISTERED;
    }
    else
    {
        entry->flags &= (uint8_t)~AODV_EN_PEER_FLAG_REGISTERED;
    }

    return AODV_EN_OK;
}

aodv_en_status_t aodv_en_peer_set_pinned(
    aodv_en_peer_cache_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
    bool pinned)
{
    aodv_en_peer_cache_entry_t *entry = aodv_en_peer_find(table, mac);

    if (entry == NULL)
    {
        return AODV_EN_ERR_NOT_FOUND;
    }

    if (pinned)
    {
        entry->flags |= AODV_EN_PEER_FLAG_PINNED;
    }
    else
    {
        entry->flags &= (uint8_t)~AODV_EN_PEER_FLAG_PINNED;
    }

    return AODV_EN_OK;
}

aodv_en_status_t aodv_en_peer_remove(
    aodv_en_peer_cache_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    int index = aodv_en_peer_find_index(table, mac);

    if (index < 0)
    {
        return AODV_EN_ERR_NOT_FOUND;
    }

    aodv_en_peer_remove_at(table, (uint16_t)index);
    return AODV_EN_OK;
}
