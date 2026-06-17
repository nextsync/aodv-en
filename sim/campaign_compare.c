#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aodv_en_messages.h"
#include "aodv_en_node.h"

#define SIM_MAX_NODES 16u
#define SIM_MAX_PACKETS 8192u
#define SIM_FLOOD_SEEN_CAP 512u
#define SIM_PAYLOAD_BYTES 24u

#define SIM_DEFAULT_DURATION_MS 120000u
#define SIM_DEFAULT_TICK_MS 100u
#define SIM_DEFAULT_SEND_INTERVAL_MS 1200u

#define SIM_TX_ENERGY_MJ 1.20
#define SIM_RX_ENERGY_MJ 0.80

typedef enum
{
    SIM_PROTOCOL_AODV = 0,
    SIM_PROTOCOL_FLOODING = 1,
} sim_protocol_t;

typedef enum
{
    SIM_TOPOLOGY_LINEAR = 0,
    SIM_TOPOLOGY_TREE = 1,
    SIM_TOPOLOGY_PARTIAL_MESH = 2,
} sim_topology_t;

typedef struct
{
    const char *name;
    sim_topology_t topology;
    size_t node_count;
    size_t source_index;
    size_t destination_index;
    bool induce_failure;
    size_t fail_u;
    size_t fail_v;
    uint32_t failure_start_ms;
    uint32_t failure_end_ms;
    uint8_t flood_ttl;
} sim_scenario_t;

typedef struct
{
    const char *name;
    uint8_t hop_weight;
    uint8_t rssi_weight;
    int8_t rssi_best_dbm;
    int8_t rssi_worst_dbm;
} sim_aodv_profile_t;

typedef struct
{
    uint32_t seq;
    uint32_t sent_at_ms;
    bool delivered;
} sim_packet_record_t;

typedef struct
{
    bool used;
    uint8_t origin_mac[AODV_EN_MAC_ADDR_LEN];
    uint32_t seq;
} sim_flood_seen_entry_t;

typedef struct
{
    sim_flood_seen_entry_t entries[SIM_FLOOD_SEEN_CAP];
    uint16_t overwrite_cursor;
} sim_flood_node_t;

typedef struct __attribute__((packed))
{
    uint8_t marker;
    uint8_t ttl;
    uint8_t reserved[2];
    uint8_t origin_mac[AODV_EN_MAC_ADDR_LEN];
    uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN];
    uint32_t sequence;
    uint16_t payload_len;
    uint8_t payload[SIM_PAYLOAD_BYTES];
} sim_flood_frame_t;

typedef struct sim_context sim_context_t;

typedef struct
{
    sim_context_t *ctx;
    size_t index;
} sim_aodv_endpoint_t;

struct sim_context
{
    sim_protocol_t protocol;
    const sim_scenario_t *scenario;
    const sim_aodv_profile_t *profile;
    uint32_t rng_state;
    uint32_t now_ms;
    uint32_t duration_ms;
    uint32_t tick_ms;
    uint32_t send_interval_ms;

    uint8_t macs[SIM_MAX_NODES][AODV_EN_MAC_ADDR_LEN];
    bool base_link[SIM_MAX_NODES][SIM_MAX_NODES];
    int8_t base_rssi_dbm[SIM_MAX_NODES][SIM_MAX_NODES];
    uint8_t base_loss_pct[SIM_MAX_NODES][SIM_MAX_NODES];

    aodv_en_node_t aodv_nodes[SIM_MAX_NODES];
    sim_aodv_endpoint_t aodv_endpoints[SIM_MAX_NODES];

    sim_flood_node_t flood_nodes[SIM_MAX_NODES];

    sim_packet_record_t packet_records[SIM_MAX_PACKETS];
    size_t packet_count;

    uint32_t offered_packets;
    uint32_t delivered_packets;
    uint64_t latency_sum_ms;

    uint32_t tx_total;
    uint32_t rx_total;
    uint32_t tx_control;
    uint32_t tx_data;
    uint32_t tx_failures;
    uint32_t duplicate_drops_flood;
};

