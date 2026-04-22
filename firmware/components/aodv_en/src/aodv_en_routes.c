#include "aodv_en_routes.h"

#include <string.h>

#include "aodv_en_mac.h"

static void aodv_en_route_remove_at(aodv_en_route_table_t *table, uint16_t index)
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

static bool aodv_en_route_next_hop_changed(
    const aodv_en_route_entry_t *existing,
    const aodv_en_route_entry_t *candidate)
{
    if (existing == NULL || candidate == NULL)
    {
        return false;
    }

    return !aodv_en_mac_equal(existing->next_hop, candidate->next_hop);
}

static bool aodv_en_route_candidate_is_strongly_better(
    const aodv_en_route_entry_t *existing,
    const aodv_en_route_entry_t *candidate)
{
    uint32_t seq_gain = 0u;
    uint32_t metric_limit;
    uint32_t hop_limit;
    uint32_t lifetime_gain = 0u;

    if (existing == NULL || candidate == NULL)
    {
        return false;
    }

    if (candidate->dest_seq_num > existing->dest_seq_num)
    {
        seq_gain = candidate->dest_seq_num - existing->dest_seq_num;
        if (seq_gain >= (uint32_t)AODV_EN_ROUTE_SWITCH_MIN_SEQ_GAIN)
        {
            return true;
        }
    }

    metric_limit = (uint32_t)candidate->metric + (uint32_t)AODV_EN_ROUTE_SWITCH_MIN_METRIC_GAIN;
    if (metric_limit < (uint32_t)existing->metric)
    {
        return true;
    }

    hop_limit = (uint32_t)candidate->hop_count + (uint32_t)AODV_EN_ROUTE_SWITCH_MIN_HOP_GAIN;
    if (hop_limit < (uint32_t)existing->hop_count)
    {
        return true;
    }

    if (candidate->expires_at_ms > existing->expires_at_ms)
    {
        lifetime_gain = candidate->expires_at_ms - existing->expires_at_ms;
        if (lifetime_gain >= (uint32_t)AODV_EN_ROUTE_SWITCH_MIN_LIFETIME_GAIN_MS)
        {
            return true;
        }
    }

    return false;
}

void aodv_en_route_table_init(aodv_en_route_table_t *table)
{
    if (table == NULL)
    {
        return;
    }

    memset(table, 0, sizeof(*table));
}

aodv_en_route_entry_t *aodv_en_route_find(
    aodv_en_route_table_t *table,
    const uint8_t destination[AODV_EN_MAC_ADDR_LEN])
{
    uint16_t index;

    if (table == NULL || aodv_en_mac_is_zero(destination))
    {
        return NULL;
    }

    for (index = 0; index < table->count; index++)
    {
        if (aodv_en_mac_equal(table->entries[index].destination, destination))
        {
            return &table->entries[index];
        }
    }

    return NULL;
}

const aodv_en_route_entry_t *aodv_en_route_find_const(
    const aodv_en_route_table_t *table,
    const uint8_t destination[AODV_EN_MAC_ADDR_LEN])
{
    return aodv_en_route_find((aodv_en_route_table_t *)table, destination);
}

aodv_en_route_entry_t *aodv_en_route_find_valid(
    aodv_en_route_table_t *table,
    const uint8_t destination[AODV_EN_MAC_ADDR_LEN])
{
    aodv_en_route_entry_t *route = aodv_en_route_find(table, destination);

    if (route == NULL || route->state != AODV_EN_ROUTE_VALID)
    {
        return NULL;
    }

    return route;
}

bool aodv_en_route_should_replace(
    const aodv_en_route_entry_t *existing,
    const aodv_en_route_entry_t *candidate)
{
    if (candidate == NULL)
    {
        return false;
    }

    if (existing == NULL)
    {
        return true;
    }

    if (existing->state == AODV_EN_ROUTE_INVALID && candidate->state != AODV_EN_ROUTE_INVALID)
    {
        return true;
    }

    if (candidate->state == AODV_EN_ROUTE_VALID && existing->state != AODV_EN_ROUTE_VALID)
    {
        return true;
    }

    /* Hysteresis: avoid flapping between next hops unless the candidate is clearly better. */
    if (candidate->state == AODV_EN_ROUTE_VALID &&
        existing->state == AODV_EN_ROUTE_VALID &&
        aodv_en_route_next_hop_changed(existing, candidate) &&
        !aodv_en_route_candidate_is_strongly_better(existing, candidate))
    {
        return false;
    }

    if (candidate->dest_seq_num > existing->dest_seq_num)
    {
        return true;
    }

    if (candidate->dest_seq_num < existing->dest_seq_num)
    {
        return false;
    }

    if (candidate->metric < existing->metric)
    {
        return true;
    }

    if (candidate->metric > existing->metric)
    {
        return false;
    }

    if (candidate->hop_count < existing->hop_count)
    {
        return true;
    }

    if (candidate->hop_count > existing->hop_count)
    {
        return false;
    }

    return candidate->expires_at_ms > existing->expires_at_ms;
}

