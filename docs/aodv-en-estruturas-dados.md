# AODV-EN Estruturas de Dados

## Estado

- alinhado com [aodv-en-spec-v1.md](aodv-en-spec-v1.md)
- ultima revisao: 2026-05-01
- fonte da verdade: cabecalhos em [firmware/components/aodv_en/include](../firmware/components/aodv_en/include)

## Objetivo

Detalhar as estruturas de dados que sustentam a implementacao do `AODV-EN v1`. Este documento traduz a spec funcional para o layout concreto dos cabecalhos C, sem repetir as regras semanticas ja descritas na spec.

Quando houver duvida entre este documento e a spec v1, a spec define o comportamento e este documento define o layout.

## Visao geral

Cada no `AODV-EN` mantem seis estruturas residentes:

1. tabela de vizinhos
2. tabela de rotas (com precursores por rota)
3. cache de duplicatas de `RREQ`
4. cache de peers ESP-NOW
5. fila de `DATA` pendente
6. fila de `ACK` pendente

Mais a configuracao do no e o bloco de estatisticas. Todas sao alocadas estaticamente dentro de `aodv_en_node_t`, sem heap no caminho critico.

## 1. Tabela de vizinhos

### Papel

Representa nos de 1 salto observados pelo radio.

### Entrada

Definida em `aodv_en_neighbor_entry_t` ([aodv_en_types.h](../firmware/components/aodv_en/include/aodv_en_types.h)):

- `mac[6]`
- `avg_rssi` (int8)
- `last_rssi` (int8)
- `link_fail_count` (uint8)
- `state` (uint8): `INACTIVE` ou `ACTIVE`
- `last_seen_ms` (uint32)
- `last_used_ms` (uint32)

### Tabela

`aodv_en_neighbor_table_t`:

- `count` (uint16)
- `entries[AODV_EN_NEIGHBOR_TABLE_SIZE]`

### Operacoes

API em [aodv_en_neighbors.h](../firmware/components/aodv_en/include/aodv_en_neighbors.h):

- `aodv_en_neighbor_table_init`
- `aodv_en_neighbor_find` / `_const`
- `aodv_en_neighbor_touch` (passivo: cada `on_recv` valido toca o vizinho)
- `aodv_en_neighbor_mark_used` (apos uso de unicast)
- `aodv_en_neighbor_note_link_failure` (incrementa `link_fail_count`; promove a `INACTIVE` ao atingir threshold)
- `aodv_en_neighbor_expire`
- `aodv_en_neighbor_count_active`

## 2. Tabela de rotas

### Papel

Representa caminhos `destination -> next_hop`. Cada entrada tambem mantem precursores para emissao direcionada de `RERR`.

### Entrada

`aodv_en_route_entry_t`:

- `destination[6]`
- `next_hop[6]`
- `dest_seq_num` (uint32)
- `expires_at_ms` (uint32)
- `metric` (uint16) — em v1, igual a `hop_count`; valor `AODV_EN_ROUTE_METRIC_INFINITY` (`0xFFFF`) marca invalidada
- `hop_count` (uint8)
- `state` (uint8): `INVALID`, `REVERSE`, `VALID`
- `precursor_count` (uint8)
- `precursors[AODV_EN_MAX_PRECURSORS][6]`

### Tabela

`aodv_en_route_table_t`:

- `count` (uint16)
- `entries[AODV_EN_ROUTE_TABLE_SIZE]`

### Operacoes

API em [aodv_en_routes.h](../firmware/components/aodv_en/include/aodv_en_routes.h):

- `aodv_en_route_table_init`
- `aodv_en_route_find` / `_const` / `_find_valid`
- `aodv_en_route_should_replace` — implementa a regra de selecao com histerese descrita na spec v1, secao "Selecao e substituicao de rotas"
- `aodv_en_route_upsert` — preserva precursores quando o `next_hop` nao muda; reseta quando muda
- `aodv_en_route_add_precursor`
- `aodv_en_route_invalidate_destination`
- `aodv_en_route_invalidate_by_next_hop` — usado quando um vizinho cai
- `aodv_en_route_expire`

## 3. Cache de duplicatas de RREQ

### Papel

Suprimir reprocessamento de `RREQ` repetidos em janela curta.

### Entrada

`aodv_en_rreq_cache_entry_t`:

- `originator[6]`
- `rreq_id` (uint32)
- `created_at_ms` (uint32)
- `hop_count` (uint8)
- `used` (uint8)

### Tabela

`aodv_en_rreq_cache_t`:

- `count` (uint16)
- `entries[AODV_EN_RREQ_CACHE_SIZE]`

### Operacoes

API em [aodv_en_rreq_cache.h](../firmware/components/aodv_en/include/aodv_en_rreq_cache.h):

- `aodv_en_rreq_cache_init`
- `aodv_en_rreq_cache_contains`
- `aodv_en_rreq_cache_remember`
- `aodv_en_rreq_cache_expire` (governado por `rreq_cache_timeout_ms`)

## 4. Cache de peers ESP-NOW

### Papel

Limitar quantos peers ficam registrados no driver ESP-NOW de uma vez. Independente da tabela de vizinhos, mas alinhado a ela.

### Entrada

`aodv_en_peer_cache_entry_t`:

- `mac[6]`
- `last_used_ms` (uint32)
- `flags` (uint8): `NONE`, `PINNED`, `REGISTERED`
- `reserved` (uint8)

### Tabela

`aodv_en_peer_cache_t`:

- `count` (uint16)
- `entries[AODV_EN_PEER_CACHE_SIZE]`

### Operacoes

API em [aodv_en_peers.h](../firmware/components/aodv_en/include/aodv_en_peers.h):

- `aodv_en_peer_cache_init`
- `aodv_en_peer_find` / `_const`
- `aodv_en_peer_touch` (LRU)
- `aodv_en_peer_set_registered`
- `aodv_en_peer_set_pinned`
- `aodv_en_peer_remove`

## 5. Fila de DATA pendente

### Papel

Buffer temporario de `DATA` enquanto a descoberta de rota nao chega ao destino. Comportamento descrito em [features/enfilaremento-dos-dados.md](features/enfilaremento-dos-dados.md) e na spec v1, secao "Fila de DATA pendente".

### Entrada

`aodv_en_pending_data_entry_t` ([aodv_en_node.h](../firmware/components/aodv_en/include/aodv_en_node.h)):

- `destination_mac[6]`
- `payload_len` (uint16)
- `ack_required` (bool)
- `used` (bool)
- `enqueued_at_ms` (uint32)
- `last_rreq_at_ms` (uint32)
- `discovery_attempts` (uint8)
- `payload[AODV_EN_DATA_PAYLOAD_MAX]`

### Operacoes

Encapsuladas dentro de `aodv_en_node_t` (nao expostas como API publica em v1):

- enfileirar com backpressure por reciclagem do mais antigo (preferencia pelo mesmo destino)
- drenar quando uma rota valida e instalada para o destino
- reemitir `RREQ` no tick com backoff exponencial sobre `ack_timeout_ms` ate `10000 ms`
- expirar quando `now - enqueued_at_ms >= route_lifetime_ms`

## 6. Fila de ACK pendente

### Papel

Rastreia `DATA` enviados com `ACK_REQUIRED` para suportar retransmissao quando o `ACK` nao chega.

### Entrada

`aodv_en_pending_ack_entry_t`:

- `destination_mac[6]`
- `payload_len` (uint16)
- `used` (bool)
- `retries_left` (uint8)
- `sequence_number` (uint32)
- `last_sent_at_ms` (uint32)
- `payload[AODV_EN_DATA_PAYLOAD_MAX]`

### Operacoes

- alocar entrada ao enviar `DATA` com `ACK_REQUIRED`, herdando `retries_left = rreq_retry_count`
- consumir entrada ao receber `ACK` correspondente por `(destination, sequence_number)`
- no tick, retransmitir entradas cujo `now - last_sent_at_ms >= ack_timeout_ms`
- ao esgotar `retries_left`, descartar e contabilizar `ack_timeout_drops`
- se na hora do retry a rota for invalida, disparar `RREQ` antes da proxima tentativa

## Cabecalho comum das mensagens

`aodv_en_header_t` ([aodv_en_messages.h](../firmware/components/aodv_en/include/aodv_en_messages.h)):

- `protocol_version` (uint8)
- `message_type` (uint8)
- `flags` (uint8)
- `hop_count` (uint8)
- `network_id` (uint32)
- `sender_mac[6]`

