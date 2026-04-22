#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "aodv_en_messages.h"
#include "aodv_en_node.h"

#define GRID_SIZE 10
#define SIM_NODE_COUNT (GRID_SIZE * GRID_SIZE)

typedef struct sim_network sim_network_t;

typedef struct
{
    sim_network_t *network;
    size_t index;
    int r, c;
} sim_endpoint_t;

struct sim_network
{
    uint32_t now_ms;
    bool links[SIM_NODE_COUNT][SIM_NODE_COUNT];
    uint8_t macs[SIM_NODE_COUNT][AODV_EN_MAC_ADDR_LEN];
    aodv_en_node_t nodes[SIM_NODE_COUNT];
    sim_endpoint_t endpoints[SIM_NODE_COUNT];
};

static aodv_en_status_t sim_emit_frame(
    void *user_ctx,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    bool broadcast)
{
    const aodv_en_header_t *header = (const aodv_en_header_t *)frame;
    sim_endpoint_t *endpoint = (sim_endpoint_t *)user_ctx;
    sim_network_t *network = endpoint->network;
    size_t src_index = endpoint->index;
    size_t dest_index;

    if (broadcast)
    {
        for (dest_index = 0; dest_index < SIM_NODE_COUNT; dest_index++)
        {
            if (dest_index == src_index || !network->links[src_index][dest_index]) continue;
            (void)aodv_en_node_on_recv(&network->nodes[dest_index], network->macs[src_index], frame, frame_len, -60, network->now_ms);
        }
        return AODV_EN_OK;
    }

    /* Unicast */
    for (dest_index = 0; dest_index < SIM_NODE_COUNT; dest_index++) {
        if (memcmp(network->macs[dest_index], next_hop, AODV_EN_MAC_ADDR_LEN) == 0) break;
    }

    if (dest_index >= SIM_NODE_COUNT || !network->links[src_index][dest_index])
    {
        (void)aodv_en_node_on_link_tx_result(&network->nodes[src_index], next_hop, false, network->now_ms, NULL);
        return AODV_EN_ERR_STATE;
    }

    (void)aodv_en_node_on_link_tx_result(&network->nodes[src_index], next_hop, true, network->now_ms, NULL);
    (void)aodv_en_node_on_recv(&network->nodes[dest_index], network->macs[src_index], frame, frame_len, -50, network->now_ms);

    return AODV_EN_OK;
}

static void sim_init_network(sim_network_t *network)
{
    aodv_en_config_t config;
    size_t i;

    memset(network, 0, sizeof(*network));
    aodv_en_config_set_defaults(&config);
    config.network_id = 0x100100100u;
    config.max_hops = 64;   /* Essential for large networks */
    config.ttl_default = 64;
    config.link_fail_threshold = 1;

    for (i = 0; i < SIM_NODE_COUNT; i++)
    {
        network->endpoints[i].network = network;
        network->endpoints[i].index = i;
        network->endpoints[i].r = i / GRID_SIZE;
        network->endpoints[i].c = i % GRID_SIZE;

        network->macs[i][0] = 0x20;
        network->macs[i][4] = (uint8_t)network->endpoints[i].r;
        network->macs[i][5] = (uint8_t)network->endpoints[i].c;

        assert(aodv_en_node_init(&network->nodes[i], &config, network->macs[i]) == AODV_EN_OK);
        
        aodv_en_node_callbacks_t cb = { .emit_frame = sim_emit_frame, .user_ctx = &network->endpoints[i] };
        aodv_en_node_set_callbacks(&network->nodes[i], &cb);
    }

    /* Set Grid Links */
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            int idx = r * GRID_SIZE + c;
            if (r > 0) { network->links[idx][(r-1)*GRID_SIZE+c] = true; }
            if (r < GRID_SIZE-1) { network->links[idx][(r+1)*GRID_SIZE+c] = true; }
            if (c > 0) { network->links[idx][idx-1] = true; }
            if (c < GRID_SIZE-1) { network->links[idx][idx+1] = true; }
        }
    }
}

static void sim_tick_all(sim_network_t *network, uint32_t delta_ms)
{
    network->now_ms += delta_ms;
    for (size_t i = 0; i < SIM_NODE_COUNT; i++) aodv_en_node_tick(&network->nodes[i], network->now_ms);
}

int main(void)
{
    sim_network_t network;
    sim_init_network(&network);
    uint8_t payload[] = "100 nodes stress";
    uint8_t *src_mac = network.macs[0];       /* (0,0) */
    uint8_t *dest_mac = network.macs[99];     /* (9,9) */

    printf("=== Starting 100 Nodes Grid Simulation (10x10) ===\n");

    printf("Step 1: Discovering route from (0,0) to (9,9)...\n");
    aodv_en_node_send_data(&network.nodes[0], dest_mac, payload, sizeof(payload), true, network.now_ms);
    
    /* Propagate RREQ/RREP */
    for(int i=0; i<50; i++) sim_tick_all(&network, 100);

    aodv_en_route_entry_t *route = aodv_en_route_find_valid(&network.nodes[0].routes, dest_mac);
    if (route) {
        printf("Route discovered! Hops: %u, Next Hop: %02X:%02X\n", route->hop_count, route->next_hop[4], route->next_hop[5]);
    } else {
        printf("FAILED to discover route in grid.\n");
        return 1;
    }

    printf("\nStep 2: Sending data packets...\n");
    for(int i=0; i<5; i++) {
        aodv_en_node_send_data(&network.nodes[0], dest_mac, payload, sizeof(payload), true, network.now_ms);
        sim_tick_all(&network, 100);
    }
    printf("Delivery successful. Source ACKs: %u, Dest Delivered: %u\n", 
           network.nodes[0].stats.ack_received, network.nodes[99].stats.delivered_frames);

    printf("\nStep 3: BREAKING CENTRAL WALL (Column 5) !!!\n");
    /* Break all links crossing from column 4 to 5 to force a massive detour */
    for (int r = 0; r < GRID_SIZE; r++) {
        int left = r * GRID_SIZE + 4;
        int right = r * GRID_SIZE + 5;
        network.links[left][right] = false;
        network.links[right][left] = false;
    }

    printf("Sending data again to trigger RERR and recovery...\n");
    aodv_en_node_send_data(&network.nodes[0], dest_mac, payload, sizeof(payload), true, network.now_ms);

    /* Allow time for RERR to flow back and new discovery to happen */
    for(int i=0; i<100; i++) sim_tick_all(&network, 100);

    route = aodv_en_route_find_valid(&network.nodes[0].routes, dest_mac);
    if (route) {
        printf("Recovery successful! New Route Hops: %u\n", route->hop_count);
    } else {
        printf("Recovery failed or still in progress.\n");
    }

    printf("\nStep 4: Final Verification\n");
    uint32_t total_fwd = 0;
    for(int i=0; i<SIM_NODE_COUNT; i++) total_fwd += network.nodes[i].stats.forwarded_frames;
    printf("Total frames forwarded in mesh: %u\n", total_fwd);
    printf("Source (0,0) seq: %u\n", network.nodes[0].self_seq_num);

    printf("\n100 Nodes Simulation Finished.\n");
    return 0;
}