aodv_en_status_t aodv_en_route_upsert(
    aodv_en_route_table_t *table,
    const aodv_en_route_entry_t *candidate)
{
    aodv_en_route_entry_t *existing;

    if (table == NULL || candidate == NULL ||
        aodv_en_mac_is_zero(candidate->destination) ||
        aodv_en_mac_is_zero(candidate->next_hop))
    {
        return AODV_EN_ERR_ARG;
    }

    existing = aodv_en_route_find(table, candidate->destination);
    if (existing != NULL)
    {
        if (!aodv_en_route_should_replace(existing, candidate))
        {
            return AODV_EN_NOOP;
        }

        /* Preserve precursors if the next hop is the same (route refresh) */
        if (aodv_en_mac_equal(existing->next_hop, candidate->next_hop))
        {
            uint8_t saved_precursor_count = existing->precursor_count;
            uint8_t saved_precursors[AODV_EN_MAX_PRECURSORS][AODV_EN_MAC_ADDR_LEN];
            memcpy(saved_precursors, existing->precursors, sizeof(saved_precursors));

            *existing = *candidate;

            existing->precursor_count = saved_precursor_count;
            memcpy(existing->precursors, saved_precursors, sizeof(saved_precursors));
        }
        else
        {
            /* New next hop, reset precursors */
            *existing = *candidate;
        }
        return AODV_EN_OK;
    }

    if (table->count >= AODV_EN_ROUTE_TABLE_SIZE)
    {
        return AODV_EN_ERR_FULL;
    }

    table->entries[table->count++] = *candidate;
    return AODV_EN_OK;
}

aodv_en_status_t aodv_en_route_add_precursor(
    aodv_en_route_entry_t *route,
    const uint8_t precursor[AODV_EN_MAC_ADDR_LEN])
{
    uint8_t index;

    if (route == NULL || aodv_en_mac_is_zero(precursor))
    {
        return AODV_EN_ERR_ARG;
    }

    for (index = 0; index < route->precursor_count; index++)
    {
        if (aodv_en_mac_equal(route->precursors[index], precursor))
        {
            return AODV_EN_OK;
        }
    }

    if (route->precursor_count < AODV_EN_MAX_PRECURSORS)
    {
        aodv_en_mac_copy(route->precursors[route->precursor_count++], precursor);
        return AODV_EN_OK;
    }

    return AODV_EN_ERR_FULL;
}

aodv_en_status_t aodv_en_route_invalidate_destination(
    aodv_en_route_table_t *table,
    const uint8_t destination[AODV_EN_MAC_ADDR_LEN],
    uint32_t now_ms)
{
    aodv_en_route_entry_t *route = aodv_en_route_find(table, destination);

    if (route == NULL)
    {
        return AODV_EN_ERR_NOT_FOUND;
    }

    route->state = AODV_EN_ROUTE_INVALID;
    route->metric = AODV_EN_ROUTE_METRIC_INFINITY;
    route->expires_at_ms = now_ms;

    return AODV_EN_OK;
}

size_t aodv_en_route_invalidate_by_next_hop(
    aodv_en_route_table_t *table,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    uint32_t now_ms)
{
    size_t invalidated = 0;
    uint16_t index;

    if (table == NULL || aodv_en_mac_is_zero(next_hop))
    {
        return 0;
    }

    for (index = 0; index < table->count; index++)
    {
        if (aodv_en_mac_equal(table->entries[index].next_hop, next_hop))
        {
            table->entries[index].state = AODV_EN_ROUTE_INVALID;
            table->entries[index].metric = AODV_EN_ROUTE_METRIC_INFINITY;
            table->entries[index].expires_at_ms = now_ms;
            invalidated++;
        }
    }

    return invalidated;
}

size_t aodv_en_route_expire(
    aodv_en_route_table_t *table,
    uint32_t now_ms)
{
    size_t removed = 0;
    uint16_t index = 0;

    if (table == NULL)
    {
        return 0;
    }

    while (index < table->count)
    {
        if (table->entries[index].expires_at_ms <= now_ms)
        {
            aodv_en_route_remove_at(table, index);
            removed++;
            continue;
        }

        index++;
    }

    return removed;
}
