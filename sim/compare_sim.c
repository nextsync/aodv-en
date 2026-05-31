#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "aodv_en_messages.h"
#include "aodv_en_node.h"
#include "flood_en.h"

#define MAX_SIDE 5
#define MAX_NODES (MAX_SIDE * MAX_SIDE)
#define DATA_PACKETS 10
#define PUMP_ROUNDS 40
#define PUMP_STEP_MS 25u

typedef struct grid_net grid_net_t;

typedef struct
{
    grid_net_t *net;
    size_t index;
} endpoint_t;

struct grid_net
{
    uint32_t now_ms;
    size_t node_count;
    bool links[MAX_NODES][MAX_NODES];
    uint8_t macs[MAX_NODES][AODV_EN_MAC_ADDR_LEN];
    endpoint_t endpoints[MAX_NODES];
    uint32_t tx_count;
    uint32_t delivered_count;
    uint32_t ack_count;
    aodv_en_node_t aodv_nodes[MAX_NODES];
    flood_en_node_t flood_nodes[MAX_NODES];
    bool use_flood;
};

static int find_index_by_mac(const grid_net_t *net, const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    for (size_t i = 0; i < net->node_count; i++)
    {
        if (memcmp(net->macs[i], mac, AODV_EN_MAC_ADDR_LEN) == 0)
        {
            return (int)i;
        }
    }
    return -1;
}

static void deliver_frame_to(grid_net_t *net, size_t dest_index, size_t src_index, const uint8_t *frame, size_t frame_len)
{
    net->now_ms++;
    if (net->use_flood)
    {
        (void)flood_en_node_on_recv(&net->flood_nodes[dest_index], net->macs[src_index], frame, frame_len, -55, net->now_ms);
    }
    else
    {
        (void)aodv_en_node_on_recv(&net->aodv_nodes[dest_index], net->macs[src_index], frame, frame_len, -55, net->now_ms);
    }
}

static aodv_en_status_t grid_emit_frame(
    void *user_ctx,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    bool broadcast)
{
    endpoint_t *endpoint = (endpoint_t *)user_ctx;
    grid_net_t *net = endpoint->net;
    size_t src_index = endpoint->index;

    net->tx_count++;

    if (broadcast)
    {
        for (size_t d = 0; d < net->node_count; d++)
        {
            if (d != src_index && net->links[src_index][d])
            {
                deliver_frame_to(net, d, src_index, frame, frame_len);
            }
        }
        return AODV_EN_OK;
    }

    int dest_index = find_index_by_mac(net, next_hop);
    if (dest_index < 0 || !net->links[src_index][dest_index])
    {
        return AODV_EN_ERR_NO_ROUTE;
    }
    deliver_frame_to(net, (size_t)dest_index, src_index, frame, frame_len);
    return AODV_EN_OK;
}

static void grid_deliver_data(
    void *user_ctx,
    const uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len)
{
    endpoint_t *endpoint = (endpoint_t *)user_ctx;
    (void)originator_mac;
    (void)payload;
    (void)payload_len;
    endpoint->net->delivered_count++;
}

static void grid_ack_received(
    void *user_ctx,
    const uint8_t ack_sender_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t sequence_number)
{
    endpoint_t *endpoint = (endpoint_t *)user_ctx;
    (void)ack_sender_mac;
    (void)sequence_number;
    endpoint->net->ack_count++;
}

static void grid_build_links(grid_net_t *net, int side)
{
    int n = side * side;
    net->node_count = (size_t)n;
    for (int i = 0; i < n; i++)
    {
        net->macs[i][0] = 0x10;
        net->macs[i][1] = 0x00;
        net->macs[i][2] = 0x00;
        net->macs[i][3] = 0x00;
        net->macs[i][4] = 0x00;
        net->macs[i][5] = (uint8_t)i;
    }
    for (int r = 0; r < side; r++)
    {
        for (int c = 0; c < side; c++)
        {
            int idx = r * side + c;
            if (c + 1 < side)
            {
                int right = r * side + (c + 1);
                net->links[idx][right] = true;
                net->links[right][idx] = true;
            }
            if (r + 1 < side)
            {
                int down = (r + 1) * side + c;
                net->links[idx][down] = true;
                net->links[down][idx] = true;
            }
        }
    }
}

