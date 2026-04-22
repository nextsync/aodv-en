#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "aodv_en_messages.h"
#include "aodv_en_node.h"

#define SIM_NODE_COUNT 6

typedef struct sim_network sim_network_t;

typedef struct
{
    sim_network_t *network;
    size_t index;
    const char *name;
} sim_endpoint_t;

struct sim_network
{
    uint32_t now_ms;
    bool links[SIM_NODE_COUNT][SIM_NODE_COUNT];
    uint8_t macs[SIM_NODE_COUNT][AODV_EN_MAC_ADDR_LEN];
    const char *names[SIM_NODE_COUNT];
    aodv_en_node_t nodes[SIM_NODE_COUNT];
    sim_endpoint_t endpoints[SIM_NODE_COUNT];
};

static const char *sim_message_name(uint8_t message_type)
{
    switch ((aodv_en_message_type_t)message_type)
    {
    case AODV_EN_MSG_HELLO: return "HELLO";
    case AODV_EN_MSG_RREQ:  return "RREQ";
    case AODV_EN_MSG_RREP:  return "RREP";
    case AODV_EN_MSG_RERR:  return "RERR";
    case AODV_EN_MSG_DATA:  return "DATA";
    case AODV_EN_MSG_ACK:   return "ACK";
    default: return "UNKNOWN";
    }
}

static int sim_find_index_by_mac(
    const sim_network_t *network,
    const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    size_t index;
    for (index = 0; index < SIM_NODE_COUNT; index++)
    {
        if (memcmp(network->macs[index], mac, AODV_EN_MAC_ADDR_LEN) == 0)
        {
            return (int)index;
        }
    }
    return -1;
}

static void sim_print_mac(const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

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

    printf("[t=%u] %s TX %s ", network->now_ms, endpoint->name, sim_message_name(header->message_type));
    
    if (broadcast)
    {
        printf("broadcast\n");
        for (dest_index = 0; dest_index < SIM_NODE_COUNT; dest_index++)
        {
            if (dest_index == src_index || !network->links[src_index][dest_index]) continue;
            
            printf("        -> %s RX %s\n", network->names[dest_index], sim_message_name(header->message_type));
            (void)aodv_en_node_on_recv(
                &network->nodes[dest_index],
                network->macs[src_index],
                frame,
                frame_len,
                -55,
                network->now_ms);
        }
        return AODV_EN_OK;
    }

    dest_index = (size_t)sim_find_index_by_mac(network, next_hop);
    if (dest_index >= SIM_NODE_COUNT || !network->links[src_index][dest_index])
    {
        printf("unicast-drop to ");
        sim_print_mac(next_hop);
        printf("\n");
        
        /* Simulate link layer feedback of failure */
        size_t invalidated = 0;
        (void)aodv_en_node_on_link_tx_result(&network->nodes[src_index], next_hop, false, network->now_ms, &invalidated);
        if (invalidated > 0) printf("        %s invalidated %zu routes due to TX fail\n", endpoint->name, invalidated);

        return AODV_EN_ERR_STATE;
    }

    printf("unicast -> %s\n", network->names[dest_index]);
    
    /* Simulate link layer feedback of success */
    (void)aodv_en_node_on_link_tx_result(&network->nodes[src_index], next_hop, true, network->now_ms, NULL);

    (void)aodv_en_node_on_recv(
        &network->nodes[dest_index],
        network->macs[src_index],
        frame,
        frame_len,
        -50,
        network->now_ms);

    return AODV_EN_OK;
}

static void sim_deliver_data(
    void *user_ctx,
    const uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len)
{
    sim_endpoint_t *endpoint = (sim_endpoint_t *)user_ctx;
    printf("        %s DELIVER data from ", endpoint->name);
    sim_print_mac(originator_mac);
    printf(": %.*s\n", payload_len, (const char *)payload);
}

static void sim_ack_received(
    void *user_ctx,
    const uint8_t ack_sender_mac[AODV_EN_MAC_ADDR_LEN],
    uint32_t sequence_number)
{
    sim_endpoint_t *endpoint = (sim_endpoint_t *)user_ctx;
    printf("        %s ACK received from ", endpoint->name);
    sim_print_mac(ack_sender_mac);
    printf(" for seq=%u\n", (unsigned int)sequence_number);
}