typedef struct
{
    const char *scenario_name;
    const char *protocol_name;
    const char *profile_name;
    uint32_t seed;
    uint32_t duration_ms;
    uint32_t offered;
    uint32_t delivered;
    double pdr;
    double latency_avg_ms;
    double nrl;
    double nrl_control;
    double energy_est_mj;
    uint32_t tx_total;
    uint32_t rx_total;
    uint32_t tx_control;
    uint32_t tx_data;
    uint32_t tx_failures;
    uint32_t route_discoveries;
    uint32_t route_repairs;
    uint32_t duplicate_rreq_drops;
    uint32_t flood_duplicate_drops;
} sim_result_t;

static const sim_scenario_t SIM_SCENARIOS[] = {
    {
        .name = "linear_stable",
        .topology = SIM_TOPOLOGY_LINEAR,
        .node_count = 10u,
        .source_index = 0u,
        .destination_index = 9u,
        .induce_failure = false,
        .fail_u = 0u,
        .fail_v = 0u,
        .failure_start_ms = 0u,
        .failure_end_ms = 0u,
        .flood_ttl = 10u,
    },
    {
        .name = "tree_stable",
        .topology = SIM_TOPOLOGY_TREE,
        .node_count = 10u,
        .source_index = 0u,
        .destination_index = 9u,
        .induce_failure = false,
        .fail_u = 0u,
        .fail_v = 0u,
        .failure_start_ms = 0u,
        .failure_end_ms = 0u,
        .flood_ttl = 10u,
    },
    {
        .name = "partial_mesh_stable",
        .topology = SIM_TOPOLOGY_PARTIAL_MESH,
        .node_count = 10u,
        .source_index = 0u,
        .destination_index = 9u,
        .induce_failure = false,
        .fail_u = 0u,
        .fail_v = 0u,
        .failure_start_ms = 0u,
        .failure_end_ms = 0u,
        .flood_ttl = 10u,
    },
    {
        .name = "partial_mesh_failure",
        .topology = SIM_TOPOLOGY_PARTIAL_MESH,
        .node_count = 10u,
        .source_index = 0u,
        .destination_index = 9u,
        .induce_failure = true,
        .fail_u = 4u,
        .fail_v = 6u,
        .failure_start_ms = 30000u,
        .failure_end_ms = 70000u,
        .flood_ttl = 10u,
    },
};

static const sim_aodv_profile_t SIM_AODV_PROFILES[] = {
    {
        .name = "hop_only",
        .hop_weight = 8u,
        .rssi_weight = 0u,
        .rssi_best_dbm = -55,
        .rssi_worst_dbm = -90,
    },
    {
        .name = "hybrid_default",
        .hop_weight = 8u,
        .rssi_weight = 1u,
        .rssi_best_dbm = -55,
        .rssi_worst_dbm = -90,
    },
    {
        .name = "hybrid_rssi_bias",
        .hop_weight = 6u,
        .rssi_weight = 2u,
        .rssi_best_dbm = -55,
        .rssi_worst_dbm = -90,
    },
};

static uint32_t sim_rng_next(sim_context_t *ctx)
{
    uint32_t x = ctx->rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    ctx->rng_state = x;
    return x;
}

static uint32_t sim_rng_percent(sim_context_t *ctx)
{
    return sim_rng_next(ctx) % 100u;
}

static int32_t sim_rng_signed(sim_context_t *ctx, int32_t min_value, int32_t max_value)
{
    uint32_t range;
    if (max_value <= min_value)
    {
        return min_value;
    }

    range = (uint32_t)(max_value - min_value + 1);
    return min_value + (int32_t)(sim_rng_next(ctx) % range);
}