Todas as mensagens sao `__attribute__((packed))` para garantir layout determinista entre nos.

## Mensagens

| Tipo | Struct |
|---|---|
| `HELLO` | `aodv_en_hello_msg_t` |
| `RREQ` | `aodv_en_rreq_msg_t` |
| `RREP` | `aodv_en_rrep_msg_t` |
| `RERR` | `aodv_en_rerr_msg_t` |
| `ACK` | `aodv_en_ack_msg_t` |
| `DATA` | `aodv_en_data_msg_t` (header + metadados + `payload[]` flexivel) |

Os campos de cada uma estao detalhados na spec v1, secao "Mensagens", e materializados em [aodv_en_messages.h](../firmware/components/aodv_en/include/aodv_en_messages.h).

## Configuracao do no

`aodv_en_config_t`:

- `network_id` (uint32)
- `neighbor_timeout_ms` (uint32)
- `route_lifetime_ms` (uint32)
- `rreq_cache_timeout_ms` (uint32)
- `ack_timeout_ms` (uint32)
- `neighbor_table_size`, `route_table_size`, `rreq_cache_size`, `peer_cache_size` (uint16)
- `control_payload_max`, `data_payload_max` (uint16)
- `wifi_channel`, `max_hops`, `ttl_default`, `rreq_retry_count`, `link_fail_threshold` (uint8)

`aodv_en_config_set_defaults` carrega os valores documentados em [aodv_en_limits.h](../firmware/components/aodv_en/include/aodv_en_limits.h).

## Estatisticas

`aodv_en_stack_stats_t` ([aodv_en.h](../firmware/components/aodv_en/include/aodv_en.h)):

- `rx_frames`, `tx_frames`, `forwarded_frames`, `delivered_frames`
- `ack_received`
- `route_discoveries`, `route_repairs`
- `duplicate_rreq_drops`
- `pending_data_queued`, `pending_data_flushed`, `pending_data_dropped`
- `route_discovery_retries`
- `ack_retry_sent`, `ack_timeout_drops`
- `link_fail_events`, `route_invalidations_link_fail`

Esses contadores sao acumulativos e formam a base para os experimentos da Fase 8 do [plano](plano-desenvolvimento-completo.md).

## Limites iniciais

Todos definidos em [aodv_en_limits.h](../firmware/components/aodv_en/include/aodv_en_limits.h):

| Limite | Default |
|---|---|
| `NEIGHBOR_TABLE_SIZE` | `16` |
| `ROUTE_TABLE_SIZE` | `32` |
| `RREQ_CACHE_SIZE` | `64` |
| `PEER_CACHE_SIZE` | `8` |
| `PENDING_DATA_QUEUE_SIZE` | `4` |
| `MAX_PRECURSORS` | `4` |
| `CONTROL_PAYLOAD_MAX` | `128` |
| `DATA_PAYLOAD_MAX` | `1024` |
| `MAX_HOPS` / `TTL` | `16` |

## Estimativa de memoria

Com os defaults, o consumo agregado das tabelas e filas residentes em RAM por no e:

- vizinhos: 16 entradas x ~24 B = ~384 B
- rotas (com precursores e demais campos): 32 entradas x ~52 B = ~1.7 KB
- cache de `RREQ`: 64 entradas x ~16 B = ~1 KB
- cache de peers: 8 entradas x ~16 B = ~128 B
- `pending_data`: 4 entradas x (~24 B + 1024 B payload) = ~4.1 KB
- `pending_ack`: 4 entradas x (~24 B + 1024 B payload) = ~4.1 KB

Total residente da camada `AODV-EN`: ~11.5 KB por no, dominado pelos buffers de payload das filas pendentes. E confortavel para um ESP32, mas e o item de memoria a observar caso `DATA_PAYLOAD_MAX` ou `PENDING_DATA_QUEUE_SIZE` sejam aumentados em experimentos futuros.

## Principios de projeto

- tabelas pequenas, previsiveis e indexaveis sequencialmente
- nenhuma alocacao dinamica no caminho critico
- separacao clara entre vizinhos e roteamento
- precursores como sub-array da rota, nao tabela separada (mantem cohesao)
- `RREQ` cache como estrutura de primeira classe
- cache de peers desacoplado da tabela de vizinhos para refletir a politica do driver ESP-NOW