static void sim_init_network(sim_network_t *network)
{
    aodv_en_config_t config;
    aodv_en_node_callbacks_t callbacks;
    size_t index;

    memset(network, 0, sizeof(*network));
    aodv_en_config_set_defaults(&config);
    config.network_id = 0xA0DE6666u;
    config.link_fail_threshold = 1; // Aggressive for sim

    const char *names[] = {"A", "B", "C", "D", "E", "F"};
    for (index = 0; index < SIM_NODE_COUNT; index++)
    {
        network->names[index] = names[index];
        network->macs[index][0] = 0x10;
        network->macs[index][5] = (uint8_t)(0x0A + index);
        
        network->endpoints[index].network = network;
        network->endpoints[index].index = index;
        network->endpoints[index].name = network->names[index];

        assert(aodv_en_node_init(&network->nodes[index], &config, network->macs[index]) == AODV_EN_OK);

        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.emit_frame = sim_emit_frame;
        callbacks.deliver_data = sim_deliver_data;
        callbacks.ack_received = sim_ack_received;
        callbacks.user_ctx = &network->endpoints[index];
        aodv_en_node_set_callbacks(&network->nodes[index], &callbacks);
    }

    /* Chain topology: A-B-C-D-E-F */
    for (index = 0; index < SIM_NODE_COUNT - 1; index++)
    {
        network->links[index][index+1] = true;
        network->links[index+1][index] = true;
    }
}

static void sim_tick_all(sim_network_t *network, uint32_t delta_ms)
{
    size_t index;
    network->now_ms += delta_ms;
    for (index = 0; index < SIM_NODE_COUNT; index++)
    {
        aodv_en_node_tick(&network->nodes[index], network->now_ms);
    }
}

int main(void)
{
    static const uint8_t payload[] = "stress test aodv-en";
    sim_network_t network;
    sim_init_network(&network);

    printf("=== PHASE 1: Chain discovery A -> F ===\n");
    aodv_en_node_send_data(&network.nodes[0], network.macs[5], payload, sizeof(payload), true, network.now_ms);
    
    /* Give some time for discovery to propagate through 5 hops */
    for(int i=0; i<10; i++) sim_tick_all(&network, 100);

    printf("\n=== PHASE 2: Data Delivery check ===\n");
    aodv_en_node_send_data(&network.nodes[0], network.macs[5], payload, sizeof(payload), true, network.now_ms);
    assert(network.nodes[5].stats.delivered_frames > 0);
    assert(network.nodes[0].stats.ack_received > 0);

    printf("\n=== PHASE 3: Link Break D-E and RERR Propagation ===\n");
    /* Break link between D (index 3) and E (index 4) */
    printf("!!! BREAKING LINK D <-> E !!!\n");
    network.links[3][4] = false;
    network.links[4][3] = false;

    /* A tries to send data again */
    printf("A sending data to F again...\n");
    aodv_en_node_send_data(&network.nodes[0], network.macs[5], payload, sizeof(payload), true, network.now_ms);

    /* Advance time to let D detect failure and propagate RERR */
    for(int i=0; i<10; i++) sim_tick_all(&network, 100);

    printf("\n=== PHASE 4: Verification of Route Invalidation at Source ===\n");
    aodv_en_route_entry_t *route_a_to_f = aodv_en_route_find(&network.nodes[0].routes, network.macs[5]);
    if (route_a_to_f == NULL || route_a_to_f->state == AODV_EN_ROUTE_INVALID) {
        printf("SUCCESS: Node A invalidated route to F after RERR propagation.\n");
    } else {
        printf("FAILURE: Node A still has a valid/reverse route to F.\n");
        return 1;
    }

    printf("\n=== PHASE 5: Re-discovery after error ===\n");
    /* Re-enable link to see if it recovers */
    network.links[3][4] = true;
    network.links[4][3] = true;
    
    aodv_en_node_send_data(&network.nodes[0], network.macs[5], payload, sizeof(payload), true, network.now_ms);
    for(int i=0; i<10; i++) sim_tick_all(&network, 100);

    assert(network.nodes[5].stats.delivered_frames > 1);
    
    printf("\n=== Final Stats ===\n");
    for(int i=0; i<SIM_NODE_COUNT; i++) {
        printf("Node %s: rx=%u, tx=%u, delivered=%u, forwarded=%u, rreq_drops=%u\n", 
            network.names[i], 
            network.nodes[i].stats.rx_frames,
            network.nodes[i].stats.tx_frames,
            network.nodes[i].stats.delivered_frames,
            network.nodes[i].stats.forwarded_frames,
            network.nodes[i].stats.duplicate_rreq_drops);
    }

    printf("\nLarge scale simulation PASSED.\n");
    return 0;
}