static void sim_set_link(
    sim_context_t *ctx,
    size_t a,
    size_t b,
    int8_t rssi_dbm,
    uint8_t loss_pct)
{
    if (ctx == NULL || a >= SIM_MAX_NODES || b >= SIM_MAX_NODES || a == b)
    {
        return;
    }

    ctx->base_link[a][b] = true;
    ctx->base_link[b][a] = true;
    ctx->base_rssi_dbm[a][b] = rssi_dbm;
    ctx->base_rssi_dbm[b][a] = rssi_dbm;
    ctx->base_loss_pct[a][b] = loss_pct;
    ctx->base_loss_pct[b][a] = loss_pct;
}

static bool sim_link_is_active(const sim_context_t *ctx, size_t src, size_t dst)
{
    if (ctx == NULL || src >= ctx->scenario->node_count || dst >= ctx->scenario->node_count)
    {
        return false;
    }

    if (!ctx->base_link[src][dst])
    {
        return false;
    }

    if (!ctx->scenario->induce_failure)
    {
        return true;
    }

    if (ctx->now_ms < ctx->scenario->failure_start_ms ||
        ctx->now_ms > ctx->scenario->failure_end_ms)
    {
        return true;
    }

    if ((src == ctx->scenario->fail_u && dst == ctx->scenario->fail_v) ||
        (src == ctx->scenario->fail_v && dst == ctx->scenario->fail_u))
    {
        return false;
    }

    return true;
}

static bool sim_tx_success(
    sim_context_t *ctx,
    size_t src,
    size_t dst,
    bool broadcast)
{
    uint8_t loss_pct;

    if (!sim_link_is_active(ctx, src, dst))
    {
        return false;
    }

    loss_pct = ctx->base_loss_pct[src][dst];
    if (broadcast)
    {
        loss_pct = (uint8_t)((loss_pct + 8u > 95u) ? 95u : (loss_pct + 8u));
    }

    return sim_rng_percent(ctx) >= (uint32_t)loss_pct;
}

static int8_t sim_sample_rssi(sim_context_t *ctx, size_t src, size_t dst)
{
    int32_t jitter = sim_rng_signed(ctx, -3, 3);
    int32_t value = (int32_t)ctx->base_rssi_dbm[src][dst] + jitter;
    if (value > -25)
    {
        value = -25;
    }
    if (value < -100)
    {
        value = -100;
    }
    return (int8_t)value;
}

static int sim_find_index_by_mac(
    const sim_context_t *ctx,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    size_t index;

    if (ctx == NULL || mac == NULL)
    {
        return -1;
    }

    for (index = 0; index < ctx->scenario->node_count; index++)
    {
        if (memcmp(ctx->macs[index], mac, AODV_EN_MAC_ADDR_LEN) == 0)
        {
            return (int)index;
        }
    }

    return -1;
}

static void sim_record_packet_send(sim_context_t *ctx, uint32_t seq, uint32_t now_ms)
{
    sim_packet_record_t *entry;
    size_t index;

    if (ctx == NULL)
    {
        return;
    }

    for (index = 0; index < ctx->packet_count; index++)
    {
        if (ctx->packet_records[index].seq == seq)
        {
            ctx->packet_records[index].sent_at_ms = now_ms;
            ctx->packet_records[index].delivered = false;
            return;
        }
    }

    if (ctx->packet_count >= SIM_MAX_PACKETS)
    {
        return;
    }

    entry = &ctx->packet_records[ctx->packet_count++];
    memset(entry, 0, sizeof(*entry));
    entry->seq = seq;
    entry->sent_at_ms = now_ms;
}

static void sim_record_packet_delivery(sim_context_t *ctx, uint32_t seq, uint32_t now_ms)
{
    size_t index;

    if (ctx == NULL)
    {
        return;
    }

    for (index = 0; index < ctx->packet_count; index++)
    {
        sim_packet_record_t *entry = &ctx->packet_records[index];
        uint32_t latency_ms;
        if (entry->seq != seq)
        {
            continue;
        }
        if (entry->delivered)
        {
            return;
        }

        entry->delivered = true;
        ctx->delivered_packets++;
        latency_ms = (now_ms >= entry->sent_at_ms) ? (now_ms - entry->sent_at_ms + 1u) : 1u;
        ctx->latency_sum_ms += (uint64_t)latency_ms;
        return;
    }
}

