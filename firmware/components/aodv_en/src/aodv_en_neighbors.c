#include "aodv_en_neighbors.h"

#include <string.h>

#include "aodv_en_mac.h"

static void aodv_en_neighbor_remove_at(aodv_en_neighbor_table_t *table, uint16_t index)
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

static int8_t aodv_en_neighbor_weighted_rssi(int8_t current_avg, int8_t new_rssi)
{
    int16_t smoothed = (int16_t)((current_avg * 3) + new_rssi) / 4;
    return (int8_t)smoothed;
}

void aodv_en_neighbor_table_init(aodv_en_neighbor_table_t *table)
{
    if (table == NULL)
    {
        return;
    }

    memset(table, 0, sizeof(*table));
}

aodv_en_neighbor_entry_t *aodv_en_neighbor_find(
    aodv_en_neighbor_table_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    uint16_t index;

    if (table == NULL || aodv_en_mac_is_zero(mac))
    {
        return NULL;
    }

    for (index = 0; index < table->count; index++)
    {
        if (aodv_en_mac_equal(table->entries[index].mac, mac))
        {
            return &table->entries[index];
        }
    }

    return NULL;
}

const aodv_en_neighbor_entry_t *aodv_en_neighbor_find_const(
    const aodv_en_neighbor_table_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    return aodv_en_neighbor_find((aodv_en_neighbor_table_t *)table, mac);
}

aodv_en_status_t aodv_en_neighbor_touch(
    aodv_en_neighbor_table_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
    int8_t rssi,
    uint32_t now_ms)
{
    aodv_en_neighbor_entry_t *entry;

    if (table == NULL || aodv_en_mac_is_zero(mac))
    {
        return AODV_EN_ERR_ARG;
    }

    entry = aodv_en_neighbor_find(table, mac);
    if (entry != NULL)
    {
        entry->avg_rssi = (entry->state == AODV_EN_NEIGHBOR_ACTIVE)
                              ? aodv_en_neighbor_weighted_rssi(entry->avg_rssi, rssi)
                              : rssi;
        entry->last_rssi = rssi;
        entry->last_seen_ms = now_ms;
        entry->link_fail_count = 0;
        entry->state = AODV_EN_NEIGHBOR_ACTIVE;
        return AODV_EN_OK;
    }

    if (table->count >= AODV_EN_NEIGHBOR_TABLE_SIZE)
    {
        return AODV_EN_ERR_FULL;
    }

    entry = &table->entries[table->count++];
    memset(entry, 0, sizeof(*entry));

    aodv_en_mac_copy(entry->mac, mac);
    entry->avg_rssi = rssi;
    entry->last_rssi = rssi;
    entry->last_seen_ms = now_ms;
    entry->last_used_ms = now_ms;
    entry->state = AODV_EN_NEIGHBOR_ACTIVE;

    return AODV_EN_OK;
}

aodv_en_status_t aodv_en_neighbor_mark_used(
    aodv_en_neighbor_table_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t now_ms)
{
    aodv_en_neighbor_entry_t *entry = aodv_en_neighbor_find(table, mac);

    if (entry == NULL)
    {
        return AODV_EN_ERR_NOT_FOUND;
    }

    entry->last_used_ms = now_ms;
    return AODV_EN_OK;
}

aodv_en_status_t aodv_en_neighbor_note_link_failure(
    aodv_en_neighbor_table_t *table,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN],
    uint8_t fail_threshold)
{
    aodv_en_neighbor_entry_t *entry = aodv_en_neighbor_find(table, mac);

    if (entry == NULL)
    {
        return AODV_EN_ERR_NOT_FOUND;
    }

    if (entry->link_fail_count < UINT8_MAX)
    {
        entry->link_fail_count++;
    }

    if (entry->link_fail_count >= fail_threshold)
    {
        entry->state = AODV_EN_NEIGHBOR_INACTIVE;
    }

    return AODV_EN_OK;
}

size_t aodv_en_neighbor_expire(
    aodv_en_neighbor_table_t *table,
    uint32_t now_ms,
    uint32_t timeout_ms)
{
    size_t removed = 0;
    uint16_t index = 0;

    if (table == NULL)
    {
        return 0;
    }

    while (index < table->count)
    {
        uint32_t age_ms = now_ms - table->entries[index].last_seen_ms;
        if (age_ms > timeout_ms)
        {
            aodv_en_neighbor_remove_at(table, index);
            removed++;
            continue;
        }

        index++;
    }

    return removed;
}

size_t aodv_en_neighbor_count_active(const aodv_en_neighbor_table_t *table)
{
    size_t active_count = 0;
    uint16_t index;

    if (table == NULL)
    {
        return 0;
    }

    for (index = 0; index < table->count; index++)
    {
        if (table->entries[index].state == AODV_EN_NEIGHBOR_ACTIVE)
        {
            active_count++;
        }
    }

    return active_count;
}
