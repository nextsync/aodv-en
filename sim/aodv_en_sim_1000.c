#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "aodv_en_messages.h"
#include "aodv_en_node.h"

#define GRID_SIDE 32
#define SIM_NODE_COUNT (GRID_SIDE * GRID_SIDE) /* 1024 Nodes */

typedef struct sim_network sim_network_t;

typedef struct {
    sim_network_t *network;
    size_t index;
    int r, c;
    bool alive;
} sim_endpoint_t;

struct sim_network {
    uint32_t now_ms;
    uint8_t macs[SIM_NODE_COUNT][AODV_EN_MAC_ADDR_LEN];
    aodv_en_node_t nodes[SIM_NODE_COUNT];
    sim_endpoint_t endpoints[SIM_NODE_COUNT];
    uint32_t total_tx;
    uint32_t total_rx;
    uint32_t rerr_count;
};

/* Fast MAC to Index mapping for 1000 nodes */
static int sim_get_index_from_mac(const uint8_t mac[AODV_EN_MAC_ADDR_LEN]) {
    if (mac[0] != 0x30) return -1;
    int r = mac[4];
    int c = mac[5];
    int idx = r * GRID_SIDE + c;
    if (idx < 0 || idx >= SIM_NODE_COUNT) return -1;
    return idx;
}

static bool sim_is_in_range(sim_endpoint_t *a, sim_endpoint_t *b) {
    if (!a->alive || !b->alive) return false;
    int dr = abs(a->r - b->r);
    int dc = abs(a->c - b->c);
    /* Radio range: neighbors + diagonals (approx 1.5 units) */
    return (dr <= 1 && dc <= 1);
}

static aodv_en_status_t sim_emit_frame(
    void *user_ctx,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    bool broadcast)
{
    const aodv_en_header_t *header = (const aodv_en_header_t *)frame;
    sim_endpoint_t *src = (sim_endpoint_t *)user_ctx;
    sim_network_t *network = src->network;
    
    network->total_tx++;
    if (header->message_type == AODV_EN_MSG_RERR) network->rerr_count++;

    if (broadcast) {
        for (int i = 0; i < SIM_NODE_COUNT; i++) {
            if (i == src->index) continue;
            if (sim_is_in_range(src, &network->endpoints[i])) {
                network->total_rx++;
                (void)aodv_en_node_on_recv(&network->nodes[i], network->macs[src->index], frame, frame_len, -65, network->now_ms);
            }
        }
        return AODV_EN_OK;
    }

    /* Unicast */
    int dest_idx = sim_get_index_from_mac(next_hop);
    if (dest_idx < 0 || !sim_is_in_range(src, &network->endpoints[dest_idx])) {
        (void)aodv_en_node_on_link_tx_result(&network->nodes[src->index], next_hop, false, network->now_ms, NULL);
        return AODV_EN_ERR_STATE;
    }

    (void)aodv_en_node_on_link_tx_result(&network->nodes[src->index], next_hop, true, network->now_ms, NULL);
    network->total_rx++;
    (void)aodv_en_node_on_recv(&network->nodes[dest_idx], network->macs[src->index], frame, frame_len, -55, network->now_ms);

    return AODV_EN_OK;
}

int main(void) {
    sim_network_t *network = calloc(1, sizeof(sim_network_t));
    aodv_en_config_t config;
    aodv_en_config_set_defaults(&config);
    config.max_hops = 100;
    config.ttl_default = 100;
    config.route_table_size = 64;
    config.neighbor_table_size = 16;
    config.link_fail_threshold = 2;

    printf("=== Smart City Simulation: 1024 Nodes (32x32 Grid) ===\n");
    for (int i = 0; i < SIM_NODE_COUNT; i++) {
        network->endpoints[i].network = network;
        network->endpoints[i].index = i;
        network->endpoints[i].r = i / GRID_SIDE;
        network->endpoints[i].c = i % GRID_SIDE;
        network->endpoints[i].alive = true;
        
        network->macs[i][0] = 0x30;
        network->macs[i][4] = (uint8_t)network->endpoints[i].r;
        network->macs[i][5] = (uint8_t)network->endpoints[i].c;

        aodv_en_node_init(&network->nodes[i], &config, network->macs[i]);
        aodv_en_node_callbacks_t cb = { .emit_frame = sim_emit_frame, .user_ctx = &network->endpoints[i] };
        aodv_en_node_set_callbacks(&network->nodes[i], &cb);
    }

    uint8_t payload[] = "POSTE_OK";
    uint8_t *gw_mac = network->macs[0];      /* Gateway poste (0,0) */
    uint8_t *far_mac = network->macs[1023];  /* Poste mais distante (31,31) */

    printf("\n[SITUACAO 1] Descoberta Inicial em Rede de Grande Escala...\n");
    aodv_en_node_send_data(&network->nodes[0], far_mac, payload, sizeof(payload), true, network->now_ms);
    for(int i=0; i<150; i++) {
        network->now_ms += 100;
        for(int n=0; n<SIM_NODE_COUNT; n++) aodv_en_node_tick(&network->nodes[n], network->now_ms);
    }
    
    aodv_en_route_entry_t *r = aodv_en_route_find_valid(&network->nodes[0].routes, far_mac);
    if(r) printf("Rota estabelecida! Saltos: %u\n", r->hop_count);
    else printf("Alerta: Rota nao convergiu a tempo.\n");

    printf("\n[SITUACAO 2] Cotidiano: 5%% dos nos falham (Manutencao/Quebra)...\n");
    int death_count = 0;
    for(int i=0; i<SIM_NODE_COUNT; i++) {
        if (i != 0 && i != 1023 && (rand() % 100 < 5)) {
            network->endpoints[i].alive = false;
            death_count++;
        }
    }
    printf("%d postes pararam de funcionar.\n", death_count);

    printf("\n[SITUACAO 3] Tentando comunicar atraves das falhas...\n");
    uint32_t successful_acks = 0;
    for(int i=0; i<10; i++) {
        aodv_en_node_send_data(&network->nodes[0], far_mac, payload, sizeof(payload), true, network->now_ms);
        for(int t=0; t<50; t++) { 
             network->now_ms += 100;
             for(int n=0; n<SIM_NODE_COUNT; n++) aodv_en_node_tick(&network->nodes[n], network->now_ms);
        }
        if (network->nodes[0].stats.ack_received > successful_acks) {
            successful_acks = network->nodes[0].stats.ack_received;
            printf("Sucesso na entrega apos reconvergencia!\n");
        }
    }

    printf("\n=== Resumo do Stress Test (1024 Nos) ===\n");
    printf("Total de frames TX na malha: %u\n", network->total_tx);
    printf("Total de mensagens RERR: %u\n", network->rerr_count);
    printf("Mensagens entregues no destino: %u\n", network->nodes[1023].stats.delivered_frames);
    printf("Estabilidade do sistema: %s\n", (network->total_tx > 0) ? "OPERACIONAL" : "FALHA");

    free(network);
    return 0;
}