static void sim_build_topology(sim_context_t *ctx)
{
    size_t index;
    size_t node_count = ctx->scenario->node_count;

    memset(ctx->base_link, 0, sizeof(ctx->base_link));
    memset(ctx->base_rssi_dbm, 0, sizeof(ctx->base_rssi_dbm));
    memset(ctx->base_loss_pct, 0, sizeof(ctx->base_loss_pct));

    for (index = 0; index < node_count; index++)
    {
        memset(ctx->macs[index], 0, AODV_EN_MAC_ADDR_LEN);
        ctx->macs[index][0] = 0x42;
        ctx->macs[index][5] = (uint8_t)(index + 1u);
    }

    if (ctx->scenario->topology == SIM_TOPOLOGY_LINEAR)
    {
        for (index = 0; (index + 1u) < node_count; index++)
        {
            int8_t rssi = (int8_t)(-61 - (int8_t)(index % 3u) * 2);
            uint8_t loss = (uint8_t)(8u + (uint8_t)(index % 3u) * 2u);
            sim_set_link(ctx, index, index + 1u, rssi, loss);
        }
        return;
    }

    if (ctx->scenario->topology == SIM_TOPOLOGY_TREE)
    {
        sim_set_link(ctx, 0u, 1u, -58, 5u);
        sim_set_link(ctx, 0u, 2u, -60, 6u);
        sim_set_link(ctx, 1u, 3u, -64, 8u);
        sim_set_link(ctx, 1u, 4u, -65, 10u);
        sim_set_link(ctx, 2u, 5u, -66, 10u);
        sim_set_link(ctx, 2u, 6u, -67, 12u);
        sim_set_link(ctx, 4u, 7u, -70, 14u);
        sim_set_link(ctx, 5u, 8u, -72, 16u);
        sim_set_link(ctx, 6u, 9u, -73, 18u);
        return;
    }

    if (ctx->scenario->topology == SIM_TOPOLOGY_PARTIAL_MESH)
    {
        for (index = 0; (index + 1u) < node_count; index++)
        {
            sim_set_link(ctx, index, index + 1u, -64, 10u);
        }

        sim_set_link(ctx, 0u, 2u, -70, 15u);
        sim_set_link(ctx, 1u, 3u, -68, 12u);
        sim_set_link(ctx, 2u, 4u, -67, 11u);
        sim_set_link(ctx, 3u, 5u, -66, 10u);
        sim_set_link(ctx, 4u, 6u, -65, 9u);
        sim_set_link(ctx, 5u, 7u, -67, 10u);
        sim_set_link(ctx, 6u, 8u, -69, 12u);
        sim_set_link(ctx, 7u, 9u, -70, 14u);
        sim_set_link(ctx, 1u, 4u, -74, 20u);
        sim_set_link(ctx, 2u, 5u, -72, 18u);
        sim_set_link(ctx, 3u, 6u, -71, 16u);
        sim_set_link(ctx, 4u, 7u, -70, 14u);
        sim_set_link(ctx, 5u, 9u, -73, 19u);
    }
}

static void sim_flood_seen_remember(
    sim_flood_node_t *node,
    const uint8_t origin_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t seq)
{
    uint16_t index;

    if (node == NULL || origin_mac == NULL)
    {
        return;
    }

    for (index = 0; index < SIM_FLOOD_SEEN_CAP; index++)
    {
        if (!node->entries[index].used)
        {
            node->entries[index].used = true;
            memcpy(node->entries[index].origin_mac, origin_mac, AODV_EN_MAC_ADDR_LEN);
            node->entries[index].seq = seq;
            return;
        }
    }

    index = node->overwrite_cursor++ % SIM_FLOOD_SEEN_CAP;
    node->entries[index].used = true;
    memcpy(node->entries[index].origin_mac, origin_mac, AODV_EN_MAC_ADDR_LEN);
    node->entries[index].seq = seq;
}

