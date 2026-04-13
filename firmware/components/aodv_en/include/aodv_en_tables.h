#pragma once

#include <stdint.h>

#include "aodv_en_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        uint16_t count;
        aodv_en_neighbor_entry_t entries[AODV_EN_NEIGHBOR_TABLE_SIZE];
    } aodv_en_neighbor_table_t;

    typedef struct
    {
        uint16_t count;
        aodv_en_route_entry_t entries[AODV_EN_ROUTE_TABLE_SIZE];
    } aodv_en_route_table_t;

    typedef struct
    {
        uint16_t count;
        aodv_en_rreq_cache_entry_t entries[AODV_EN_RREQ_CACHE_SIZE];
    } aodv_en_rreq_cache_t;

    typedef struct
    {
        uint16_t count;
        aodv_en_peer_cache_entry_t entries[AODV_EN_PEER_CACHE_SIZE];
    } aodv_en_peer_cache_t;

#ifdef __cplusplus
}
#endif