static void grid_pump(grid_net_t *net, uint32_t ack_timeout_ms)
{
    for (size_t i = 0; i < net->node_count; i++)
    {
        if (net->use_flood)
        {
            flood_en_node_tick(&net->flood_nodes[i], net->now_ms);
        }
        else
        {
            aodv_en_node_tick(&net->aodv_nodes[i], net->now_ms);
        }
    }
    net->now_ms += ack_timeout_ms;
}

static void run_protocol(int side, bool use_flood, const char *label)
{
    static grid_net_t net;
    aodv_en_config_t config;
    static const uint8_t payload[] = "compare-payload";
    uint16_t payload_len = (uint16_t)(sizeof(payload) - 1u);
    size_t src, dst;
    uint32_t sent = 0;

    memset(&net, 0, sizeof(net));
    net.use_flood = use_flood;
    grid_build_links(&net, side);

    aodv_en_config_set_defaults(&config);
    config.network_id = 0xC0DE0001u;

    for (size_t i = 0; i < net.node_count; i++)
    {
        net.endpoints[i].net = &net;
        net.endpoints[i].index = i;

        if (use_flood)
        {
            assert(flood_en_node_init(&net.flood_nodes[i], &config, net.macs[i]) == AODV_EN_OK);
            aodv_en_node_callbacks_t cb = {0};
            cb.emit_frame = grid_emit_frame;
            cb.deliver_data = grid_deliver_data;
            cb.ack_received = grid_ack_received;
            cb.user_ctx = &net.endpoints[i];
            flood_en_node_set_callbacks(&net.flood_nodes[i], &cb);
        }
        else
        {
            assert(aodv_en_node_init(&net.aodv_nodes[i], &config, net.macs[i]) == AODV_EN_OK);
            aodv_en_node_callbacks_t cb = {0};
            cb.emit_frame = grid_emit_frame;
            cb.deliver_data = grid_deliver_data;
            cb.ack_received = grid_ack_received;
            cb.user_ctx = &net.endpoints[i];
            aodv_en_node_set_callbacks(&net.aodv_nodes[i], &cb);
        }
    }

    src = 0;
    dst = net.node_count - 1;

    for (uint32_t k = 0; k < DATA_PACKETS; k++)
    {
        net.now_ms += 10u;
        if (use_flood)
        {
            (void)flood_en_node_send_data(&net.flood_nodes[src], net.macs[dst], payload, payload_len, true, net.now_ms);
        }
        else
        {
            (void)aodv_en_node_send_data(&net.aodv_nodes[src], net.macs[dst], payload, payload_len, true, net.now_ms);
        }
        sent++;

        uint32_t delivered_before = net.delivered_count;
        for (int pump = 0; pump < PUMP_ROUNDS; pump++)
        {
            grid_pump(&net, PUMP_STEP_MS);
            if (net.delivered_count > delivered_before && net.ack_count >= net.delivered_count)
            {
                break;
            }
        }
    }

    int hops = 2 * (side - 1);
    double txper = (net.delivered_count > 0)
                       ? (double)net.tx_count / (double)net.delivered_count
                       : 0.0;

    printf("%s,%d,%zu,%d,%u,%u,%u,%u,%.2f\n",
           label,
           side,
           net.node_count,
           hops,
           sent,
           net.delivered_count,
           net.ack_count,
           net.tx_count,
           txper);
}

int main(void)
{
    printf("protocol,side,nodes,hops,sent,delivered,ack,total_tx,tx_per_delivered\n");
    for (int side = 2; side <= MAX_SIDE; side++)
    {
        run_protocol(side, false, "aodv");
        run_protocol(side, true, "flood");
    }
    return 0;
}