static bool sim_flood_seen_contains(
    const sim_flood_node_t *node,
    const uint8_t origin_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t seq)
{
    uint16_t index;

    if (node == NULL || origin_mac == NULL)
    {
        return false;
    }

    for (index = 0; index < SIM_FLOOD_SEEN_CAP; index++)
    {
        if (!node->entries[index].used)
        {
            continue;
        }
        if (node->entries[index].seq == seq &&
            memcmp(node->entries[index].origin_mac, origin_mac, AODV_EN_MAC_ADDR_LEN) == 0)
        {
            return true;
        }
    }

    return false;
}

static void sim_flood_on_recv(
    sim_context_t *ctx,
    size_t receiver_index,
    const sim_flood_frame_t *frame);

static void sim_flood_emit_broadcast(
    sim_context_t *ctx,
    size_t sender_index,
    const sim_flood_frame_t *frame)
{
    size_t destination_index;

    if (ctx == NULL || frame == NULL)
    {
        return;
    }

    ctx->tx_total++;
    ctx->tx_data++;

    for (destination_index = 0; destination_index < ctx->scenario->node_count; destination_index++)
    {
        if (destination_index == sender_index)
        {
            continue;
        }
        if (!sim_tx_success(ctx, sender_index, destination_index, true))
        {
            continue;
        }

        ctx->rx_total++;
        sim_flood_on_recv(ctx, destination_index, frame);
    }
}

static void sim_flood_on_recv(
    sim_context_t *ctx,
    size_t receiver_index,
    const sim_flood_frame_t *frame)
{
    sim_flood_node_t *receiver;
    sim_flood_frame_t forward;
    uint32_t seq;

    if (ctx == NULL || frame == NULL || receiver_index >= ctx->scenario->node_count)
    {
        return;
    }

    if (frame->marker != 0xF1u || frame->payload_len > SIM_PAYLOAD_BYTES)
    {
        return;
    }

    receiver = &ctx->flood_nodes[receiver_index];
    if (sim_flood_seen_contains(receiver, frame->origin_mac, frame->sequence))
    {
        ctx->duplicate_drops_flood++;
        return;
    }

    sim_flood_seen_remember(receiver, frame->origin_mac, frame->sequence);

    if (memcmp(ctx->macs[receiver_index], frame->destination_mac, AODV_EN_MAC_ADDR_LEN) == 0)
    {
        if (frame->payload_len >= sizeof(uint32_t))
        {
            memcpy(&seq, frame->payload, sizeof(uint32_t));
            sim_record_packet_delivery(ctx, seq, ctx->now_ms);
        }
    }

    if (frame->ttl <= 1u)
    {
        return;
    }

    memcpy(&forward, frame, sizeof(forward));
    forward.ttl = (uint8_t)(frame->ttl - 1u);
    sim_flood_emit_broadcast(ctx, receiver_index, &forward);
}

static void sim_flood_send_data(sim_context_t *ctx, uint32_t sequence)
{
    sim_flood_frame_t frame;
    size_t source_index;

    if (ctx == NULL)
    {
        return;
    }

    source_index = ctx->scenario->source_index;

    memset(&frame, 0, sizeof(frame));
    frame.marker = 0xF1u;
    frame.ttl = ctx->scenario->flood_ttl;
    memcpy(frame.origin_mac, ctx->macs[source_index], AODV_EN_MAC_ADDR_LEN);
    memcpy(frame.destination_mac, ctx->macs[ctx->scenario->destination_index], AODV_EN_MAC_ADDR_LEN);
    frame.sequence = sequence;
    frame.payload_len = sizeof(uint32_t);
    memcpy(frame.payload, &sequence, sizeof(uint32_t));

    sim_flood_seen_remember(&ctx->flood_nodes[source_index], frame.origin_mac, sequence);
    sim_flood_emit_broadcast(ctx, source_index, &frame);
}

