#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "flood_en.h"

#define SIM_NODE_COUNT 3

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
    uint8_t macs[SIM_NODE_COUNT][FLOOD_EN_MAC_ADDR_LEN];
    const char *names[SIM_NODE_COUNT];
    flood_en_node_t nodes[SIM_NODE_COUNT];
    sim_endpoint_t endpoints[SIM_NODE_COUNT];
};

static const char *sim_message_name(uint8_t message_type)
{
    switch ((flood_en_message_type_t)message_type)
    {
    case FLOOD_EN_MSG_DATA:
        return "DATA";
    case FLOOD_EN_MSG_ACK:
        return "ACK";
    default:
        return "OTHER";
    }
}

static void sim_print_mac(const uint8_t mac[FLOOD_EN_MAC_ADDR_LEN])
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static flood_en_status_t sim_emit_frame(
    void *user_ctx,
    const uint8_t next_hop[FLOOD_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    bool broadcast)
{
    const flood_en_header_t *header = (const flood_en_header_t *)frame;
    sim_endpoint_t *endpoint = (sim_endpoint_t *)user_ctx;
    sim_network_t *network = endpoint->network;
    size_t src_index = endpoint->index;
    size_t dest_index;

    (void)next_hop;
    (void)broadcast;

    printf("[t=%u] %s TX %s broadcast (ttl-hop=%u)\n",
           network->now_ms,
           endpoint->name,
           sim_message_name(header->message_type),
           header->hop_count);

    for (dest_index = 0; dest_index < SIM_NODE_COUNT; dest_index++)
    {
        if (dest_index == src_index || !network->links[src_index][dest_index])
        {
            continue;
        }

        network->now_ms++;
        printf("        -> %s RX %s\n", network->names[dest_index], sim_message_name(header->message_type));
        (void)flood_en_node_on_recv(
            &network->nodes[dest_index],
            network->macs[src_index],
            frame,
            frame_len,
            -55,
            network->now_ms);
    }

    return FLOOD_EN_OK;
}

static void sim_deliver_data(
    void *user_ctx,
    const uint8_t originator_mac[FLOOD_EN_MAC_ADDR_LEN],
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
    const uint8_t ack_sender_mac[FLOOD_EN_MAC_ADDR_LEN],
    uint32_t sequence_number)
{
    sim_endpoint_t *endpoint = (sim_endpoint_t *)user_ctx;

    printf("        %s ACK received from ", endpoint->name);
    sim_print_mac(ack_sender_mac);
    printf(" for seq=%u\n", sequence_number);
}

static void sim_init_network(sim_network_t *network)
{
    flood_en_config_t config;
    flood_en_callbacks_t callbacks;
    size_t index;

    memset(network, 0, sizeof(*network));
    flood_en_config_set_defaults(&config);
    config.network_id = 0xA0DE0001u;

    network->names[0] = "A";
    network->names[1] = "B";
    network->names[2] = "C";

    memcpy(network->macs[0], ((uint8_t[]){0x10, 0x00, 0x00, 0x00, 0x00, 0x0A}), FLOOD_EN_MAC_ADDR_LEN);
    memcpy(network->macs[1], ((uint8_t[]){0x10, 0x00, 0x00, 0x00, 0x00, 0x0B}), FLOOD_EN_MAC_ADDR_LEN);
    memcpy(network->macs[2], ((uint8_t[]){0x10, 0x00, 0x00, 0x00, 0x00, 0x0C}), FLOOD_EN_MAC_ADDR_LEN);

    network->links[0][1] = true;
    network->links[1][0] = true;
    network->links[1][2] = true;
    network->links[2][1] = true;

    for (index = 0; index < SIM_NODE_COUNT; index++)
    {
        network->endpoints[index].network = network;
        network->endpoints[index].index = index;
        network->endpoints[index].name = network->names[index];

        assert(flood_en_node_init(&network->nodes[index], &config, network->macs[index]) == FLOOD_EN_OK);

        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.tx_frame = sim_emit_frame;
        callbacks.deliver_data = sim_deliver_data;
        callbacks.ack_received = sim_ack_received;
        callbacks.user_ctx = &network->endpoints[index];
        flood_en_node_set_callbacks(&network->nodes[index], &callbacks);
    }
}

int main(void)
{
    static const uint8_t payload[] = "hello over flood-en";
    sim_network_t network;

    sim_init_network(&network);

    printf("=== flood delivery phase (A -> C via B, no routes) ===\n");
    assert(flood_en_node_send_data(
               &network.nodes[0],
               network.macs[2],
               payload,
               (uint16_t)(sizeof(payload) - 1u),
               true,
               network.now_ms) == FLOOD_EN_OK);

    assert(network.nodes[2].stats.delivered_frames == 1u);
    assert(network.nodes[0].stats.ack_received == 1u);

    printf("\n=== duplicate suppression check ===\n");
    assert(network.nodes[0].stats.duplicate_drops >= 1u);
    assert(network.nodes[2].stats.duplicate_drops >= 1u);

    printf("\n=== second flood (seq increments, still delivers) ===\n");
    assert(flood_en_node_send_data(
               &network.nodes[0],
               network.macs[2],
               payload,
               (uint16_t)(sizeof(payload) - 1u),
               true,
               network.now_ms) == FLOOD_EN_OK);

    assert(network.nodes[2].stats.delivered_frames == 2u);
    assert(network.nodes[0].stats.ack_received == 2u);

    printf("\n=== ttl bound check (no storm) ===\n");
    assert(network.nodes[0].stats.ttl_drops == 0u);
    assert(network.nodes[1].stats.rebroadcast_frames >= 2u);

    printf("\n=== summary ===\n");
    printf("Node A: tx=%u rx=%u ack_received=%u dup_drops=%u\n",
           network.nodes[0].stats.tx_frames,
           network.nodes[0].stats.rx_frames,
           network.nodes[0].stats.ack_received,
           network.nodes[0].stats.duplicate_drops);
    printf("Node B: tx=%u rx=%u rebroadcast=%u dup_drops=%u\n",
           network.nodes[1].stats.tx_frames,
           network.nodes[1].stats.rx_frames,
           network.nodes[1].stats.rebroadcast_frames,
           network.nodes[1].stats.duplicate_drops);
    printf("Node C: tx=%u rx=%u delivered=%u dup_drops=%u\n",
           network.nodes[2].stats.tx_frames,
           network.nodes[2].stats.rx_frames,
           network.nodes[2].stats.delivered_frames,
           network.nodes[2].stats.duplicate_drops);
    printf("Flood simulation passed.\n");

    return 0;
}
