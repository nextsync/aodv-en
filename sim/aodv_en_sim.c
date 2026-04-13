#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "aodv_en_messages.h"
#include "aodv_en_node.h"

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
    uint8_t macs[SIM_NODE_COUNT][AODV_EN_MAC_ADDR_LEN];
    const char *names[SIM_NODE_COUNT];
    aodv_en_node_t nodes[SIM_NODE_COUNT];
    sim_endpoint_t endpoints[SIM_NODE_COUNT];
};

static const char *sim_message_name(uint8_t message_type)
{
    switch ((aodv_en_message_type_t)message_type)
    {
    case AODV_EN_MSG_HELLO:
        return "HELLO";
    case AODV_EN_MSG_RREQ:
        return "RREQ";
    case AODV_EN_MSG_RREP:
        return "RREP";
    case AODV_EN_MSG_RERR:
        return "RERR";
    case AODV_EN_MSG_DATA:
        return "DATA";
    case AODV_EN_MSG_ACK:
        return "ACK";
    default:
        return "UNKNOWN";
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
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0],
           mac[1],
           mac[2],
           mac[3],
           mac[4],
           mac[5]);
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
            if (dest_index == src_index || !network->links[src_index][dest_index])
            {
                continue;
            }

            network->now_ms++;
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
        printf("unicast-drop ");
        sim_print_mac(next_hop);
        printf("\n");
        return AODV_EN_ERR_NO_ROUTE;
    }

    printf("unicast -> %s\n", network->names[dest_index]);
    network->now_ms++;
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
    printf(" for seq=%u\n", sequence_number);
}

static void sim_init_network(sim_network_t *network)
{
    aodv_en_config_t config;
    aodv_en_node_callbacks_t callbacks;
    size_t index;

    memset(network, 0, sizeof(*network));
    aodv_en_config_set_defaults(&config);
    config.network_id = 0xA0DE0001u;

    network->names[0] = "A";
    network->names[1] = "B";
    network->names[2] = "C";

    memcpy(network->macs[0], ((uint8_t[]){0x10, 0x00, 0x00, 0x00, 0x00, 0x0A}), AODV_EN_MAC_ADDR_LEN);
    memcpy(network->macs[1], ((uint8_t[]){0x10, 0x00, 0x00, 0x00, 0x00, 0x0B}), AODV_EN_MAC_ADDR_LEN);
    memcpy(network->macs[2], ((uint8_t[]){0x10, 0x00, 0x00, 0x00, 0x00, 0x0C}), AODV_EN_MAC_ADDR_LEN);

    network->links[0][1] = true;
    network->links[1][0] = true;
    network->links[1][2] = true;
    network->links[2][1] = true;

    for (index = 0; index < SIM_NODE_COUNT; index++)
    {
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
}

int main(void)
{
    static const uint8_t payload[] = "hello over aodv-en";
    sim_network_t network;
    aodv_en_route_entry_t *route_a_to_c;
    aodv_en_route_entry_t *route_b_to_c;

    sim_init_network(&network);

    printf("=== route discovery phase ===\n");
    assert(aodv_en_node_send_data(
               &network.nodes[0],
               network.macs[2],
               payload,
               (uint16_t)(sizeof(payload) - 1u),
               true,
               network.now_ms) == AODV_EN_QUEUED);

    route_a_to_c = aodv_en_route_find_valid(&network.nodes[0].routes, network.macs[2]);
    route_b_to_c = aodv_en_route_find_valid(&network.nodes[1].routes, network.macs[2]);

    assert(route_a_to_c != NULL);
    assert(route_b_to_c != NULL);
    assert(memcmp(route_a_to_c->next_hop, network.macs[1], AODV_EN_MAC_ADDR_LEN) == 0);

    printf("\n=== data phase ===\n");
    assert(aodv_en_node_send_data(
               &network.nodes[0],
               network.macs[2],
               payload,
               (uint16_t)(sizeof(payload) - 1u),
               true,
               network.now_ms) == AODV_EN_OK);

    assert(network.nodes[2].stats.delivered_frames >= 1u);
    assert(network.nodes[0].stats.ack_received >= 1u);

    printf("\n=== summary ===\n");
    printf("A route discoveries=%u ack_received=%u\n",
           network.nodes[0].stats.route_discoveries,
           network.nodes[0].stats.ack_received);
    printf("B forwarded_frames=%u\n", network.nodes[1].stats.forwarded_frames);
    printf("C delivered_frames=%u\n", network.nodes[2].stats.delivered_frames);
    printf("Simulation passed.\n");

    return 0;
}