static aodv_en_status_t sim_aodv_emit_frame(
    void *user_ctx,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    bool broadcast)
{
    const aodv_en_header_t *header = (const aodv_en_header_t *)frame;
    sim_aodv_endpoint_t *endpoint = (sim_aodv_endpoint_t *)user_ctx;
    sim_context_t *ctx = endpoint->ctx;
    size_t source_index = endpoint->index;

    if (ctx == NULL || frame == NULL || frame_len < sizeof(aodv_en_header_t))
    {
        return AODV_EN_ERR_ARG;
    }

    ctx->tx_total++;
    if (header->message_type == AODV_EN_MSG_DATA)
    {
        ctx->tx_data++;
    }
    else
    {
        ctx->tx_control++;
    }

    if (broadcast)
    {
        size_t destination_index;
        for (destination_index = 0; destination_index < ctx->scenario->node_count; destination_index++)
        {
            int8_t rssi;
            if (destination_index == source_index)
            {
                continue;
            }
            if (!sim_tx_success(ctx, source_index, destination_index, true))
            {
                continue;
            }
            ctx->rx_total++;
            rssi = sim_sample_rssi(ctx, source_index, destination_index);
            (void)aodv_en_node_on_recv(
                &ctx->aodv_nodes[destination_index],
                ctx->macs[source_index],
                frame,
                frame_len,
                rssi,
                ctx->now_ms);
        }
        return AODV_EN_OK;
    }

    {
        int destination_index = sim_find_index_by_mac(ctx, next_hop);
        size_t invalidated_routes = 0u;
        int8_t rssi;

        if (destination_index < 0 ||
            !sim_link_is_active(ctx, source_index, (size_t)destination_index) ||
            !sim_tx_success(ctx, source_index, (size_t)destination_index, false))
        {
            ctx->tx_failures++;
            (void)aodv_en_node_on_link_tx_result(
                &ctx->aodv_nodes[source_index],
                next_hop,
                false,
                ctx->now_ms,
                &invalidated_routes);
            return AODV_EN_ERR_STATE;
        }

        (void)aodv_en_node_on_link_tx_result(
            &ctx->aodv_nodes[source_index],
            next_hop,
            true,
            ctx->now_ms,
            &invalidated_routes);

        ctx->rx_total++;
        rssi = sim_sample_rssi(ctx, source_index, (size_t)destination_index);
        (void)aodv_en_node_on_recv(
            &ctx->aodv_nodes[(size_t)destination_index],
            ctx->macs[source_index],
            frame,
            frame_len,
            rssi,
            ctx->now_ms);
    }

    return AODV_EN_OK;
}

static void sim_aodv_deliver_data(
    void *user_ctx,
    const uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len)
{
    sim_aodv_endpoint_t *endpoint = (sim_aodv_endpoint_t *)user_ctx;
    sim_context_t *ctx = endpoint->ctx;
    uint32_t sequence = 0u;

    (void)originator_mac;
    if (ctx == NULL || payload == NULL || payload_len < sizeof(uint32_t))
    {
        return;
    }

    memcpy(&sequence, payload, sizeof(uint32_t));
    sim_record_packet_delivery(ctx, sequence, ctx->now_ms);
}

static void sim_aodv_ack_received(
    void *user_ctx,
    const uint8_t ack_sender_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t sequence_number)
{
    (void)user_ctx;
    (void)ack_sender_mac;
    (void)sequence_number;
}

