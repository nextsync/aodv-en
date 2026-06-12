# AODV-EN - Mapa do Código

## Estado

- alinhado com [aodv-en-spec-v1.md](aodv-en-spec-v1.md)
- complementar a [aodv-en-funcionamento.md](aodv-en-funcionamento.md)
- ultima revisao: 2026-05-01

## Para quem este documento e

Voce ja entendeu **o que** o protocolo faz (em [aodv-en-funcionamento.md](aodv-en-funcionamento.md)) e quer agora saber **onde** cada peca esta implementada. Este documento e um guia de estudo do codigo, organizado por:

- arvore do projeto
- camadas e seus arquivos
- o que cada arquivo contem
- onde encontrar funcionalidades especificas (mapeamento por feature)

Quem precisa: voce mesmo daqui a 3 meses; coorientador; banca pedindo "me mostra onde isso acontece"; alguem que quer estender o protocolo.

---

## Sumario

1. [Arvore do projeto](#arvore-do-projeto)
2. [Camada 1 - Nucleo do protocolo](#camada-1---nucleo-do-protocolo-firmwarecomponentsaodv_en)
3. [Camada 2 - Fachada da lib](#camada-2---fachada-da-lib-aodv_enc)
4. [Camada 3 - Aplicacao firmware](#camada-3---aplicacao-firmware-firmwaremain)
5. [Camada 4 - Build system](#camada-4---build-system)
6. [Camada 5 - Scripts](#camada-5---scripts)
7. [Camada 6 - Simulacao](#camada-6---simulacao-sim)
8. [Camada 7 - Ferramentas de analise](#camada-7---ferramentas-de-analise-firmwaretools)
9. [Camada 8 - Testes funcionais](#camada-8---testes-funcionais)
10. [Camada 9 - Documentacao](#camada-9---documentacao-docs)
11. [Mapa por feature](#mapa-por-feature)
12. [Cross-reference de constantes](#cross-reference-de-constantes)
13. [Como uma mensagem viaja pelo codigo](#como-uma-mensagem-viaja-pelo-codigo)

---

## Arvore do projeto

```
aodv-en/
├── README.md
├── docs/                                  <- documentacao
│   ├── aodv-en-spec-v1.md
│   ├── aodv-en-funcionamento.md           <- "como funciona"
│   ├── aodv-en-mapa-do-codigo.md          <- voce esta aqui
│   ├── aodv-en-estruturas-dados.md
│   ├── aodv-base-invariantes.md
│   ├── aodv-en-spec-v0.md                 (OBSOLETA)
│   ├── plano-desenvolvimento-completo.md
│   ├── runbook-bancada.md
│   ├── features/
│   │   ├── enfilaremento-dos-dados.md
│   │   ├── precursores.md
│   │   └── articulation-point-planejado.md
│   └── tests/                             <- casos de teste
│       ├── README.md
│       ├── tc-001-descoberta-e-entrega-direta.md
│       ├── tc-002-primeiro-multi-hop.md
│       ├── tc-003-reconvergencia-apos-falha.md
│       ├── tc-004-soak-estabilidade-e-reconvergencia.md
│       ├── tc-005-cadeia-4-nos.md
│       └── guia-leitura-graficos-monitor.md
│
├── firmware/                              <- ESP-IDF project
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── sdkconfig.defaults
│   ├── build.sh, flash_monitor.sh, monitor_log.sh, idf-env.sh
│   │
│   ├── components/aodv_en/                <- BIBLIOTECA DO PROTOCOLO
│   │   ├── CMakeLists.txt
│   │   ├── include/                       <- headers publicos
│   │   │   ├── aodv_en.h                  <- API publica (stack)
│   │   │   ├── aodv_en_node.h             <- API do nucleo
│   │   │   ├── aodv_en_messages.h         <- structs no fio
│   │   │   ├── aodv_en_types.h            <- tipos centrais + config
│   │   │   ├── aodv_en_status.h           <- codigos de retorno
│   │   │   ├── aodv_en_limits.h           <- constantes/defaults
│   │   │   ├── aodv_en_tables.h           <- structs das tabelas
│   │   │   ├── aodv_en_mac.h              <- utilitarios de MAC
│   │   │   ├── aodv_en_neighbors.h
│   │   │   ├── aodv_en_routes.h
│   │   │   ├── aodv_en_rreq_cache.h
│   │   │   └── aodv_en_peers.h
│   │   └── src/                           <- implementacao
│   │       ├── aodv_en.c                  (537 linhas) fachada
│   │       ├── aodv_en_node.c             (1658 linhas) nucleo
│   │       ├── aodv_en_routes.c           (352)  tabela de rotas + precursores
│   │       ├── aodv_en_neighbors.c        (203)  tabela de vizinhos
│   │       ├── aodv_en_peers.c            (209)  cache LRU de peers ESP-NOW
│   │       ├── aodv_en_rreq_cache.c       (150)  cache de duplicatas RREQ
│   │       └── aodv_en_mac.c              (59)   utilitarios de MAC
│   │
│   ├── main/                              <- APLICACAO ESP-IDF
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig.projbuild              <- menuconfig
│   │   ├── main.c                         (10) entry point com branch
│   │   ├── app_demo.c                     (555) app legado (HELLO + DATA)
│   │   ├── app_demo.h
│   │   ├── app_proto_example.c            (1282) app com CLI + HEALTH/TEXT/CMD
│   │   ├── app_proto_example.h
│   │   ├── aodv_en_app_proto.c            (670) protocolo app-layer
│   │   └── aodv_en_app_proto.h
│   │
│   ├── tests/                             <- perfis por papel
│   │   ├── tc001/
│   │   │   ├── build_flash.sh
│   │   │   ├── node_a.defaults
│   │   │   └── node_b.defaults
│   │   ├── tc002/
│   │   │   ├── build_flash.sh
│   │   │   ├── monitor_log.sh
│   │   │   └── node_{a,b,c}.defaults
│   │   └── tc005/
│   │       ├── build_flash.sh
│   │       ├── monitor_log.sh
│   │       └── node_{a,b,c,d}.defaults
│   │
│   ├── tools/                             <- analise + dashboard
│   │   ├── extract_monitor_metrics.py     (661) parsing de log -> CSV/JSON
│   │   ├── plot_monitor_metrics.py        (213) graficos por run
│   │   ├── plot_comparison_metrics.py     (222) compara runs
│   │   ├── draw_topology.py               (349) Mermaid/DOT/SVG
│   │   ├── live_monitor.py                (700+) dashboard real-time backend
│   │   └── live_monitor_web/
│   │       ├── index.html
│   │       ├── app.css
│   │       └── app.js
│   │
│   └── (build/, logs/ - gerados, gitignored)
│
└── sim/                                   <- SIMULACAO LOCAL
    ├── README.md
    ├── run_sim.sh                         <- aceita basic|large|100|1000
    ├── aodv_en_sim.c                      (331) 3 nos A-B-C
    ├── aodv_en_sim_large.c                (265) 6 nos A-F
    ├── aodv_en_sim_100.c                  (180) grade 10x10
    └── aodv_en_sim_1000.c                 (165) grade 32x32
```

Linhas totais aproximadas: **~9.700** (sem contar build/, logs/).

---

## Camada 1 - Nucleo do protocolo (`firmware/components/aodv_en/`)

A biblioteca `aodv_en` e onde mora o protocolo de fato. Ela e portavel: o mesmo nucleo roda no firmware ESP32 e na simulacao em desktop.

### Headers publicos (`include/`)

#### `aodv_en.h` (184 linhas) - **API publica da lib**

A unica fachada que a aplicacao consome. Define:

| Tipo / funcao | O que e |
|---|---|
| `aodv_en_now_ms_fn` | callback que retorna tempo monotonico em ms |
| `aodv_en_adapter_tx_frame_fn` | callback que envia um frame no transporte |
| `aodv_en_adapter_t` | par `now_ms + tx_frame + user_ctx` |
| `aodv_en_app_callbacks_t` | `on_data` e `on_ack` que a app implementa |
| `aodv_en_stack_t` | handle opaco `{ void *impl }` |
| `aodv_en_stack_stats_t` | bloco de contadores cumulativos (16 contadores) |
| `aodv_en_route_snapshot_t` | snapshot de rota (somente leitura) |
| `aodv_en_overview_t` | resumo (count rotas/vizinhos + stats) |
| `aodv_en_stack_init/deinit` | ciclo de vida da stack |
| `aodv_en_stack_tick(_at)` | loop periodico |
| `aodv_en_stack_send_hello(_at)` | emite HELLO |
| `aodv_en_stack_send_data(_at)` | envia DATA com ACK opcional |
| `aodv_en_stack_on_recv(_at)` | entrega frame recebido |
| `aodv_en_stack_on_link_tx_result(_at)` | reporta resultado do TX |
| `aodv_en_stack_get_overview/get_route_count/get_route_at/get_stats` | leitura |

> Versoes `_at` recebem `now_ms` explicito; versoes sem `_at` chamam `now_ms()` do adapter automaticamente.

#### `aodv_en_node.h` (151 linhas) - **API do nucleo (uso interno + sim)**

Interface mais baixo nivel, usada pela fachada e diretamente pela simulacao.

| Tipo / funcao | O que e |
|---|---|
| `aodv_en_emit_frame_fn` | callback de emissao (interno) |
| `aodv_en_deliver_data_fn`, `aodv_en_ack_received_fn` | callbacks pra app |
| `aodv_en_node_callbacks_t` | bundle dos 3 callbacks acima |
| `aodv_en_stats_t` | mesmos 16 contadores (espelhado em stack_stats) |
| `aodv_en_pending_data_entry_t` | item da fila de DATA pendente |
| `aodv_en_pending_ack_entry_t` | item da fila de ACK pendente |
| `aodv_en_node_t` | a struct gigante: config, self_mac, sequencias, todas as tabelas, ambas as filas, callbacks, stats |
| `aodv_en_node_init/set_callbacks/tick` | ciclo de vida e tick |
| `aodv_en_node_send_hello/send_data` | TX |
| `aodv_en_node_on_recv/on_link_tx_result` | RX e feedback |

#### `aodv_en_messages.h` (85 linhas) - **Layout no fio**

Define todas as structs de mensagem, todas com `__attribute__((packed))`:

```c
aodv_en_header_t        14 bytes (cabecalho comum)
aodv_en_hello_msg_t     28 bytes
aodv_en_rreq_msg_t      39 bytes
aodv_en_rrep_msg_t      34 bytes
aodv_en_rerr_msg_t      24 bytes
aodv_en_ack_msg_t       30 bytes
aodv_en_data_msg_t      33 bytes header + payload[N]
```

Constantes de flags: `AODV_EN_MSG_FLAG_NONE/ACK_REQUIRED/ROUTE_REPAIR`.

Layout byte-a-byte de cada mensagem esta documentado em [aodv-en-funcionamento.md - Mensagens no fio](aodv-en-funcionamento.md#mensagens-no-fio).

#### `aodv_en_types.h` (112 linhas) - **Tipos centrais e config**

| Tipo | Uso |
|---|---|
| `aodv_en_message_type_t` | enum HELLO/RREQ/RREP/RERR/DATA/ACK |
| `aodv_en_neighbor_state_t` | enum INACTIVE/ACTIVE |
| `aodv_en_route_state_t` | enum INVALID/REVERSE/VALID |
| `aodv_en_peer_flags_t` | enum bitmask NONE/PINNED/REGISTERED |
| `aodv_en_mac_addr_t` | wrapper com 6 bytes |
| `aodv_en_neighbor_entry_t` | linha da tabela de vizinhos |
| `aodv_en_route_entry_t` | linha da tabela de rotas (com precursores) |
| `aodv_en_rreq_cache_entry_t` | linha do cache de duplicatas |
| `aodv_en_peer_cache_entry_t` | linha do cache de peers |
| `aodv_en_config_t` | struct de configuracao do no |

#### `aodv_en_status.h` (25 linhas) - **Codigos de retorno**

```c
AODV_EN_OK              =  0
AODV_EN_NOOP            =  1   // operacao sem efeito (RREQ duplicado)
AODV_EN_QUEUED          =  2   // DATA enfileirado, aguardando rota
AODV_EN_ERR_ARG         = -1
AODV_EN_ERR_FULL        = -2   // tabela ou fila lotada
AODV_EN_ERR_NOT_FOUND   = -3
AODV_EN_ERR_EXISTS      = -4
AODV_EN_ERR_NO_ROUTE    = -5
AODV_EN_ERR_SIZE        = -6
AODV_EN_ERR_PARSE       = -7
AODV_EN_ERR_STATE       = -8   // adapter ou estado interno inconsistente
```

#### `aodv_en_limits.h` (86 linhas) - **Constantes e defaults**

Toda constante magica do protocolo nasce aqui. Todas tem versao `_DEFAULT` e versao "efetiva" (que pode ser sobrescrita por `-D` no compilador).

Categorias:

```c
AODV_EN_PROTOCOL_VERSION                1
AODV_EN_MAC_ADDR_LEN                    6

// tamanhos de tabela (compile-time, arrays estaticos)
AODV_EN_NEIGHBOR_TABLE_SIZE             16
AODV_EN_ROUTE_TABLE_SIZE                32
AODV_EN_RREQ_CACHE_SIZE                 64
AODV_EN_PEER_CACHE_SIZE                 8
AODV_EN_PENDING_DATA_QUEUE_SIZE         4
AODV_EN_MAX_PRECURSORS                  4

// limites de payload
AODV_EN_CONTROL_PAYLOAD_MAX             128
AODV_EN_DATA_PAYLOAD_MAX                1024

// hops e ttl
AODV_EN_MAX_HOPS                        16
AODV_EN_TTL_DEFAULT                     16

// timers (ms)
AODV_EN_NEIGHBOR_TIMEOUT_MS             15000
AODV_EN_ROUTE_LIFETIME_MS               30000
AODV_EN_RREQ_CACHE_TIMEOUT_MS           10000
AODV_EN_ACK_TIMEOUT_MS                  1000

// retries e thresholds
AODV_EN_RREQ_RETRY_COUNT                3
AODV_EN_LINK_FAIL_THRESHOLD             3

// hysterese de troca de rota
AODV_EN_ROUTE_SWITCH_MIN_SEQ_GAIN       2
AODV_EN_ROUTE_SWITCH_MIN_METRIC_GAIN    1
AODV_EN_ROUTE_SWITCH_MIN_HOP_GAIN       1
AODV_EN_ROUTE_SWITCH_MIN_LIFETIME_GAIN_MS 5000

// metric INFINITY
AODV_EN_ROUTE_METRIC_INFINITY           0xFFFF
```

#### `aodv_en_tables.h` (38 linhas) - **Wrappers das tabelas**

Cada tabela e um array fixo + `count`:

```c
aodv_en_neighbor_table_t   { count, entries[NEIGHBOR_TABLE_SIZE] }
aodv_en_route_table_t      { count, entries[ROUTE_TABLE_SIZE] }
aodv_en_rreq_cache_t       { count, entries[RREQ_CACHE_SIZE] }
aodv_en_peer_cache_t       { count, entries[PEER_CACHE_SIZE] }
```

Os arrays sao alocados dentro de `aodv_en_node_t` (sem heap no caminho critico).

#### `aodv_en_mac.h` (23 linhas)

| Funcao | O que faz |
|---|---|
| `aodv_en_mac_equal` | compara dois MACs |
| `aodv_en_mac_is_zero` | testa MAC `00:00:00:00:00:00` |
| `aodv_en_mac_is_broadcast` | testa MAC `FF:FF:FF:FF:FF:FF` |
| `aodv_en_mac_copy` | memcpy de 6 bytes |
| `aodv_en_mac_clear` | zera 6 bytes |

#### `aodv_en_neighbors.h` (50 linhas)

API de operacoes na tabela de vizinhos. Detalhes na implementacao em `.c`.

#### `aodv_en_routes.h` (57 linhas)

API de operacoes na tabela de rotas + precursores.

#### `aodv_en_rreq_cache.h` (36 linhas)

API do cache de duplicatas de RREQ.

#### `aodv_en_peers.h` (45 linhas)

API do cache LRU de peers ESP-NOW.

---

### Implementacao (`src/`)

#### `aodv_en_node.c` (1.658 linhas) - **O coracao do protocolo**

Este e o arquivo mais denso. Vou mapear por blocos de linha aproximada:

| Linha | Bloco | O que faz |
|---|---|---|
| 1-7 | imports + constantes | inclui headers; define `BROADCAST_MAC` |
| 9-17 | forward declarations | `send_rreq` e `flush_pending_data_for_destination` |
| 19-56 | helpers de checagem | `is_self`, `route_is_usable`, `find_usable_route` |
| 58-97 | `aodv_en_node_emit` | wrapper de emissao via callback (atualiza peer cache + neighbor mark_used) |
| 99-113 | `aodv_en_fill_header` | escreve cabecalho comum em qualquer mensagem |
| 115-204 | helpers de pending_data | `clear`, `find_free`, `find_oldest`, `find_oldest_for_destination` |
| 206-250 | `send_data_with_sequence` | monta frame DATA com sequence_number e emite |
| 252-311 | helpers de pending_ack | `clear`, `find_free`, `find_oldest` |
| 313-359 | `track_pending_ack` | adiciona DATA na fila de ACK pendente |
| 361-397 | `pending_ack_consume` | consome (remove) entrada quando ACK chega |
| 399-447 | `send_data_via_route` | combina send + track_pending_ack |
| 449-518 | `queue_data` | enfileira DATA com **backpressure** (recicla mais antigo do mesmo destino) |
| 520-547 | `pending_note_rreq_attempt` | marca tentativa de descoberta numa entrada pendente |
| 549-572 | `pending_destination_seen` | dedup helper para evitar disparar 2 RREQs do mesmo destino |
| 574-614 | `discovery_retry_interval_ms` | **backoff exponencial** sobre `ack_timeout_ms` ate teto de 10s |
| 616-660 | `trigger_discovery_for_pending_destinations` | dispara RREQ pra todos os destinos pendentes (usado apos quebra) |
| 662-714 | `retry_route_discovery_for_pending` | reemite RREQ respeitando backoff (chamado no tick) |
| 716-767 | `flush_pending_data_for_destination` | drena fila quando rota e instalada |
| 769-801 | `expire_pending_data` | remove entradas que excedem `route_lifetime_ms` |
| 803-878 | `process_pending_ack_retries` | retransmite DATA pendente apos timeout, reduz `retries_left` |
| 880-909 | **`send_rreq`** | emite RREQ broadcast, incrementa `self_seq_num` e `next_rreq_id`, registra no cache |
| 911-944 | **`send_rrep`** | regras RFC 3561 §6.6.1 para `self_seq_num` + envia unicast pelo caminho reverso |
| 946-966 | **`send_rerr`** | emite RERR de 1 destino (unicast ou broadcast) |
| 968-1000 | **`notify_precursors_of_break`** | propaga RERR para precursores excluindo o `next_hop` quebrado; broadcast como fallback |
| 1002-1023 | **`send_ack`** | emite ACK pelo caminho reverso |
| 1025-1042 | `forward_rreq` | repassa RREQ com `hop_count++` e `ttl--` |
| 1044-1062 | `forward_rrep` | repassa RREP pelo caminho reverso |
| 1064-1098 | `forward_data` | repassa DATA, decrementa TTL, **adiciona precursor** (RFC 3561 §6.6.2) |
| 1100-1118 | **`handle_hello`** | instala vizinho + rota direta de 1 salto |
| 1120-1162 | **`handle_rreq`** | dedup por cache, instala rota reversa, responde se for o destino, senao reencaminha |
| 1164-1211 | **`handle_rrep`** | instala rota direta VALID, **mantem precursores** (3 atualizacoes RFC 3561 §6.6.2), drena pending data |
| 1213-1239 | **`handle_rerr`** | invalida rota local e propaga para precursores excluindo o sender |
| 1241-1273 | **`handle_ack`** | consome pending_ack ou repassa unicast |
| 1275-1310 | **`handle_data`** | entrega ao callback se for destino + envia ACK; senao repassa |
| 1312-1356 | `validate_header`, `validate_message_size` | sanity check antes de despachar |
| 1358-1380 | `aodv_en_config_set_defaults` | popula `aodv_en_config_t` com defaults |
| 1382-1409 | `aodv_en_node_init` | zera tabelas, copia config, copia self_mac |
| 1411-1427 | `aodv_en_node_set_callbacks` | injeta callbacks |
| 1429-1444 | `aodv_en_node_tick` | expiracao de vizinhos/rotas/cache + retry route discovery + retry ACK + expira pending data |
| 1446-1533 | `aodv_en_node_on_link_tx_result` | conta falhas de unicast, marca vizinho INACTIVE, invalida rotas em cascata, notifica precursores |
| 1535-1552 | `aodv_en_node_send_hello` | monta e emite HELLO |
| 1554-1610 | `aodv_en_node_send_data` | tenta enviar; se sem rota: enfileira + dispara RREQ + retorna `AODV_EN_QUEUED` |
| 1612-1658 | `aodv_en_node_on_recv` | valida frame, atualiza vizinho, dispatch para `handle_*` por tipo |

#### `aodv_en_routes.c` (352 linhas) - **Tabela de rotas + precursores**

| Linha | Funcao | O que faz |
|---|---|---|
| 7-23 | `route_remove_at` (static) | shift do array |
| 25-35 | `next_hop_changed` | helper para hysterese |
| 37-82 | **`candidate_is_strongly_better`** | verifica ganho forte para evitar flap (4 criterios: seq, metric, hop, lifetime) |
| 84-92 | `aodv_en_route_table_init` | zera tabela |
| 94-114 | `aodv_en_route_find` | busca linear por destination |
| 116-121 | `find_const` | wrapper const |
| 123-135 | `find_valid` | retorna so se `state == VALID` |
| 137-201 | **`should_replace`** | decisao de upsert: estado, hysterese, seq_num, metric, hop_count, expires_at_ms (ordem de prioridade) |
| 203-251 | **`aodv_en_route_upsert`** | insere ou substitui; **preserva precursores quando next_hop e o mesmo**, reseta quando muda |
| 253-279 | **`aodv_en_route_add_precursor`** | adiciona MAC ao array de precursores (idempotente, retorna FULL se cheio) |
| 281-298 | `aodv_en_route_invalidate_destination` | marca state=INVALID, metric=INFINITY |
| 300-325 | `aodv_en_route_invalidate_by_next_hop` | invalida em massa quando vizinho cai |
| 327-352 | `aodv_en_route_expire` | remove entradas com `expires_at_ms <= now_ms` |

> **Hysterese explicada**: quando uma candidata `VALID` chega para um destino que ja tem rota `VALID` mas com `next_hop` diferente, `should_replace` exige que `candidate_is_strongly_better` retorne true. Isso evita "flapping" entre caminhos quase equivalentes.

#### `aodv_en_neighbors.c` (203 linhas) - **Tabela de vizinhos**

| Funcao | O que faz |
|---|---|
| `neighbor_remove_at` (static) | shift do array |
| `neighbor_weighted_rssi` (static) | **EMA**: `(3 * old + new) / 4` |
| `neighbor_table_init` | zera |
| `aodv_en_neighbor_find` (+const) | busca por MAC |
| **`aodv_en_neighbor_touch`** | atualiza ou cria vizinho com EMA do RSSI; se vinha de INACTIVE, **re-inicializa o EMA** com o RSSI atual; sempre marca ACTIVE e zera `link_fail_count` |
| `aodv_en_neighbor_mark_used` | apenas atualiza `last_used_ms` |
| **`aodv_en_neighbor_note_link_failure`** | incrementa `link_fail_count`; ao atingir threshold, marca INACTIVE |
| `aodv_en_neighbor_expire` | remove entradas com `last_seen_ms` antigo demais |
| `aodv_en_neighbor_count_active` | conta vizinhos ACTIVE |

#### `aodv_en_peers.c` (209 linhas) - **Cache LRU de peers ESP-NOW**

Independente da tabela de vizinhos. Existe pra controlar quantos peers estao registrados no driver `ESP-NOW` simultaneamente (limite de hardware).

| Funcao | O que faz |
|---|---|
| `peer_find_index` (static) | busca posicao por MAC |
| **`peer_find_lru_index`** (static) | encontra menos-recente, **pulando entradas com `PINNED`** |
| `peer_remove_at` (static) | shift |
| `aodv_en_peer_cache_init` | zera |
| `aodv_en_peer_find` (+const) | busca |
| **`aodv_en_peer_touch`** | atualiza `last_used_ms`; se cache cheio, evicta LRU nao-pinned |
| `aodv_en_peer_set_registered/set_pinned` | flips de flags |
| `aodv_en_peer_remove` | delete explicito |

#### `aodv_en_rreq_cache.c` (150 linhas) - **Cache de duplicatas RREQ**

| Funcao | O que faz |
|---|---|
| `rreq_cache_find_index` (static) | busca por `(originator, rreq_id)` |
| `oldest_index` (static) | indice da entrada mais antiga por `created_at_ms` |
| `remove_at` (static) | shift |
| `aodv_en_rreq_cache_init` | zera |
| `aodv_en_rreq_cache_contains` | bool de existencia |
| **`aodv_en_rreq_cache_remember`** | insere; **se cheio, substitui o mais antigo** (politica de eviction documentada na spec v1) |
| `aodv_en_rreq_cache_expire` | remove entradas com idade `> rreq_cache_timeout_ms` |

#### `aodv_en_mac.c` (59 linhas) - **Utilitarios de MAC**

5 funcoes simples (compare, is_zero, is_broadcast, copy, clear). Reutilizadas em todos os modulos.

---

## Camada 2 - Fachada da lib (`aodv_en.c`)

### `aodv_en.c` (537 linhas) - **`aodv_en_stack_*` API**

A camada que a aplicacao consome. Esconde o `aodv_en_node_t` por tras de um handle opaco.

```c
typedef struct {
    aodv_en_node_t          node;
    aodv_en_adapter_t       adapter;       // now_ms + tx_frame
    aodv_en_app_callbacks_t app_callbacks; // on_data + on_ack
} aodv_en_stack_impl_t;
```

| Bloco | O que faz |
|---|---|
| linhas 8-33 | helpers para casting `stack_t* -> stack_impl_t*` |
| 35-60 | `stats_from_node` - copia 16 contadores do node para o snapshot |
| 62-82 | `stack_emit_frame` - bridge entre `aodv_en_emit_frame_fn` (interno) e `adapter.tx_frame` (externo) |
| 84-120 | `stack_deliver_data` e `stack_ack_received` - bridges para callbacks da app |
| 122-138 | `stack_now_ms` - helper que chama o `adapter.now_ms` com guard de NULL |
| 140-194 | `aodv_en_stack_init` - **calloc()** unico para `stack_impl_t`, copia adapter+app_callbacks, inicializa `node` interno e injeta os bridges |
| 196-208 | `stack_deinit` - `free` |
| 210-228 | `set_app_callbacks` - troca callbacks em runtime |
| 230-264 | `tick(_at)` - delega para `aodv_en_node_tick` |
| 266-299 | `send_hello(_at)` |
| 301-354 | `send_data(_at)` |
| 356-409 | `on_recv(_at)` |
| 411-460 | `on_link_tx_result(_at)` |
| 462-478 | `get_overview` - retorna count + stats |
| 480-491 | `get_route_count` |
| 493-521 | **`get_route_at`** - copia para `aodv_en_route_snapshot_t` (so leitura) |
| 523-537 | `get_stats` |

> **Heap usage**: a fachada usa `calloc` apenas no init para alocar `stack_impl_t` (~600 bytes). Tudo mais e estatico dentro da struct. Caminho critico (`tick`, `on_recv`) nao usa heap.

---

## Camada 3 - Aplicacao firmware (`firmware/main/`)

### `main.c` (10 linhas) - **Entry point**

```c
#include "sdkconfig.h"
#include "app_demo.h"
#include "app_proto_example.h"

void app_main(void) {
#ifdef CONFIG_AODV_EN_APP_USE_APP_DEMO
    app_demo_run();
#else
    app_proto_example_run();
#endif
}
```

A escolha vem do `Kconfig.projbuild` (menuconfig), com default em `app_demo` (conservador, mais simples).

### `app_demo.c` (555 linhas) - **App legado simples**

App de bancada minimal. Emite HELLO periodico e opcionalmente DATA periodico para um MAC alvo.

| Bloco | O que faz |
|---|---|
| 1-30 | imports + macros (`APP_RX_QUEUE_LEN=8`, `APP_TX_RESULT_QUEUE_LEN=16`, `APP_LOOP_DELAY_MS=100`) |
| 38-50 | structs de eventos: `app_rx_event_t` (frame recebido), `app_tx_result_event_t` (resultado de TX) |
| 52-72 | `app_context_t` - guarda stack, queues, configs, agendamento de proximos eventos |
| 75-104 | helpers: `app_now_ms` (esp_timer), `app_format_mac`, `app_parse_mac`, `app_mac_is_broadcast` |
| 153-169 | `app_ensure_peer` - registra peer no driver ESP-NOW se ainda nao esta |
| 171-205 | **`app_emit_frame`** - implementacao do `tx_frame` do adapter; chama `esp_now_send` |
| 207-230 | `app_deliver_data`, `app_ack_received` - callbacks da app |
| 232-262 | `app_send_cb` - callback do ESP-NOW (resultado de tx); empurra para queue |
| 264-288 | `app_recv_cb` - callback do ESP-NOW (frame recebido); empurra para queue |
| 290-332 | `app_log_routes` - dump periodico de stats e tabela de rotas no log |
| 334-376 | `process_rx_queue`, `process_tx_result_queue` - drena queues no contexto da task |
| 378-435 | **`app_protocol_task`** - loop principal: drain queues, tick, send hello, send data, log |
| 437-477 | `app_init_nvs`, `app_init_wifi`, `app_init_espnow` - boilerplate ESP-IDF |
| 479-555 | **`app_demo_run`** - entry point: setup tudo + cria task |

### `app_proto_example.c` (1.282 linhas) - **App com CLI + protocolo de aplicacao**

Mesmo esqueleto do `app_demo.c`, mais:

- camada de aplicacao tipada (`aodv_en_app_proto_t`) embutida no contexto
- task adicional `app_cli_task` que le `stdin` e despacha comandos
- handlers `cmd_ping`, `cmd_echo`, `cmd_info`
- broadcast periodico de `HEALTH_REQ`
- envio periodico opcional de `TEXT` e `CMD_REQ` para um target

| Bloco | O que faz |
|---|---|
| 1-65 | imports + macros + flags Kconfig (`APP_PROTO_ENABLE_UNICAST`, `APP_PROTO_ENABLE_CLI`) |
| 65-120 | structs: `app_rx_event_t`, `app_tx_result_event_t`, `app_cli_event_t`, `app_context_t` (com `aodv_en_app_proto_t` embutido) |
| 124-200 | helpers de mac formatting e parsing |
| 202-237 | helpers de string e `app_log_cli_help` |
| 239-401 | **`app_cli_parse_line`** - parse `help`, `routes`, `health all|<mac>`, `text <mac> <msg>`, `cmd <mac> <comando> [args]` |
| 403-455 | `app_emit_frame` - mesmo do app_demo |
| 457-479 | bridges: `app_proto_send_data` (`send_data` do transport do app_proto chama `aodv_en_stack_send_data_at`) e `app_proto_now_ms` |
| 481-561 | callbacks app_proto: `on_health_req`, `on_health_rsp`, `on_text`, `on_cmd_req`, `on_cmd_rsp` (logging) |
| 563-648 | **handlers de comandos**: `cmd_ping`, `cmd_echo`, `cmd_info` |
| 650-686 | `app_stack_on_data` (passa pro app_proto), `app_stack_on_ack` |
| 688-744 | callbacks ESP-NOW (send_cb, recv_cb) |
| 746-832 | `app_log_routes`, `process_rx_queue`, `process_tx_result_queue` |
| 834-940 | **`app_process_cli_queue`** - dispatcher para os comandos digitados |
| 942-1009 | **`app_cli_task`** - le stdin caractere por caractere, monta linhas, parse e enfileira |
| 1011-1122 | **`app_protocol_task`** - loop principal: drain queues, tick, hello, health_req, text/cmd opcional, log |
| 1124-1164 | init_nvs, init_wifi, init_espnow |
| 1166-1282 | **`app_proto_example_run`** - setup + 2 tasks (`app_protocol_task` + `app_cli_task`) |

### `aodv_en_app_proto.h` (161 linhas) - **API do protocolo de aplicacao**

Define o protocolo app-layer que roda **acima** do AODV-EN. Mensagens:

```c
HEALTH_REQ = 1   // ping de saude (broadcast ou unicast)
HEALTH_RSP = 2   // resposta automatica do node
TEXT       = 3   // mensagem de texto livre
CMD_REQ    = 4   // requisicao de comando
CMD_RSP    = 5   // resposta de comando
```

| Tipo | Uso |
|---|---|
| `aodv_en_app_proto_t` | struct com node_name, request_id sequencial, transport, callbacks, comandos registrados |
| `aodv_en_app_proto_transport_t` | par `send_data` + `now_ms` (injetado pela aplicacao) |
| `aodv_en_app_proto_callbacks_t` | callbacks por tipo de mensagem |
| `aodv_en_app_proto_command_handler_fn` | assinatura de handler de comando custom |
| funcoes `_init`, `_set_callbacks`, `_register_command`, `_send_health_req`, `_send_text`, `_send_command`, `_on_mesh_payload` | API de uso |

### `aodv_en_app_proto.c` (670 linhas) - **Implementacao do protocolo de aplicacao**

| Bloco | O que faz |
|---|---|
| 7-43 | helpers de leitura/escrita LE de uint16 e uint32 |
| 45-67 | `next_request_id` (incremento idempotente que pula 0) e `now_ms` |
| 69-113 | **`send_frame`** - monta header de 8 bytes (`version|type|body_len|request_id`) + body, e chama `transport.send_data` |
| 115-149 | **`decode`** - valida e parseia header de 8 bytes |
| 151-178 | `copy_text` - extrai string com size guard |
| 180-205 | `find_command` - busca handler por nome |
| 207-237 | `send_health_rsp` - resposta automatica com `node=NAME uptime_ms=N` |
| 239-330 | **`handle_cmd_req`** - parse `cmd_len|cmd|args_len|args`, despacha handler ou retorna erro 404, monta CMD_RSP |
| 332-380 | `init`, `set_callbacks` |
| 382-431 | **`register_command`** - adiciona ao array de comandos (max 8) |
| 433-500 | `send_health_req`, `send_text` |
| 502-563 | **`send_command`** - encapsula `cmd|args` no body |
| 565-670 | **`on_mesh_payload`** - dispatcher por tipo de mensagem (health_req/rsp, text, cmd_req, cmd_rsp) |

> Esta camada **nao faz parte do AODV-EN**. E uma aplicacao que consome a malha. A spec v1 afirma isso explicitamente. Pode ser substituida por qualquer outra app sem afetar o protocolo.

---

## Camada 4 - Build system

### `firmware/CMakeLists.txt` (4 linhas)

Project root. Inclui o `project.cmake` do ESP-IDF e declara `project(aodv_en_firmware)`.

### `firmware/main/CMakeLists.txt` (9 linhas)

Registra o componente `main` com 4 sources (`main.c`, `app_demo.c`, `app_proto_example.c`, `aodv_en_app_proto.c`) e dependencia em `aodv_en` + componentes ESP-IDF (`esp_wifi`, `nvs_flash`, `esp_event`, `esp_netif`, `esp_timer`).

### `firmware/components/aodv_en/CMakeLists.txt` (12 linhas)

Registra o componente `aodv_en` com 7 sources (todas as `.c` em `src/`) e `INCLUDE_DIRS "include"`. Sem dependencias de ESP-IDF (e portavel).

### `firmware/main/Kconfig.projbuild` (99 linhas)

Define o menu `AODV-EN App` no `idf.py menuconfig`. Campos:

| Config | Range/default |
|---|---|
| `AODV_EN_APP_EXAMPLE_MODE` | choice: `app_demo` (default) ou `app_proto_example` |
| `AODV_EN_APP_NODE_NAME` | string default `NODE_A` |
| `AODV_EN_APP_NETWORK_ID` | hex default `0xA0DE0001` |
| `AODV_EN_APP_WIFI_CHANNEL` | int 1-13 default 6 |
| `AODV_EN_APP_HELLO_INTERVAL_MS` | int 1000-60000 default 5000 |
| `AODV_EN_APP_SEND_INTERVAL_MS` | int 1000-60000 default 7000 |
| `AODV_EN_APP_PRINT_INTERVAL_MS` | int 1000-60000 default 10000 |
| `AODV_EN_APP_ENABLE_DATA` | bool default n |
| `AODV_EN_APP_TARGET_MAC`, `_PAYLOAD_TEXT` | string (so quando ENABLE_DATA=y) |
| `AODV_EN_APP_PROTO_HEALTH_INTERVAL_MS`, `_UNICAST_INTERVAL_MS`, `_ENABLE_UNICAST`, `_ENABLE_CLI`, `_TARGET_MAC`, `_TEXT`, `_COMMAND`, `_COMMAND_ARGS` | configs do app_proto_example |

### `firmware/sdkconfig.defaults` (2 linhas)

Configura `ESPTOOLPY_FLASHSIZE=4MB`. So.

---

## Camada 5 - Scripts

### Build / flash / monitor (`firmware/`)

| Script | O que faz |
|---|---|
| `build.sh` (11) | source `idf-env.sh` + `cd firmware` + `idf.py set-target esp32` + `idf.py build` |
| `flash_monitor.sh` (16) | recebe porta + flash + monitor numa task |
| `monitor_log.sh` (107) | versao mais elaborada: tee para arquivo em `firmware/logs/serial/<label>_<timestamp>.log` |
| `idf-env.sh` (59) | bootstrap ESP-IDF (busca em `ESP_IDF_EXPORT`, `IDF_PATH/export.sh`, `~/esp/esp-idf/export.sh`, ou `idf.py` no PATH) |

### Por TC (`firmware/tests/tcXXX/`)

| Script | O que faz |
|---|---|
| `tc001/build_flash.sh` | recebe `node_a|node_b <PORT>` e (so para a) `<TARGET_MAC>`. Builda em `build/tc001_node_X/` e flasha. |
| `tc002/build_flash.sh` | mesma logica para 3 papeis |
| `tc002/monitor_log.sh` | wrapper que chama `firmware/monitor_log.sh -B build/tc002_node_X -l node_X -t <run_tag>` |
| `tc005/build_flash.sh` | mesma logica para 4 papeis |
| `tc005/monitor_log.sh` | wrapper para os 4 |
| `tcXXX/node_*.defaults` | overrides de Kconfig (network_id, channel, intervals, ENABLE_DATA, payload_text). Aplicados via `SDKCONFIG_DEFAULTS=...defaults;perfil` |

> **Nota importante**: `build_flash.sh` faz `cd "$FW_DIR"` antes de invocar `idf.py` (foi um fix recente). Sem isso, `idf.py` procura `CMakeLists.txt` no cwd do invocador.

---

## Camada 6 - Simulacao (`sim/`)

A simulacao roda o **mesmo nucleo** do firmware (lib `aodv_en` em C) com mocks de transporte. Compila com `cc` em desktop, sem ESP-IDF.

### `aodv_en_sim.c` (331 linhas) - **Simulacao basica 3 nos**

Cenario A-B-C linear. Validation completa do fluxo:

| Fase | O que valida |
|---|---|
| route discovery | A envia DATA sem rota -> retorna `AODV_EN_QUEUED`; verifica rotas instaladas em A e B |
| data | segundo `send_data` retorna OK; verifica `delivered_frames` em C e `ack_received` em A |
| ack retry | seta `drop_next_ack_to_a`; verifica que `ack_retry_sent` cresce e ACK chega depois |
| late join retry discovery | quebra link B-C, invalida rotas, espera retries de RREQ; reativa link e verifica entrega |

Estrutura:

```c
typedef struct sim_network {
    uint32_t now_ms;
    bool links[3][3];                 // matriz de conectividade
    bool drop_next_ack_to_a;          // injecao de falha
    uint8_t macs[3][6];
    aodv_en_node_t nodes[3];
    sim_endpoint_t endpoints[3];
} sim_network_t;
```

Funcoes:

- `sim_emit_frame` - simula radio: broadcast entrega para todos os links ativos; unicast verifica link
- `sim_deliver_data`, `sim_ack_received` - logging
- `sim_init_network` - configura 3 nos com MACs e callbacks
- `sim_tick_all` - chama `aodv_en_node_tick` em todos
- `main` - orquestra as 4 fases com asserts

### `aodv_en_sim_large.c` (265 linhas)

6 nos em cadeia A-B-C-D-E-F. Foco em propagacao de RERR. Quebra link D-E e verifica que RERR propaga corretamente para A.

### `aodv_en_sim_100.c` (180 linhas)

Grade 10x10 (100 nos). Tenta descobrir rota de (0,0) ate (9,9). Quebra coluna central (parede) e verifica reconvergencia em torno.

### `aodv_en_sim_1000.c` (165 linhas)

Grade 32x32 (1024 nos) com 5% de falhas aleatorias. Cenario "smart city". Verifica robustez em escala.

### `run_sim.sh` (60 linhas)

Aceita argumento opcional: `basic` (default), `large`, `100`, `1000`. Compila o `.c` correspondente com `cc -std=c11 -Wall -Wextra` em `/tmp/aodv_en_sim_<variant>` e executa.

---

## Camada 7 - Ferramentas de analise (`firmware/tools/`)

### `extract_monitor_metrics.py` (661 linhas) - **Parsing de log -> CSV/JSON**

Le um arquivo `.log` do `idf.py monitor` e produz um diretorio de analise com:

- `summary.json` - metricas agregadas
- `summary.txt` - mesma coisa em texto
- `events.csv` - todos os eventos parseados (timeline)
- `snapshots.csv` - snapshots periodicos com tx/rx/delivered deltas
- `routes.csv` - linhas de rota associadas a cada snapshot
- `target_route_series.csv` - estado da rota para o destino alvo ao longo do tempo
- `target_route_transitions.csv` - mudancas de estado (flap detection)
- `ack_events.csv`, `fail_events.csv`, `data_status_events.csv`
- `discovery_windows.csv` - intervalos abertos por `data_queued_discovery` e fechados por `ack_received`
- `minute_metrics.csv` - bucketizado por minuto (ack/min, fail/min, hops_1, hops_2, etc.)

Regexes principais (linhas 11-25):

```python
LOG_RE       = r"^\s*([IWE]) \((\d+)\) ([^:]+): (.*)$"
NODE_RE      = r"node=([A-Z0-9_]+)\s+self_mac=...\s+channel=(\d+)\s+network_id=0x(...)"
SNAPSHOT_RE  = r"routes=(\d+)\s+neighbors=(\d+)\s+tx=(\d+)\s+rx=(\d+)\s+delivered=(\d+)"
ROUTE_RE     = r"route\[(\d+)\]\s+dest=...\s+via=...\s+hops=(\d+)\s+metric=(\d+)\s+state=(\d+)\s+expires=(\d+)"
ACK_RE       = r"ACK received from ... for seq=(\d+)"
DELIVER_RE   = r"DATA deliver from ...:"
SEND_FAIL_RE = r"ESP-NOW send fail to ..."
DATA_STATUS_RE = r"DATA send status=(-?\d+)"
INVALIDATED_RE = r"invalidated (\d+) route\(s\) via ..."
```

Estas mesmas regexes sao reusadas pelo `live_monitor.py`.

### `plot_monitor_metrics.py` (213 linhas) - **Graficos por run**

Gera 7 PNGs em `<analysis_dir>/plots/`:

1. `01_ack_vs_fail_per_minute.png`
2. `02_queue_and_status_per_minute.png`
3. `03_route_state_timeline.png`
4. `04_route_state_distribution.png`
5. `05_neighbors_routes_per_minute.png`
6. `06_discovery_window_durations.png`
7. `07_tx_rx_delta_over_time.png`

### `plot_comparison_metrics.py` (222 linhas) - **Compara runs**

Recebe varios `--run label::summary.json` e plota:

1. `01_core_metrics.png` - ACK/min, fail/min, status_neg2/min, queued_discovery/min
2. `02_stability_metrics.png` - flap/h, target absent ratio, max blackout
3. `03_route_profile.png` - distribuicao de hops_1/hops_2/absent

Suporta marcador `--change-index N` que desenha linha vertical entre runs "pre" e "pos" mudanca, util para A/B test antes/depois de fix.

### `draw_topology.py` (349 linhas) - **Mermaid + DOT + SVG**

Recebe um ou mais diretorios de analise e gera:

- `topology.mmd` - Mermaid (renderizavel em GitHub)
- `topology.dot` - Graphviz DOT
- `topology.svg` - se `dot` estiver instalado
- `topology.json` - resumo estruturado (nos + edges + contextos)

Modo `--mode latest` usa o ultimo snapshot de cada nó. Modo `--mode observed` faz uniao de todas as edges observadas.

### `live_monitor.py` (~700 linhas) - **Dashboard real-time backend**

Componentes:

| Bloco | O que faz |
|---|---|
| `find_esptool`, `read_mac_via_esptool` | descobre MAC sem flashar (resolve alias->mac no startup) |
| Regexes (`LOG_RE`, `NODE_RE`, etc.) | mesmas do `extract_monitor_metrics.py` |
| `MeshState` | dicionario de nos + alias_to_mac + ring buffer de eventos |
| `Hub` | mantem subscribers WS + broadcast |
| **`LineParser`** | feed por linha; emite eventos (`node_seen`, `stats`, `routes`, `ack`, `deliver`, `send_fail`, `data_status`, `invalidated`, `queued_route`, `queued_discovery`); modo verbose imprime cada evento com cores ANSI |
| `SerialPump` | thread por porta; `pyserial.Serial.readline()` blocking; backoff exponencial em desconexao; tee opcional pra stdout em `-vv` |
| `serial_loop` | drena queue do `SerialPump` para `LineParser` |
| `demo_loop` | gera eventos sinteticos de uma malha A-B-C-D para teste sem hardware |
| `websocket_handler` | aiohttp WS - manda snapshot inicial + push de eventos |
| `index_handler`, `make_app` | aiohttp HTTP serve `live_monitor_web/` |
| `amain` | orquestra: pre-le MACs, abre serials, sobe HTTP+WS |

### `live_monitor_web/` - **Dashboard frontend**

| Arquivo | O que faz |
|---|---|
| `index.html` | layout grid: graph card a esquerda + sidebar (resumo, lista de nos, eventos recentes) |
| `app.css` | estilos: paleta indigo/verde/laranja, console preto, animacoes pulse/flash |
| `app.js` | WebSocket client + Cytoscape.js: applyEvent dispatcha por tipo, atualiza state/sidebar/grafo, anima pulsePath em ACK, flash em send_fail/invalidated, ghost online quando rota e VALID |

---

## Camada 8 - Testes funcionais

### Documentos (`docs/tests/`)

| TC | Cenario | Cobre criterios da spec v1 |
|---|---|---|
| `tc-001-descoberta-e-entrega-direta.md` | 2 nos diretos | 1 |
| `tc-002-primeiro-multi-hop.md` | 3 nos cadeia | 2, 5 |
| `tc-003-reconvergencia-apos-falha.md` | 3 nos com falha intermediaria | 3, 7 |
| `tc-004-soak-estabilidade-e-reconvergencia.md` | 3 nos sob ciclos por 30 min | 4, 6 |
| `tc-005-cadeia-4-nos.md` | 4 nos cadeia (3 saltos) | 2 estendido |

Cada TC segue a estrutura definida em [docs/tests/README.md](tests/README.md):

1. Objetivo
2. Topologia
3. Configuracao dos nos (com referencia ao perfil `.defaults`)
4. Procedimento (com comandos prontos)
5. Evidencias esperadas
6. Criterio de aprovacao
7. Resultado (preencher a cada execucao)

### Perfis (`firmware/tests/`)

Veja Camada 5 - Scripts.

---

## Camada 9 - Documentacao (`docs/`)

| Arquivo | Tipo | Quem le |
|---|---|---|
| `aodv-en-spec-v1.md` | normativo | quem quer saber o que o protocolo deve fazer |
| `aodv-en-funcionamento.md` | tutorial | quem quer entender como funciona, com worked examples |
| `aodv-en-mapa-do-codigo.md` | guia de codigo | voce esta aqui - estudo do codigo |
| `aodv-en-estruturas-dados.md` | referencia | layout das tabelas e mensagens |
| `aodv-base-invariantes.md` | normativo | regras do AODV que nao podem ser quebradas |
| `aodv-en-spec-v0.md` | OBSOLETA | so registro historico |
| `plano-desenvolvimento-completo.md` | gestao | roadmap das fases do TCC |
| `runbook-bancada.md` | operacional | passo a passo de bancada |
| `features/enfilaremento-dos-dados.md` | feature integrada na v1 | fila pendente de DATA |
| `features/precursores.md` | feature integrada na v1 | precursores e RERR direcionado |
| `features/articulation-point-planejado.md` | feature de v2 | detecao de no de corte |
| `tests/*` | casos de teste | bancada |

---

## Mapa por feature

A pergunta tipica: "**onde esta implementado X?**". Tabela cruzada:

| Feature | Estado | Onde encontrar | Comentario |
|---|---|---|---|
| Cabecalho comum 14 bytes | implementado | [aodv_en_messages.h](../firmware/components/aodv_en/include/aodv_en_messages.h) struct `aodv_en_header_t` | preenchido por `aodv_en_fill_header` em `aodv_en_node.c` |
| HELLO emissao | implementado | [aodv_en_node.c:1535](../firmware/components/aodv_en/src/aodv_en_node.c) `aodv_en_node_send_hello` | chamado a cada `hello_interval_ms` pela app |
| HELLO recepcao | implementado | [aodv_en_node.c:1100](../firmware/components/aodv_en/src/aodv_en_node.c) `handle_hello` | instala vizinho + rota direta |
| **Descoberta passiva de vizinhos** | implementado | [aodv_en_node.c:1612](../firmware/components/aodv_en/src/aodv_en_node.c) `aodv_en_node_on_recv` linha `aodv_en_neighbor_touch(...)` | qualquer frame valido toca o vizinho |
| RREQ emissao | implementado | [aodv_en_node.c:880](../firmware/components/aodv_en/src/aodv_en_node.c) `send_rreq` | incrementa `self_seq_num` e `next_rreq_id` |
| **Dedup de RREQ** | implementado | [aodv_en_node.c:1120](../firmware/components/aodv_en/src/aodv_en_node.c) `handle_rreq` checa `aodv_en_rreq_cache_contains` | cache em [aodv_en_rreq_cache.c](../firmware/components/aodv_en/src/aodv_en_rreq_cache.c) |
| RREP emissao (RFC 3561 §6.6.1) | implementado | [aodv_en_node.c:911](../firmware/components/aodv_en/src/aodv_en_node.c) `send_rrep` | regras de `self_seq_num` corretas |
| **Precursores** (RFC 3561 §6.6.2) | implementado | [aodv_en_node.c:1164](../firmware/components/aodv_en/src/aodv_en_node.c) `handle_rrep`; storage em `aodv_en_route_entry_t.precursors` | 3 atualizacoes: receive, forward target, reverse |
| RERR emissao | implementado | [aodv_en_node.c:946](../firmware/components/aodv_en/src/aodv_en_node.c) `send_rerr` | 1 destino por RERR em v1 |
| **RERR direcionado a precursores** | implementado | [aodv_en_node.c:968](../firmware/components/aodv_en/src/aodv_en_node.c) `notify_precursors_of_break` | broadcast como fallback se `precursor_count == 0` |
| Encaminhamento de DATA | implementado | [aodv_en_node.c:1064](../firmware/components/aodv_en/src/aodv_en_node.c) `forward_data` | adiciona `link_src_mac` como precursor (RFC 3561) |
| ACK fim a fim | implementado | [aodv_en_node.c:1002, 1241](../firmware/components/aodv_en/src/aodv_en_node.c) `send_ack`, `handle_ack` | dispara quando `flags & ACK_REQUIRED` |
| **Fila de DATA pendente** | implementado | [aodv_en_node.c:449](../firmware/components/aodv_en/src/aodv_en_node.c) `queue_data` (com backpressure) | drena em `flush_pending_data_for_destination` ao instalar rota VALID |
| **Retry exponencial de RREQ** | implementado | [aodv_en_node.c:574](../firmware/components/aodv_en/src/aodv_en_node.c) `discovery_retry_interval_ms` | backoff `ack_timeout_ms * 2^attempt` ate 10s |
| **Fila de ACK pendente + retry** | implementado | [aodv_en_node.c:803](../firmware/components/aodv_en/src/aodv_en_node.c) `process_pending_ack_retries` | `retries_left` compartilhado com `rreq_retry_count` |
| **Hysterese de troca de rota** | implementado | [aodv_en_routes.c:37](../firmware/components/aodv_en/src/aodv_en_routes.c) `candidate_is_strongly_better` | 4 ganhos (seq, metric, hop, lifetime) |
| **Selecao de rota** | implementado | [aodv_en_routes.c:137](../firmware/components/aodv_en/src/aodv_en_routes.c) `should_replace` | priority: state > hysterese > seq > metric > hop > lifetime |
| **Suavizacao EMA do RSSI** | implementado | [aodv_en_neighbors.c:25](../firmware/components/aodv_en/src/aodv_en_neighbors.c) `weighted_rssi` | `(3*old + new) / 4`; reset em INACTIVE->ACTIVE |
| **Cascading de falha de vizinho** | implementado | [aodv_en_node.c:1446](../firmware/components/aodv_en/src/aodv_en_node.c) `on_link_tx_result` | `link_fail_count >= threshold` -> INACTIVE -> invalida rotas + notify precursores |
| Expiracao de rota por tempo | implementado | [aodv_en_routes.c:327](../firmware/components/aodv_en/src/aodv_en_routes.c) `aodv_en_route_expire` | chamado em `aodv_en_node_tick` |
| Expiracao de vizinho por tempo | implementado | [aodv_en_neighbors.c:155](../firmware/components/aodv_en/src/aodv_en_neighbors.c) `aodv_en_neighbor_expire` | chamado em `aodv_en_node_tick` |
| Expiracao de RREQ cache | implementado | [aodv_en_rreq_cache.c:123](../firmware/components/aodv_en/src/aodv_en_rreq_cache.c) `aodv_en_rreq_cache_expire` | chamado em `aodv_en_node_tick` |
| **Cache LRU de peers ESP-NOW** | implementado | [aodv_en_peers.c:29](../firmware/components/aodv_en/src/aodv_en_peers.c) `peer_find_lru_index` | pula entradas PINNED |
| Validacao de header (version, network_id) | implementado | [aodv_en_node.c:1312](../firmware/components/aodv_en/src/aodv_en_node.c) `validate_header` | descarta cross-network/cross-version |
| Validacao de tamanho de mensagem | implementado | [aodv_en_node.c:1329](../firmware/components/aodv_en/src/aodv_en_node.c) `validate_message_size` | descarta DATA com `payload_length` inconsistente |
| Telemetria de stats | implementado | [aodv_en_node.h](../firmware/components/aodv_en/include/aodv_en_node.h) `aodv_en_stats_t` (16 contadores); copia em [aodv_en.c:35](../firmware/components/aodv_en/src/aodv_en.c) | exposta via `aodv_en_stack_get_overview/get_stats` |
| Adapter ESP-NOW | implementado | [firmware/main/app_demo.c:171](../firmware/main/app_demo.c) `app_emit_frame` + callbacks `recv_cb/send_cb` | atualmente acoplado ao app, fase 4 vai extrair |
| Modo `app_demo` | implementado | [firmware/main/app_demo.c](../firmware/main/app_demo.c) | HELLO + DATA periodicos |
| Modo `app_proto_example` | implementado | [firmware/main/app_proto_example.c](../firmware/main/app_proto_example.c) | + CLI + HEALTH/TEXT/CMD |
| **Articulation point detection** | **planejada para v2** | [docs/features/articulation-point-planejado.md](features/articulation-point-planejado.md) | nao implementada |
| **Multi-destino por RERR** | planejada para v2 | spec v1 - secao "Caminho de evolucao para v2" | atualmente 1 destino por RERR |
| Wrap-around de seq number | parcial | comparacao por `>` simples; suficiente para janelas curtas | v2: comparacao serial estilo RFC 1982 |
| Metrica composta com RSSI | nao | `metric = hop_count` em v1; RSSI usado so para diagnostico | mapeado para v2 |

---

## Cross-reference de constantes

Quando voce ver uma constante misteriosa no codigo, esta e a tabela:

### Tamanhos de tabelas (compile-time)

| Constante | Default | Onde define | Uso |
|---|---|---|---|
| `AODV_EN_NEIGHBOR_TABLE_SIZE` | 16 | [aodv_en_limits.h:9](../firmware/components/aodv_en/include/aodv_en_limits.h) | array de `aodv_en_neighbor_table_t.entries[]` |
| `AODV_EN_ROUTE_TABLE_SIZE` | 32 | aodv_en_limits.h:10 | array de `aodv_en_route_table_t.entries[]` |
| `AODV_EN_RREQ_CACHE_SIZE` | 64 | aodv_en_limits.h:11 | array de `aodv_en_rreq_cache_t.entries[]` |
| `AODV_EN_PEER_CACHE_SIZE` | 8 | aodv_en_limits.h:12 | array de `aodv_en_peer_cache_t.entries[]` |
| `AODV_EN_PENDING_DATA_QUEUE_SIZE` | 4 | aodv_en_limits.h:13 | filas `pending_data` e `pending_ack` |
| `AODV_EN_MAX_PRECURSORS` | 4 | aodv_en_limits.h:14 | array de precursores por rota |

### Timers (runtime, configuraveis em `aodv_en_config_t`)

| Constante | Default ms | Uso |
|---|---|---|
| `NEIGHBOR_TIMEOUT_MS` | 15000 | janela sem `last_seen` antes de expirar vizinho |
| `ROUTE_LIFETIME_MS` | 30000 | tempo de vida de rota nova; tambem janela de validade da fila pendente |
| `RREQ_CACHE_TIMEOUT_MS` | 10000 | janela do cache de duplicatas |
| `ACK_TIMEOUT_MS` | 1000 | timeout para retransmitir DATA com ACK_REQUIRED |

### Retries e thresholds

| Constante | Default | Uso |
|---|---|---|
| `RREQ_RETRY_COUNT` | 3 | retries de RREQ e ACK (compartilhado em v1) |
| `LINK_FAIL_THRESHOLD` | 3 | falhas seguidas para marcar vizinho INACTIVE |
| `MAX_HOPS` | 16 | limite de saltos do protocolo |
| `TTL_DEFAULT` | 16 | TTL inicial em RREQ e DATA |

### Hysterese de troca de rota

| Constante | Default | Uso |
|---|---|---|
| `ROUTE_SWITCH_MIN_SEQ_GAIN` | 2 | ganho de `dest_seq_num` para mudar `next_hop` |
| `ROUTE_SWITCH_MIN_METRIC_GAIN` | 1 | ganho de `metric` |
| `ROUTE_SWITCH_MIN_HOP_GAIN` | 1 | reducao de `hop_count` |
| `ROUTE_SWITCH_MIN_LIFETIME_GAIN_MS` | 5000 | aumento de `expires_at_ms` |

### Tamanhos de payload

| Constante | Default | Uso |
|---|---|---|
| `CONTROL_PAYLOAD_MAX` | 128 | maior frame de controle |
| `DATA_PAYLOAD_MAX` | 1024 | maior payload de DATA |

### Codigos especiais

| Constante | Valor | Uso |
|---|---|---|
| `AODV_EN_PROTOCOL_VERSION` | 1 | escrito em todo header; recv descarta versao diferente |
| `AODV_EN_ROUTE_METRIC_INFINITY` | 0xFFFF | marca rota invalidada (em metric) |
| `AODV_EN_MAC_ADDR_LEN` | 6 | bytes de MAC |

---

## Como uma mensagem viaja pelo codigo

Tracejando o caminho de um pacote DATA de A para C, com B intermediario:

### 1. App de A chama `send_data`

```
app_protocol_task() (firmware/main/app_demo.c:378)
  → aodv_en_stack_send_data_at() (aodv_en.c:301)
    → aodv_en_node_send_data() (aodv_en_node.c:1554)
       Sem rota valida:
         → aodv_en_node_queue_data() (aodv_en_node.c:449)
         → aodv_en_node_send_rreq() (aodv_en_node.c:880)
         → aodv_en_node_pending_note_rreq_attempt() (aodv_en_node.c:520)
         retorna AODV_EN_QUEUED
```

### 2. RREQ vai pro radio

```
aodv_en_node_send_rreq()
  → aodv_en_fill_header() (aodv_en_node.c:99)
  → aodv_en_node_emit() (aodv_en_node.c:58)
    → callbacks.emit_frame()  // injetado pelo stack
      → stack_emit_frame() (aodv_en.c:62)
        → adapter.tx_frame()
          → app_emit_frame() (firmware/main/app_demo.c:171)
            → esp_now_send()  // ESP-NOW driver
```

### 3. RREQ chega em B (no radio)

```
ESP-NOW driver
  → app_recv_cb() (firmware/main/app_demo.c:264)
    → xQueueSend()  // empurra para fila de RX
  ...
app_protocol_task() (loop)
  → app_process_rx_queue() (firmware/main/app_demo.c:334)
    → aodv_en_stack_on_recv_at() (aodv_en.c:356)
      → aodv_en_node_on_recv() (aodv_en_node.c:1612)
         → validate_header, validate_message_size
         → aodv_en_neighbor_touch() (aodv_en_neighbors.c:70)  // observacao passiva
         → handle_rreq() (aodv_en_node.c:1120)
            → rreq_cache_remember() (aodv_en_rreq_cache.c:83)
            → route_upsert(rota REVERSE pra A) (aodv_en_routes.c:203)
            B nao e o destino:
            → forward_rreq() (aodv_en_node.c:1025)
               → emit (broadcast com hop_count++ e ttl--)
```

### 4. RREQ chega em C, C responde RREP

```
on_recv -> handle_rreq:
  - cache: nao encontrou (originator+rreq_id), insere
  - install rota reversa pra A (via B, hops=2, REVERSE)
  - C e o destino:
  → send_rrep() (aodv_en_node.c:911)
     - regras RFC 3561 §6.6.1 sobre self_seq_num
     - emit unicast pelo next_hop da rota reversa = B
```

### 5. RREP volta em B

```
on_recv -> handle_rrep:
  → route_upsert(rota direta pra C, via=C, hops=1, VALID)
  → 3 chamadas a route_add_precursor (RFC 3561 §6.6.2):
    - rota(C).precursors += C  (de quem recebeu)
    - rota(C).precursors += A  (para quem repassa)
    - rota(A).precursors += C  (precursor da rota reversa)
  B nao e a origem:
  → forward_rrep() (aodv_en_node.c:1044)
```

### 6. RREP chega em A

```
on_recv -> handle_rrep:
  → route_upsert(rota direta pra C, via=B, hops=2, VALID)
  → rota(C).precursors += B
  A e o originator, retorna OK
  → flush_pending_data_for_destination(C) (aodv_en_node.c:716)
     → loop: send_data_via_route() (aodv_en_node.c:399)
        → track_pending_ack() (aodv_en_node.c:313)
        → send_data_with_sequence() (aodv_en_node.c:206)
           → emit DATA (unicast para B)
        → stats.pending_data_flushed++
```

### 7. DATA chega em B, repassa para C

```
on_recv -> handle_data:
  B nao e destino:
  → forward_data() (aodv_en_node.c:1064)
     - ttl > 1
     - encontra rota_valida(C) -> via=C
     - aodv_en_route_add_precursor(rota_C, link_src=A) // RFC 3561
     - hop_count++, ttl--, sender_mac=B
     - stats.forwarded_frames++
     - emit DATA (unicast para C)
```

### 8. DATA chega em C, entrega + envia ACK

```
on_recv -> handle_data:
  C e destino:
  - stats.delivered_frames++
  - callbacks.deliver_data(originator=A, payload, len)
    → app_deliver_data() (firmware/main/app_demo.c:207)
      → ESP_LOGI("DATA deliver from ...")
  - flag ACK_REQUIRED esta setada:
  → send_ack(target=A, seq) (aodv_en_node.c:1002)
     - encontra rota_usavel(A) -> via=B
     - emit ACK (unicast para B)
```

### 9. ACK volta para A

```
B: on_recv -> handle_ack:
  B nao e destino:
  - stats.forwarded_frames++
  - encontra rota_usavel(A) -> via=A
  - hop_count++, sender_mac=B
  - emit ACK (unicast para A)

A: on_recv -> handle_ack:
  A e destino:
  → pending_ack_consume(C, seq) (aodv_en_node.c:361)
     - remove entrada da fila de ACK pendente
     - stats.ack_received++
     - callbacks.ack_received(C, seq)
       → app_ack_received() (firmware/main/app_demo.c:220)
         → ESP_LOGI("ACK received from ...")
```

Cada salto incrementa contadores de `tx_frames` e `rx_frames`. `forwarded_frames` cresce em B nas duas direcoes.

---

## Apendice - Fluxo do tick periodico

`aodv_en_node_tick(now_ms)` ([aodv_en_node.c:1429](../firmware/components/aodv_en/src/aodv_en_node.c)) e chamado a cada ~50-100ms pela app. Em ordem:

1. `aodv_en_neighbor_expire` - remove vizinhos inativos
2. `aodv_en_route_expire` - remove rotas vencidas
3. `aodv_en_rreq_cache_expire` - remove entradas antigas do cache
4. `aodv_en_node_retry_route_discovery_for_pending` - reemite RREQ pendentes com backoff
5. `aodv_en_node_process_pending_ack_retries` - retransmite DATA pendente
6. `aodv_en_node_expire_pending_data` - descarta DATA enfileirado por tempo demais

Tudo isso mantem o estado limpo sem precisar de threads adicionais. O proprio `tick` e single-threaded.

---

## Como navegar este codigo

**Se voce esta debugando** uma feature (ex: "ACK nao chega"):
1. Olha o mapa por feature acima
2. Vai pra funcao indicada
3. Le os comentarios + asserts em volta
4. Coloca um `ESP_LOGI` extra se precisar
5. Re-flasha e usa `live_monitor.py -v` ou `idf.py monitor`

**Se voce esta estendendo** o protocolo (ex: adicionar v2 articulation point):
1. Le [features/articulation-point-planejado.md](features/articulation-point-planejado.md)
2. Decide onde no codigo encaixa (provavelmente novos handlers no `aodv_en_node.c` + novas estruturas no `aodv_en_types.h`)
3. Adiciona testes na simulacao (`sim/aodv_en_sim_*.c`)
4. Documenta na spec v1 ou cria spec v2

**Se voce esta escrevendo o TCC**:
- "Como o protocolo funciona" -> copia de [aodv-en-funcionamento.md](aodv-en-funcionamento.md)
- "Onde o codigo implementa cada feature" -> copia tabelas deste documento
- "Implementacao em camadas" -> usa a estrutura "Camada 1-9" deste documento

**Se voce e novo no projeto**:
1. Le [aodv-en-spec-v1.md](aodv-en-spec-v1.md) primeiro (o que o protocolo e)
2. Depois [aodv-en-funcionamento.md](aodv-en-funcionamento.md) (como funciona)
3. Finalmente este documento (onde encontrar)

---

## Referencia rapida - "preciso ver"

| Quero ver | Va em |
|---|---|
| O que cada mensagem tem byte a byte | [aodv-en-funcionamento.md](aodv-en-funcionamento.md) - "Mensagens no fio" |
| Como `send_data` decide entre enviar e enfileirar | [aodv_en_node.c:1554](../firmware/components/aodv_en/src/aodv_en_node.c) |
| Como o ACK encontra o caminho de volta | rota reversa instalada por `handle_rreq` ([aodv_en_node.c:1120](../firmware/components/aodv_en/src/aodv_en_node.c)); `send_ack` consulta tabela ([aodv_en_node.c:1002](../firmware/components/aodv_en/src/aodv_en_node.c)) |
| Onde o `link_fail_threshold` e checado | [aodv_en_node.c:1467](../firmware/components/aodv_en/src/aodv_en_node.c) `on_link_tx_result` |
| Por que rotas as vezes nao sao trocadas | hysterese em [aodv_en_routes.c:37](../firmware/components/aodv_en/src/aodv_en_routes.c) `candidate_is_strongly_better` |
| Onde a fila de DATA e drenada | [aodv_en_node.c:716](../firmware/components/aodv_en/src/aodv_en_node.c) `flush_pending_data_for_destination`; chamada em `handle_rrep` |
| Onde o RSSI EMA e calculado | [aodv_en_neighbors.c:25](../firmware/components/aodv_en/src/aodv_en_neighbors.c) `weighted_rssi` |
| Onde o snapshot do log e formatado | [firmware/main/app_demo.c:290](../firmware/main/app_demo.c) e [app_proto_example.c:746](../firmware/main/app_proto_example.c) `app_log_routes` |
| Onde o dashboard parseia o snapshot | [firmware/tools/live_monitor.py](../firmware/tools/live_monitor.py) - `LineParser.feed` + `SNAPSHOT_RE` + `ROUTE_RE` |

---

## Ultima linha de defesa: o codigo nao mente

Quando este documento estiver desatualizado em relacao ao codigo, **o codigo ganha**. O codigo tem:
- testes da simulacao (`sim/aodv_en_sim*.c`) executando o nucleo
- testes de bancada (TC-001..TC-005)
- comentarios `RFC 3561 §X.Y.Z` em pontos chave

Se voce encontrar discrepancia, abra issue ou atualize aqui antes de seguir adiante.