static void sim_context_prepare(
    sim_context_t *ctx,
    sim_protocol_t protocol,
    const sim_scenario_t *scenario,
    const sim_aodv_profile_t *profile,
    uint32_t seed)
{
    size_t index;

    memset(ctx, 0, sizeof(*ctx));
    ctx->protocol = protocol;
    ctx->scenario = scenario;
    ctx->profile = profile;
    ctx->rng_state = (seed == 0u) ? 0xA5A5A5A5u : seed;
    ctx->duration_ms = SIM_DEFAULT_DURATION_MS;
    ctx->tick_ms = SIM_DEFAULT_TICK_MS;
    ctx->send_interval_ms = SIM_DEFAULT_SEND_INTERVAL_MS;

    sim_build_topology(ctx);

    if (protocol == SIM_PROTOCOL_AODV)
    {
        aodv_en_config_t config;

        aodv_en_config_set_defaults(&config);
        config.network_id = 0xA0DE2026u;
        config.max_hops = (uint8_t)(scenario->node_count + 2u);
        config.ttl_default = (uint8_t)(scenario->node_count + 2u);
        config.route_metric_hop_weight = profile->hop_weight;
        config.route_metric_rssi_weight = profile->rssi_weight;
        config.route_metric_rssi_best_dbm = profile->rssi_best_dbm;
        config.route_metric_rssi_worst_dbm = profile->rssi_worst_dbm;

        for (index = 0; index < scenario->node_count; index++)
        {
            aodv_en_node_callbacks_t callbacks;

            (void)aodv_en_node_init(&ctx->aodv_nodes[index], &config, ctx->macs[index]);

            ctx->aodv_endpoints[index].ctx = ctx;
            ctx->aodv_endpoints[index].index = index;

            memset(&callbacks, 0, sizeof(callbacks));
            callbacks.emit_frame = sim_aodv_emit_frame;
            callbacks.deliver_data = sim_aodv_deliver_data;
            callbacks.ack_received = sim_aodv_ack_received;
            callbacks.user_ctx = &ctx->aodv_endpoints[index];
            aodv_en_node_set_callbacks(&ctx->aodv_nodes[index], &callbacks);
        }
    }
}

static void sim_offer_packet(sim_context_t *ctx, uint32_t sequence)
{
    uint8_t payload[SIM_PAYLOAD_BYTES];
    aodv_en_status_t status;

    memset(payload, 0, sizeof(payload));
    memcpy(payload, &sequence, sizeof(sequence));

    ctx->offered_packets++;
    sim_record_packet_send(ctx, sequence, ctx->now_ms);

    if (ctx->protocol == SIM_PROTOCOL_AODV)
    {
        status = aodv_en_node_send_data(
            &ctx->aodv_nodes[ctx->scenario->source_index],
            ctx->macs[ctx->scenario->destination_index],
            payload,
            sizeof(uint32_t),
            false,
            ctx->now_ms);
        (void)status;
        return;
    }

    sim_flood_send_data(ctx, sequence);
}

static void sim_tick_protocol(sim_context_t *ctx)
{
    size_t index;

    if (ctx->protocol != SIM_PROTOCOL_AODV)
    {
        return;
    }

    for (index = 0; index < ctx->scenario->node_count; index++)
    {
        aodv_en_node_tick(&ctx->aodv_nodes[index], ctx->now_ms);
    }
}

static void sim_run(sim_context_t *ctx, sim_result_t *result)
{
    uint32_t next_send_ms = 0u;
    uint32_t sequence = 1u;
    uint32_t settle_steps = 0u;

    while (ctx->now_ms < ctx->duration_ms)
    {
        while (next_send_ms <= ctx->now_ms)
        {
            sim_offer_packet(ctx, sequence++);
            next_send_ms += ctx->send_interval_ms;
        }

        sim_tick_protocol(ctx);
        ctx->now_ms += ctx->tick_ms;
    }

    for (settle_steps = 0u; settle_steps < 120u; settle_steps++)
    {
        sim_tick_protocol(ctx);
        ctx->now_ms += ctx->tick_ms;
    }

    memset(result, 0, sizeof(*result));
    result->scenario_name = ctx->scenario->name;
    result->protocol_name = (ctx->protocol == SIM_PROTOCOL_AODV) ? "aodv_en" : "flooding";
    result->profile_name = (ctx->profile != NULL) ? ctx->profile->name : "baseline";
    result->duration_ms = ctx->duration_ms;
    result->offered = ctx->offered_packets;
    result->delivered = ctx->delivered_packets;
    result->tx_total = ctx->tx_total;
    result->rx_total = ctx->rx_total;
    result->tx_control = ctx->tx_control;
    result->tx_data = ctx->tx_data;
    result->tx_failures = ctx->tx_failures;
    result->flood_duplicate_drops = ctx->duplicate_drops_flood;

    if (ctx->protocol == SIM_PROTOCOL_AODV)
    {
        const aodv_en_stats_t *source_stats = &ctx->aodv_nodes[ctx->scenario->source_index].stats;
        result->route_discoveries = source_stats->route_discoveries;
        result->route_repairs = source_stats->route_repairs;
        result->duplicate_rreq_drops = source_stats->duplicate_rreq_drops;
    }

    if (result->offered > 0u)
    {
        result->pdr = (double)result->delivered / (double)result->offered;
    }

    if (result->delivered > 0u)
    {
        result->latency_avg_ms = (double)ctx->latency_sum_ms / (double)result->delivered;
        result->nrl = (double)result->tx_total / (double)result->delivered;
        result->nrl_control = (double)result->tx_control / (double)result->delivered;
    }

    result->energy_est_mj =
        (double)result->tx_total * SIM_TX_ENERGY_MJ +
        (double)result->rx_total * SIM_RX_ENERGY_MJ;
}

static void sim_write_csv_header(FILE *file)
{
    fprintf(file,
            "scenario,protocol,profile,seed,duration_ms,offered,delivered,pdr,latency_avg_ms,"
            "nrl,nrl_control,energy_est_mj,tx_total,rx_total,tx_control,tx_data,tx_failures,"
            "route_discoveries,route_repairs,duplicate_rreq_drops,flood_duplicate_drops\n");
}

static void sim_write_csv_row(FILE *file, const sim_result_t *result)
{
    fprintf(file,
            "%s,%s,%s,%u,%u,%u,%u,%.6f,%.3f,%.6f,%.6f,%.3f,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
            result->scenario_name,
            result->protocol_name,
            result->profile_name,
            result->seed,
            result->duration_ms,
            result->offered,
            result->delivered,
            result->pdr,
            result->latency_avg_ms,
            result->nrl,
            result->nrl_control,
            result->energy_est_mj,
            result->tx_total,
            result->rx_total,
            result->tx_control,
            result->tx_data,
            result->tx_failures,
            result->route_discoveries,
            result->route_repairs,
            result->duplicate_rreq_drops,
            result->flood_duplicate_drops);
}

int main(int argc, char **argv)
{
    const uint32_t repetitions = 8u;
    const size_t scenario_count = sizeof(SIM_SCENARIOS) / sizeof(SIM_SCENARIOS[0]);
    const size_t profile_count = sizeof(SIM_AODV_PROFILES) / sizeof(SIM_AODV_PROFILES[0]);
    FILE *output;
    size_t scenario_index;
    uint32_t rep;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <output_csv_path>\n", argv[0]);
        return 1;
    }

    output = fopen(argv[1], "w");
    if (output == NULL)
    {
        perror("fopen");
        return 1;
    }

    sim_write_csv_header(output);

    for (scenario_index = 0; scenario_index < scenario_count; scenario_index++)
    {
        for (rep = 0u; rep < repetitions; rep++)
        {
            uint32_t seed = (uint32_t)(1000u + (uint32_t)scenario_index * 100u + rep);
            sim_context_t context;
            sim_result_t result;
            size_t profile_index;

            for (profile_index = 0; profile_index < profile_count; profile_index++)
            {
                sim_context_prepare(
                    &context,
                    SIM_PROTOCOL_AODV,
                    &SIM_SCENARIOS[scenario_index],
                    &SIM_AODV_PROFILES[profile_index],
                    seed + (uint32_t)(profile_index * 17u));

                sim_run(&context, &result);
                result.seed = seed + (uint32_t)(profile_index * 17u);
                sim_write_csv_row(output, &result);
            }

            sim_context_prepare(
                &context,
                SIM_PROTOCOL_FLOODING,
                &SIM_SCENARIOS[scenario_index],
                NULL,
                seed + 91u);

            sim_run(&context, &result);
            result.seed = seed + 91u;
            sim_write_csv_row(output, &result);
        }
    }

    fclose(output);
    return 0;
}

