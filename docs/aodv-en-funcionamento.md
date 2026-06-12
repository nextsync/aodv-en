# AODV-EN - Funcionamento Completo do Protocolo

## Estado

- alinhado com [aodv-en-spec-v1.md](aodv-en-spec-v1.md)
- ultima revisao: 2026-05-01
- objetivo deste documento: explicar **como** o protocolo funciona na pratica, mostrando fluxos ponta a ponta, com layout de bytes, traces de execucao e exemplos completos para 2, 3 e 4 nos.

Este documento e um guia didatico. Para a definicao normativa do protocolo use a [spec v1](aodv-en-spec-v1.md). Para os invariantes do AODV preservados pelo projeto, ver [aodv-base-invariantes.md](aodv-base-invariantes.md).

## Sumario

1. [Visao em uma pagina](#visao-em-uma-pagina)
2. [Camadas e contratos](#camadas-e-contratos)
3. [Mensagens no fio](#mensagens-no-fio)
4. [Estado interno por no](#estado-interno-por-no)
5. [Numeros de sequencia](#numeros-de-sequencia)
6. [Fluxos do protocolo (com exemplos)](#fluxos-do-protocolo-com-exemplos)
7. [Selecao e substituicao de rotas](#selecao-e-substituicao-de-rotas)
8. [Filas pendentes](#filas-pendentes)
9. [Falhas, RERR e precursores](#falhas-rerr-e-precursores)
10. [Worked example A-B-C-D](#worked-example-a-b-c-d)
11. [Timers, limites e calibragem](#timers-limites-e-calibragem)
12. [Telemetria e diagnostico](#telemetria-e-diagnostico)
13. [Limites conhecidos](#limites-conhecidos)
14. [Referencia rapida](#referencia-rapida)

---

## Visao em uma pagina

`AODV-EN` e uma adaptacao do `AODV` (RFC 3561) para redes mesh multi-hop sobre `ESP-NOW v2` em `ESP32`. Cada no:

- mantem **tabelas locais** (vizinhos, rotas, cache de `RREQ`, peers `ESP-NOW`, fila de `DATA` pendente, fila de `ACK` pendente);
- descobre rotas **sob demanda** (`RREQ` em broadcast, `RREP` em unicast pelo caminho reverso);
- entrega `DATA` **hop-by-hop** seguindo a tabela de rotas;
- responde `DATA` com `ACK` fim a fim quando a flag `ACK_REQUIRED` esta marcada;
- recupera de falhas com `RERR` direcionado a **precursores** (RFC 3561 secao 6.2).

Os componentes sao desacoplados via um `adapter` injetavel:

```
   APP        +-----------------+      +----------------+
   ----- on_data, on_ack -->    |      |                |
              |    AODV-EN      | <--> |    ESP-NOW     |
   <-- send_data, send_hello -- |      |   ou MOCK SIM  |
              +-----------------+      +----------------+
                  ^   ^
                  |   |
                  now_ms()    tx_frame(next_hop, frame, len, broadcast)
```

A mesma logica roda no firmware ESP32 e no simulador local em [sim/aodv_en_sim.c](../sim/aodv_en_sim.c).

---

## Camadas e contratos

O codigo em [firmware/components/aodv_en](../firmware/components/aodv_en) esta dividido em tres camadas claras:

### 1. Nucleo do protocolo (`aodv_en_node.*`)

Implementa a maquina de estado do `AODV-EN`. E **agnostico de transporte**. Suas dependencias externas sao apenas duas funcoes que voce passa por callback:

```c
typedef aodv_en_status_t (*aodv_en_emit_frame_fn)(
    void *user_ctx,
    const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *frame,
    size_t frame_len,
    bool broadcast);

typedef void (*aodv_en_deliver_data_fn)(
    void *user_ctx,
    const uint8_t originator_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len);
```

Mais um callback opcional `ack_received` para a aplicacao saber quando um `ACK` chega.

### 2. Fachada (`aodv_en.*`)

Camada `aodv_en_stack_*` que a aplicacao consome. Adiciona:

- alocacao do `aodv_en_node_t` no heap (uma vez na init);
- contrato de adapter limpo (`now_ms`, `tx_frame`);
- versoes "no_at" das funcoes que chamam `now_ms` automaticamente;
- snapshot de rotas e estatisticas em formato leitura-publica (`aodv_en_route_snapshot_t`).

```c
aodv_en_status_t aodv_en_stack_init(
    aodv_en_stack_t *stack,
    const aodv_en_config_t *config,
    const uint8_t self_mac[AODV_EN_MAC_ADDR_LEN],
    const aodv_en_adapter_t *adapter,
    const aodv_en_app_callbacks_t *app_callbacks);

aodv_en_status_t aodv_en_stack_send_data(
    aodv_en_stack_t *stack,
    const uint8_t destination_mac[AODV_EN_MAC_ADDR_LEN],
    const uint8_t *payload,
    uint16_t payload_len,
    bool ack_required);
```

### 3. Adapter ESP-NOW (na app, atualmente em `app_proto_example.c` e `app_demo.c`)

Implementa as funcoes `tx_frame` e `now_ms` para `ESP-NOW`/`esp_timer_get_time`, processa o `recv_cb` e o `send_cb` do `ESP-NOW` e empurra para o nucleo.

> A separacao entre app e adapter ainda nao esta 100% pronta - ver Fase 4 do [plano](plano-desenvolvimento-completo.md). Mas o contrato ja esta limpo o suficiente para portar para outros transportes (UART, BLE Mesh, simulacao em desktop, etc.).

---

## Mensagens no fio

Todas as mensagens compartilham um cabecalho comum de **14 bytes** packed (`__attribute__((packed))`).

### Cabecalho comum (14 bytes)

| Offset | Tamanho | Campo | Tipo | Notas |
|---|---|---|---|---|
| 0 | 1 | `protocol_version` | uint8 | sempre `1` em v1 |
| 1 | 1 | `message_type` | uint8 | `0=HELLO 1=RREQ 2=RREP 3=RERR 4=DATA 5=ACK` |
| 2 | 1 | `flags` | uint8 | `0x01=ACK_REQUIRED` (em DATA), `0x02=ROUTE_REPAIR` (reservado) |
| 3 | 1 | `hop_count` | uint8 | salto atual; incrementado a cada repasse |
| 4-7 | 4 | `network_id` | uint32 LE | identidade da malha |
| 8-13 | 6 | `sender_mac` | bytes | quem **acabou de transmitir** (1 hop atras) |

`sender_mac` e reescrito a cada repasse. Para o emissor original do dado fim a fim, use `originator_mac` (presente em RREQ, RREP, DATA, ACK).

### HELLO (28 bytes packed)

Anuncio periodico de presenca, em broadcast.

| Offset | Tamanho | Campo |
|---|---|---|
| 0-13 | 14 | header (`message_type=0`) |
| 14-19 | 6 | `node_mac` |
| 20-23 | 4 | `node_seq_num` |
| 24-27 | 4 | `timestamp_ms` |

Exemplo de bytes (NODE_A `28:05:A5:33:D6:1C`, network `0xA0DE0001`, seq `5`, ts `12345 ms`):

```
01 00 00 00  01 00 DE A0   protocol=1 type=0(HELLO) flags=0 hop=0 network=0xA0DE0001
28 05 A5 33  D6 1C         sender_mac
28 05 A5 33  D6 1C         node_mac (igual ao sender quando origem)
05 00 00 00                node_seq_num = 5
39 30 00 00                timestamp_ms = 12345
```

### RREQ (39 bytes packed)

Pedido de descoberta de rota, em broadcast.

| Offset | Tamanho | Campo |
|---|---|---|
| 0-13 | 14 | header (`message_type=1`) |
| 14-19 | 6 | `originator_mac` (origem da descoberta) |
| 20-25 | 6 | `destination_mac` (destino procurado) |
| 26-29 | 4 | `originator_seq_num` |
| 30-33 | 4 | `destination_seq_num` (ultimo conhecido pela origem; 0 se desconhecido) |
| 34-37 | 4 | `rreq_id` (deduplicacao com `originator_mac`) |
| 38 | 1 | `ttl` |

A par chave de deduplicacao e `(originator_mac, rreq_id)`. Cada no incrementa `hop_count` no header e decrementa `ttl` antes de repassar.

### RREP (34 bytes packed)

Resposta a `RREQ`, unicast pelo caminho reverso.

| Offset | Tamanho | Campo |
|---|---|---|
| 0-13 | 14 | header (`message_type=2`) |
| 14-19 | 6 | `originator_mac` (quem fez o `RREQ` original) |
| 20-25 | 6 | `destination_mac` (quem responde) |
| 26-29 | 4 | `destination_seq_num` |
| 30-33 | 4 | `lifetime_ms` |

### RERR (24 bytes packed)

Sinalizacao de quebra de rota, unicast aos precursores ou broadcast como fallback.

| Offset | Tamanho | Campo |
|---|---|---|
| 0-13 | 14 | header (`message_type=3`) |
| 14-19 | 6 | `unreachable_destination_mac` |
| 20-23 | 4 | `unreachable_dest_seq_num` |

A v1 carrega exatamente um destino por `RERR`. Se varias rotas sao invalidadas pela mesma falha, varios `RERR` sao emitidos em sequencia.

### ACK (30 bytes packed)

Confirmacao fim a fim de `DATA` com `ACK_REQUIRED`.

| Offset | Tamanho | Campo |
|---|---|---|
| 0-13 | 14 | header (`message_type=5`) |
| 14-19 | 6 | `originator_mac` (no que origina o `ACK` = destino do `DATA`) |
| 20-25 | 6 | `destination_mac` (alvo do `ACK` = origem do `DATA`) |
| 26-29 | 4 | `ack_for_sequence` |

### DATA (33 bytes header + payload)

Carga de aplicacao, unicast pela rota instalada.

| Offset | Tamanho | Campo |
|---|---|---|
| 0-13 | 14 | header (`message_type=4`) |
| 14-19 | 6 | `originator_mac` |
| 20-25 | 6 | `destination_mac` |
| 26-29 | 4 | `sequence_number` |
| 30 | 1 | `ttl` |
| 31-32 | 2 | `payload_length` (uint16 LE) |
| 33+ | n | `payload` (bytes da aplicacao) |

`payload_length` permite que o receptor valide o tamanho. O parser do nucleo descarta frames com `payload_length + sizeof(header) != frame_len`.

> **Sobre limites**: `AODV_EN_DATA_PAYLOAD_MAX = 1024` bytes. O ESP-NOW v2 suporta ate 1490 bytes de payload por frame, entao um `DATA` cheio (33 + 1024 = 1057 bytes) cabe sem problemas.

---

## Estado interno por no

Cada no mantem seis estruturas residentes, todas alocadas estaticamente dentro de `aodv_en_node_t` ([firmware/components/aodv_en/include/aodv_en_node.h](../firmware/components/aodv_en/include/aodv_en_node.h)):

### 1. Tabela de vizinhos (`aodv_en_neighbor_table_t`)

Quem foi visto a 1 salto, com qualidade de enlace.

```c
typedef struct {
    uint8_t mac[6];
    int8_t  avg_rssi;       // EMA: (3*old + new)/4
    int8_t  last_rssi;      // ultimo valor
    uint8_t link_fail_count;
    uint8_t state;          // 0=INACTIVE, 1=ACTIVE
    uint32_t last_seen_ms;
    uint32_t last_used_ms;
} aodv_en_neighbor_entry_t;
```

Capacidade default: `16` vizinhos. Substituicao: nao tem - quando lota, novos vizinhos retornam `AODV_EN_ERR_FULL` ate que outro expire.

### 2. Tabela de rotas (`aodv_en_route_table_t`)

Caminhos `destination -> next_hop`, com **precursores** (RFC 3561 secao 6.2):

```c
typedef struct {
    uint8_t  destination[6];
    uint8_t  next_hop[6];
    uint32_t dest_seq_num;
    uint32_t expires_at_ms;
    uint16_t metric;            // em v1, igual a hop_count; 0xFFFF = INFINITY
    uint8_t  hop_count;
    uint8_t  state;             // 0=INVALID, 1=REVERSE, 2=VALID
    uint8_t  precursor_count;
    uint8_t  precursors[4][6];  // ate 4 precursores por rota
} aodv_en_route_entry_t;
```

Estados:
- `INVALID`: rota conhecida mas nao usavel; pode ser substituida.
- `REVERSE`: instalada durante recepcao de `RREQ`; valida apenas para `RREP`/`ACK` voltarem.
- `VALID`: pronta para `DATA`.

Capacidade default: `32` rotas.

### 3. Cache de duplicatas de RREQ (`aodv_en_rreq_cache_t`)

Suprime processamento repetido de inundacoes:

```c
typedef struct {
    uint8_t  originator[6];
    uint32_t rreq_id;
    uint32_t created_at_ms;
    uint8_t  hop_count;
    uint8_t  used;
} aodv_en_rreq_cache_entry_t;
```

Capacidade default: `64`. **Politica de evicao**: se cheio e chega novo `RREQ`, o mais antigo por `created_at_ms` e substituido. Expira por `rreq_cache_timeout_ms` (default `10000 ms`).

### 4. Cache de peers ESP-NOW (`aodv_en_peer_cache_t`)

Liga o `next_hop` da tabela de rotas com a lista de peers registrados no driver `ESP-NOW`. Politica `LRU` que pula entradas com flag `PINNED`. Capacidade default: `8`.

### 5. Fila de DATA pendente (`aodv_en_pending_data_entry_t[4]`)

Buffer para `DATA` enquanto a rota nao chega:

```c
typedef struct {
    uint8_t  destination_mac[6];
    uint16_t payload_len;
    bool     ack_required;
    bool     used;
    uint32_t enqueued_at_ms;
    uint32_t last_rreq_at_ms;
    uint8_t  discovery_attempts;
    uint8_t  payload[1024];
} aodv_en_pending_data_entry_t;
```

Capacidade default: `4` itens. **Backpressure**: quando cheia, recicla a entrada mais antiga do mesmo destino primeiro (preserva diversidade entre destinos).

### 6. Fila de ACK pendente (`aodv_en_pending_ack_entry_t[4]`)

Rastreio de `DATA` com `ACK_REQUIRED` aguardando confirmacao para retransmissao:

```c
typedef struct {
    uint8_t  destination_mac[6];
    uint16_t payload_len;
    bool     used;
    uint8_t  retries_left;
    uint32_t sequence_number;
    uint32_t last_sent_at_ms;
    uint8_t  payload[1024];
} aodv_en_pending_ack_entry_t;
```

`retries_left` herda `rreq_retry_count` (default `3`). Timeout por entrada: `ack_timeout_ms` (default `1000 ms`).

### Configuracao e estatisticas

Toda a configuracao expostavel esta em `aodv_en_config_t`. Os contadores cumulativos para diagnostico/experimentos estao em `aodv_en_stack_stats_t`:

```
rx_frames, tx_frames, forwarded_frames, delivered_frames
ack_received, ack_retry_sent, ack_timeout_drops
route_discoveries, route_repairs, route_discovery_retries
duplicate_rreq_drops
pending_data_queued, pending_data_flushed, pending_data_dropped
link_fail_events, route_invalidations_link_fail
```

---

## Numeros de sequencia

O protocolo usa numeros de sequencia para definir **frescor** da informacao de roteamento. Sao monotonicos crescentes por no.

### Tres contadores

| Contador | Onde fica | Quando incrementa |
|---|---|---|
| `self_seq_num` | `aodv_en_node_t.self_seq_num` | ao emitir `RREQ` (origem); ao responder `RREP` se `req_dest_seq_num >= self_seq_num` |
| `next_rreq_id` | `aodv_en_node_t.next_rreq_id` | a cada `RREQ` emitido |
| `next_data_seq` | `aodv_en_node_t.next_data_seq` | a cada `DATA` emitido |

### Regras RFC 3561 §6.6.1 (implementadas em `aodv_en_node_send_rrep`)

Quando um no e o destino e responde a um `RREQ`:

```c
if (rreq_dest_seq_num > node->self_seq_num) {
    node->self_seq_num = rreq_dest_seq_num;
} else if (rreq_dest_seq_num == node->self_seq_num) {
    node->self_seq_num++;
}
// caso contrario: mantem self_seq_num
```

Isso garante que o `RREP` carregue um numero de sequencia **estritamente maior ou igual** ao que a origem ja conhece, alinhando com o invariante "rotas mais recentes vencem".

### Comparacao de frescor

Em `aodv_en_route_should_replace`, comparamos `dest_seq_num` por `>` simples sobre `uint32_t`. Isso e suficiente para a janela de tempo dos cenarios do TCC. Tratamento explicito de wrap-around (logica `(a - b) < 2^31`) fica mapeado para v2.

---

## Fluxos do protocolo (com exemplos)

A partir daqui, cada secao mostra:

1. **diagrama de sequencia** ASCII;
2. **estado das tabelas** antes e depois;
3. **trace** do que aparece nos logs reais.

Os exemplos usam MACs do TC-002:

```
NODE_A = 28:05:A5:33:D6:1C
NODE_B = 28:05:A5:33:EB:80
NODE_C = 28:05:A5:34:99:34
NODE_D = 28:05:A5:34:AA:AA   (TC-005 apenas)
```

### 5.1 HELLO + descoberta passiva de vizinhos

`HELLO` e o anuncio mais simples. Vai em broadcast a cada `hello_interval_ms` (default `5000 ms`, configuravel pelo Kconfig).

```
   t=0ms       NODE_A          NODE_B         NODE_C
                |               |               |
                |--- HELLO ---->|<--- alcance --|
                |               |  direto       |
                |  (A nao ouve C)               |
                |               |               |
   t=1000ms    on_recv          on_recv
              touch B (rssi=-50)  touch B
                                  touch nothing
```

### Comportamento ao receber `HELLO` (`aodv_en_node_handle_hello`)

```c
// Ao receber HELLO de mac=X:
1. neighbor_touch(X, rssi)           // EMA do RSSI, last_seen=now
2. route_upsert({
     destination = node_mac_no_HELLO, // emissor do HELLO
     next_hop    = link_src_mac,      // mesma coisa, pois e 1 hop
     dest_seq_num = node_seq_num,
     hop_count = 1, metric = 1,
     state = VALID
   })
```

Isso cria uma rota direta de 1 salto sem precisar passar por `RREQ`.

#### Estado depois de NODE_A receber HELLO de NODE_B

Tabela de vizinhos de NODE_A:
```
mac                     avg_rssi  last_seen_ms  state
28:05:A5:33:EB:80       -50       1000          ACTIVE
```

Tabela de rotas de NODE_A:
```
dest                    via                     hops  metric  state    expires
28:05:A5:33:EB:80       28:05:A5:33:EB:80       1     1       VALID    31000
```

#### Trace de log

```
I (1023) aodv_en_proto: routes=1 neighbors=1 tx=1 rx=1 delivered=0
I (1024) aodv_en_proto: route[0] dest=28:05:A5:33:EB:80 via=28:05:A5:33:EB:80 hops=1 metric=1 state=2 expires=31000
```

> Alem do `HELLO` ativo, o nucleo tambem usa **observacao passiva**: qualquer `on_recv` valido faz `neighbor_touch` no `link_src_mac` com o `RSSI` real. Entao mesmo um `RREQ`/`RREP`/`DATA` recebido atualiza a vizinhanca.

### 5.2 Descoberta de rota: RREQ -> RREP

Cenario classico: `NODE_A` quer enviar `DATA` para `NODE_C`, mas nao tem rota. `B` esta no meio.

```
                 NODE_A             NODE_B             NODE_C
                   |                  |                  |
   send_data()-->  |                  |                  |
   (sem rota)      |                  |                  |
                   | enqueue pending  |                  |
                   |                  |                  |
                   |---- RREQ -------> (broadcast)        |
                   |                  | hop=0            |
                   |                  | rreq_id=1        |
                   |                  | orig_seq=1       |
                   |                  |                  |
                   |                  | install rev_route|
                   |                  | A via A, hop=1   |
                   |                  |                  |
                   |                  |---- RREQ ------->| (broadcast forward)
                   |                  |  hop=1           |
                   |                  |                  |
                   |                  |                  | install rev_route
                   |                  |                  | A via B, hop=2
                   |                  |                  |
                   |                  |                  | C is dest!
                   |                  |                  | self_seq=max(req_dst_seq, self_seq)+1
                   |                  |                  |
                   |                  |<---- RREP -------| (unicast to B,
                   |                  |  dest_seq=2      |  via reverse_route)
                   |                  |                  |
                   |                  | install dir_route|
                   |                  | C via C, hop=1   |
                   |                  | add precursor A  |
                   |                  |                  |
                   |<-- RREP ---------|                  |
                   |  hop=1           |                  |
                   |                  |                  |
                   | install dir_route                   |
                   | C via B, hop=2                      |
                   | flush_pending_data() --> envia DATA |
```

#### Layout dos frames (resumo)

`RREQ` emitido por A (broadcast):
```
header:        protocol=1, type=1(RREQ), flags=0, hop_count=0, net=0xA0DE0001
sender_mac:    A
originator:    A
destination:   C
orig_seq_num:  1
dest_seq_num:  0    (A nao conhecia C)
rreq_id:       1
ttl:           16
```

Ao chegar em B, ele:
1. consulta cache de `RREQ`: nao encontra `(A, 1)`, registra agora;
2. instala rota reversa para A: `next_hop=A, hops=1, state=REVERSE`;
3. nao e o destino, entao reencaminha:

`RREQ` reencaminhado por B (broadcast):
```
header:        ... hop_count=1 ...
sender_mac:    B          <- reescrito por B
(originator, destination, orig_seq_num, dest_seq_num, rreq_id intactos)
ttl:           15         <- decrementado
```

C recebe o `RREQ`:
1. cache nao tem `(A, 1)` (C tambem nao viu antes);
2. instala rota reversa para A: `next_hop=B, hops=2, state=REVERSE`;
3. **e o destino** -> emite `RREP`.

`RREP` emitido por C (unicast para B):
```
header:        protocol=1, type=2(RREP), flags=0, hop_count=0, net=...
sender_mac:    C
originator:    A         <- a origem da descoberta
destination:   C         <- quem responde
dest_seq_num:  2         <- novo self_seq de C
lifetime_ms:   30000
```

C envia para `next_hop` da rota reversa para A, que e B.

B recebe o `RREP`:
1. instala rota direta para C: `next_hop=C, hops=1, state=VALID, dest_seq_num=2, lifetime=30000`;
2. **adiciona precursores** (RFC 3561 secao 6.6.2):
   - vizinho de quem recebeu o RREP (= C) e precursor de C - na pratica, `route(C).precursors += [C]`;
   - vizinho para quem repassa o RREP (= A) e precursor de C - `route(C).precursors += [A]`;
   - vizinho de quem recebeu o RREP (= C) tambem e precursor da rota reversa para A - `route(A).precursors += [C]`;
3. nao e a origem, repassa o RREP para A.

A recebe o `RREP`:
1. instala rota direta para C: `next_hop=B, hops=2, state=VALID`;
2. atualiza precursores: `route(C).precursors += [B]`;
3. **drena a fila pendente**: o `DATA` que ficou enfileirado quando `send_data` retornou `AODV_EN_QUEUED` agora vai pelo `next_hop=B`.

#### Estado depois da descoberta

NODE_A:
```
neighbors:  B (rssi)
routes:     C via B, hops=2, state=VALID, dest_seq=2
            B via B, hops=1, state=VALID  (vinha de HELLO ou observacao passiva)
            (A->A nunca aparece, e self)
```

NODE_B:
```
neighbors:  A, C
routes:     A via A, hops=1, state=VALID, precursors=[]
            C via C, hops=1, state=VALID, precursors=[A]
            (rota reversa para A foi sobrescrita pela rota direta de HELLO/observacao)
```

NODE_C:
```
neighbors:  B
routes:     A via B, hops=2, state=REVERSE, dest_seq=1
            B via B, hops=1, state=VALID
```

> **Observacao**: a rota reversa em C para A continua como `REVERSE` ate haver trafego que justifique torna-la `VALID`. Quando A enviar `DATA`, os intermediarios podem usar `REVERSE` para encaminhar `RREP`/`ACK` mas o `DATA` em si exige rota `VALID`. Quando C precisar enviar `DATA` para A, isso disparara um novo `RREQ`.

### 5.3 Encaminhamento de DATA hop-by-hop

Logo apos a descoberta, o `DATA` pendente e drenado.

```
   NODE_A                NODE_B                NODE_C
     |                     |                     |
     | DATA seq=1          |                     |
     | flag=ACK_REQUIRED   |                     |
     |--unicast-->         |                     |
     |  via=B, ttl=16      |                     |
     |                     | forward_data()      |
     |                     | ttl=15              |
     |                     | sender=B            |
     |                     | add_precursor(A)    |
     |                     |                     |
     |                     |--unicast-->         |
     |                     |  via=C              |
     |                     |                     | deliver_data()
     |                     |                     | callbacks.on_data(A, payload)
     |                     |                     |
     |                     |                     | ACK_REQUIRED set ->
     |                     |                     | send_ack(A, seq=1)
```

Cada salto:
- consulta `route_find_valid(destination)`;
- decrementa `ttl`;
- incrementa `header.hop_count`;
- reescreve `sender_mac` para si mesmo;
- adiciona o vizinho de quem recebeu como precursor da rota para o destino (RFC 3561);
- chama `emit_frame(next_hop, ...)`.

Se em algum salto a rota nao existe ou nao e usavel, o no descarta o `DATA` e emite `RERR` para o `unreachable_destination_mac`.

### 5.4 ACK fim a fim

Quando o `DATA` chega em C com `ACK_REQUIRED`:

```c
if (flags & AODV_EN_MSG_FLAG_ACK_REQUIRED) {
    aodv_en_node_send_ack(node, originator_mac=A, seq=1, now);
}
```

`send_ack` consulta a rota para A em C (que esta `REVERSE`, suficiente):

```
   NODE_C            NODE_B             NODE_A
     |                 |                  |
     | ACK seq=1       |                  |
     | originator=C    |                  |
     | destination=A   |                  |
     |--unicast-->     |                  |
     |  via=B          |                  |
     |                 | handle_ack       |
     |                 | nao e destino    |
     |                 | repassa unicast  |
     |                 |                  |
     |                 |--unicast-->      |
     |                 |  via=A           |
     |                 |                  | handle_ack
     |                 |                  | mac=A, e destino
     |                 |                  | pending_ack.consume(C, 1)
     |                 |                  | callbacks.on_ack(C, 1)
```

Em A:
- a fila `pending_ack` tinha uma entrada `(C, seq=1, retries_left=3, last_sent_at)`;
- `pending_ack_consume(C, 1)` remove ela e incrementa `stats.ack_received`;
- a app recebe o callback.

Se o `ACK` nao chegar em `ack_timeout_ms` (default `1000 ms`):
- `retries_left--`;
- se rota ainda valida, retransmite o mesmo `sequence_number` (o que ja esta no buffer pendente);
- se rota nao valida mais, dispara `RREQ` antes da proxima tentativa;
- ao esgotar `retries_left` (= 0), remove e contabiliza em `ack_timeout_drops`.

#### Regra importante de v1

`rreq_retry_count` e `ack_retry_count` **compartilham o mesmo orcamento** (`config.rreq_retry_count`). Separar isso esta mapeado para v2.

### 5.5 Fila de DATA pendente

Quando `send_data` e chamado mas a rota nao existe ou esta expirada, o `DATA` e **enfileirado**, um `RREQ` e disparado, e o status retornado e `AODV_EN_QUEUED`.

#### Sequencia exata em `aodv_en_node_send_data`:

```c
route = route_find_valid(destination);
if (route == NULL || !route_is_usable(route, now)) {
    queue_data(destination, payload, payload_len, ack_required, now);
    send_rreq(destination, now);
    pending_note_rreq_attempt(destination, now);
    return AODV_EN_QUEUED;
}
return send_data_via_route(route, ...);
```

#### Drain ao instalar rota nova

Quando `handle_rrep` instala uma rota direta valida:

```c
route_upsert(C, via=B, state=VALID, ...);
flush_pending_data_for_destination(C, now);  // <-- aqui
```

`flush_pending_data_for_destination` itera pelas `pending_data` e envia tudo que tem `destination_mac == C`, incrementando `pending_data_flushed`.

#### Retry de descoberta

A cada `tick`, o no checa se ha `pending_data` para destinos que ainda nao tem rota. Se sim, **reemite `RREQ`** com **backoff exponencial**:

```c
interval = ack_timeout_ms;       // 1000 ms inicial
for each attempt > 1: interval *= 2 (cap em 10000 ms);
```

Sequencia de tentativas: `1000 ms, 2000 ms, 4000 ms, 8000 ms, 10000 ms, 10000 ms ...`

### 5.6 RERR e precursores

Quando uma rota cai, o no emite `RERR` para os **precursores** dela (vizinhos que estavam usando essa rota), excluindo o `next_hop` quebrado.

#### Disparo de RERR

`RERR` e emitido em tres situacoes:

1. **Falha de TX no link layer** (`aodv_en_node_on_link_tx_result(success=false)`):
   - `link_fail_count++` no vizinho;
   - se atinge `link_fail_threshold` (default 3), vizinho vai para `INACTIVE`;
   - todas as rotas com aquele `next_hop` sao invalidadas (`state = INVALID`);
   - para cada rota invalidada, `notify_precursors_of_break` e chamado.

2. **Recepcao de DATA sem rota valida** (em `forward_data`):
   - emite `RERR` para `unreachable_destination_mac` em broadcast.

3. **Recepcao de RERR** (em `handle_rerr`):
   - invalida a rota local;
   - propaga `RERR` para os precursores dessa rota, excluindo o emissor recebido.

#### Comportamento de `notify_precursors_of_break`

```c
void notify_precursors_of_break(node, broken_next_hop, unreachable_mac, unreachable_seq, now):
    route = route_find(unreachable_mac);
    if (route == NULL) return;

    if (route->precursor_count == 0) {
        // sem precursores conhecidos, broadcast como fallback
        send_rerr(BROADCAST, unreachable_mac, unreachable_seq);
    } else {
        for each p in route->precursors:
            if (p == broken_next_hop) continue;  // nao manda para o que ja caiu
            send_rerr(p, unreachable_mac, unreachable_seq);
    }
```

#### Cenario completo

Topologia `A-B-C`. A tem rota A->C via B. C cai.

```
   NODE_A         NODE_B          NODE_C
     |              |               |
     | DATA seq=2   |               |
     |--->          |               |  (C ja desligado)
     |              | forward_data  |
     |              | ttl=15        |
     |              |--->X (timeout em send_cb)
     |              |               |
     |              | on_link_tx_result(C, success=false)
     |              | link_fail_count(C)++
     |              | nao chegou em threshold ainda
     |              |               |
     | DATA seq=3   |               |
     |--->          |               |
     |              |--->X
     |              | link_fail_count(C) = 2
     |              |               |
     | DATA seq=4   |               |
     |--->          |               |
     |              |--->X
     |              | link_fail_count(C) = 3 = threshold!
     |              | neighbor C -> INACTIVE
     |              | invalidate routes via=C
     |              |   route(C) state=INVALID
     |              |   route(D) state=INVALID
     |              |               |
     |              | notify_precursors_of_break(broken=C, dst=C):
     |              |   route(C).precursors = [A]
     |              |   send_rerr(A, dst=C, seq=...)
     |<---RERR------|               |
     |              |               |
     | handle_rerr  |               |
     | route_invalidate(C)          |
     | route(C).precursors = []     |
     | ja sem precursores, sem propagar
     |              |               |
     | (proximo send_data para C dispara novo RREQ
     |  que falhara enquanto C estiver fora,
     |  acumulando pending_data ate timeout)
```

Quando C volta:
- `RREQ` de A em broadcast alcanca B;
- B encaminha;
- C recebe, instala rota reversa, responde `RREP`;
- A instala rota direta nova, drena `pending_data`.

#### Regra importante de hysteresis

Em uma rede onde A tem dois caminhos para C (via B e via D), `route_should_replace` exige que a candidata seja **estritamente melhor** para trocar o `next_hop`. Caso contrario, mantem a atual. Isso evita "flapping" entre caminhos similares. Os limiares sao:

- `ROUTE_SWITCH_MIN_SEQ_GAIN = 2` (ganho de seq num)
- `ROUTE_SWITCH_MIN_METRIC_GAIN = 1`
- `ROUTE_SWITCH_MIN_HOP_GAIN = 1`
- `ROUTE_SWITCH_MIN_LIFETIME_GAIN_MS = 5000`

Quando o `next_hop` permanece o mesmo (refresh), os precursores existentes sao preservados. Quando o `next_hop` muda, os precursores sao zerados (vizinhos antigos nao necessariamente sabem do novo caminho).

---

## Selecao e substituicao de rotas

Em `route_should_replace(existing, candidate)` a ordem e:

1. estado: `VALID` substitui `INVALID` ou `REVERSE`;
2. histerese: se ambas `VALID` mas `next_hop` mudou, exigir ganho forte;
3. `dest_seq_num` maior vence;
4. com `dest_seq_num` igual: `metric` menor vence;
5. com `metric` igual: `hop_count` menor vence;
6. com tudo igual: maior `expires_at_ms` vence.

#### Exemplo simples

A tem rota `C via B, hops=2, dest_seq=5`. Recebe agora um `RREP` de B com `dest_seq=5, hops=2`. Sem ganho -> mantem.

Recebe outro `RREP` de D com `dest_seq=7, hops=2`. Ganho forte de seq (>= 2) -> troca para `C via D, hops=2, dest_seq=7`.

---

## Filas pendentes

Resumo do comportamento das duas filas (mais detalhe em [features/enfilaremento-dos-dados.md](features/enfilaremento-dos-dados.md)):

| Fila | Tamanho | Trigger de inclusao | Trigger de remocao | Falha apos esgotamento |
|---|---|---|---|---|
| `pending_data` | 4 | `send_data` sem rota valida | `RREP` instalado + drain | descartado em `pending_data_dropped` apos `route_lifetime_ms` |
| `pending_ack` | 4 | `send_data` com `ACK_REQUIRED` | `ACK` correspondente recebido | descartado em `ack_timeout_drops` apos `retries_left=0` |

Ambas tem capacidade fixa de 4. Quando `pending_data` enche, recicla a entrada mais antiga do mesmo destino primeiro.

---

## Falhas, RERR e precursores

Quadro de eventos relacionados a falha:

| Evento | O que dispara | Efeito |
|---|---|---|
| Vizinho silencioso | `last_seen_ms` excede `neighbor_timeout_ms` (15 s) | vizinho removido na proxima `tick` |
| TX unicast falha | adapter chama `on_link_tx_result(success=false)` | `link_fail_count++`; ao atingir threshold, vizinho `INACTIVE` e rotas via ele invalidadas |
| Rota expira | `expires_at_ms <= now` | removida em `route_expire` |
| RERR recebido | `handle_rerr` | invalida rota local; propaga `RERR` para precursores excluindo o sender |
| DATA sem rota | `forward_data` nao acha rota valida | descarta + emite `RERR(BROADCAST, dst)` |

---

## Worked example A-B-C-D

Cenario `TC-005`: cadeia linear de 4 nos. A envia `DATA` para D periodicamente.

### Fase 1 - boot e descoberta de vizinhos

`HELLO` periodico por todos. Tabelas iniciais (apos primeiros 5 segundos):

```
NODE_A.neighbors = { B }
NODE_A.routes    = { B via B hops=1 VALID }

NODE_B.neighbors = { A, C }
NODE_B.routes    = { A via A hops=1 VALID, C via C hops=1 VALID }

NODE_C.neighbors = { B, D }
NODE_C.routes    = { B via B hops=1 VALID, D via D hops=1 VALID }

NODE_D.neighbors = { C }
NODE_D.routes    = { C via C hops=1 VALID }
```

### Fase 2 - primeira tentativa A -> D

A chama `send_data(D, "tc005 chain4", ack_required=true)`. A nao tem rota para D.

```
A: queue_data(D, ...)
A: send_rreq(D)
A: send_data returns AODV_EN_QUEUED

A --RREQ--> [broadcast]   orig=A, dest=D, orig_seq=1, dest_seq=0, rreq_id=1
B handles RREQ:
   cache.remember(A, 1)
   route_upsert(A via A, hops=1, state=REVERSE)  // ja era VALID, vira VALID que vence
   forward_rreq()  hop=1, ttl=15

B --RREQ--> [broadcast]   sender=B, hop=1
A receives own RREQ via broadcast - cache.contains(A,1) - drops, increments duplicate_rreq_drops
C handles RREQ:
   cache.remember(A, 1)
   route_upsert(A via B, hops=2, state=REVERSE)
   forward_rreq()  hop=2, ttl=14

C --RREQ--> [broadcast]   sender=C, hop=2
B sees, drops as duplicate.
D handles RREQ:
   cache.remember(A, 1)
   route_upsert(A via C, hops=3, state=REVERSE)
   D is dest!
   self_seq = max(0, self_seq) = ?  D's self_seq starts at 0, so becomes 1
   send_rrep(orig=A, dest=D, dest_seq=1, lifetime=30000)
   target = next_hop of route to A = C
```

Continua na ida do `RREP`:

```
D --RREP--> [unicast to C]  orig=A, dest=D, dest_seq=1, hop=0
C handle_rrep:
   route_upsert(D via D, hops=1, state=VALID, dest_seq=1)
   precursors of route(D): add C (de quem recebeu) and add B (para quem repassa = next_hop da rota reversa para A, que e B)
                           ...na pratica, add link_src_mac=D e add reverse_route.next_hop=B
   precursors of route(A): add D (de quem recebeu o RREP)
   forward_rrep() to next_hop of route to A = B

C --RREP--> [unicast to B]  hop=1
B handle_rrep:
   route_upsert(D via C, hops=2, state=VALID, dest_seq=1)
   precursors of route(D): add C, add A
   precursors of route(A): add C
   forward_rrep() to next_hop of route to A = A

B --RREP--> [unicast to A]  hop=2
A handle_rrep:
   route_upsert(D via B, hops=3, state=VALID, dest_seq=1, lifetime=30000)
   precursors of route(D): add B (de quem recebeu)
                            (A nao tem reverse para si mesmo, segunda parte n/a)
   route(originator=A) is self, returns OK
   flush_pending_data_for_destination(D)  <-- envia o DATA enfileirado!
```

### Tabelas finais apos descoberta

NODE_A.routes:
```
B via B hops=1 VALID  precursors=[]
D via B hops=3 VALID  dest_seq=1 precursors=[B]
```

NODE_B.routes:
```
A via A hops=1 VALID  precursors=[C]   (C foi anexado durante RREP repasse)
C via C hops=1 VALID
D via C hops=2 VALID  dest_seq=1 precursors=[C, A]
```

NODE_C.routes:
```
B via B hops=1 VALID
D via D hops=1 VALID  dest_seq=1 precursors=[D, B]
A via B hops=2 REVERSE  (foi REVERSE durante RREQ; nao foi promovida pelo RREP porque A nao envia RREP de volta nessa direcao)
                          (ela vira VALID se A e D fizerem comunicacao bidirecional)
```

NODE_D.routes:
```
C via C hops=1 VALID
A via C hops=3 REVERSE (instalada durante RREQ; permanece REVERSE ate haver trafego de D->A)
```

### Fase 3 - DATA flow

```
A.flush_pending_data_for_destination(D):
   send_data_via_route(D, payload, ack_required=true)
   sequence_number = ++next_data_seq = 1
   pending_ack.track(D, payload, seq=1, retries_left=3)

A --DATA seq=1, ttl=16, ACK_REQ--> [unicast to B]
B handle_data:
   not destination
   forward_data:
     ttl=15, hop_count=1, sender=B
     route_to_D valid, next=C
     route(D).precursors += A (link_src_mac)
     stats.forwarded_frames++
B --DATA seq=1, ttl=15--> [unicast to C]
C handle_data:
   forward_data:
     ttl=14, hop=2, sender=C
     route_to_D valid, next=D
     route(D).precursors += B
C --DATA seq=1, ttl=14--> [unicast to D]
D handle_data:
   destination_mac == self_mac
   delivered_frames++
   callbacks.deliver_data(originator=A, payload, len)
   ACK_REQ flag set:
     send_ack(target=A, ack_for_sequence=1)
     find_usable_route(A) -> via=C
D --ACK seq=1--> [unicast to C]
C handle_ack:
   not destination
   forward via route_to_A.next_hop=B
C --ACK seq=1--> [unicast to B]
B handle_ack:
   forward via route_to_A.next_hop=A
B --ACK seq=1--> [unicast to A]
A handle_ack:
   destination==self, pending_ack.consume(D, 1)
   ack_received++
   callbacks.ack_received(D, 1)
```

Total: 1 DATA enviado + 2 forwards = 3 frames de DATA na malha; 1 ACK + 2 forwards = 3 frames de ACK. Para 3 saltos, **6 frames de TX no total** para entregar 1 DATA com confirmacao.

### Fase 4 - falha de enlace B-C

Suponha que B e C percam contato. O proximo `DATA` de A:

```
A --DATA seq=2--> B  (ok, B recebe)
B --DATA seq=2--> X  (TX falha)
B.on_link_tx_result(C, false): link_fail_count(C) = 1
B.adapter retorna AODV_EN_ERR_STATE; A nao sabe ainda

(repete por 3 tentativas pois link_fail_threshold = 3)
B link_fail_count(C) = 3 -> C INACTIVE
B.invalidate_routes_by_next_hop(C):
   route(C) -> INVALID
   route(D) -> INVALID
B.notify_precursors:
   route(C).precursors = []  (foi resetada quando next_hop mudou)
   broadcast RERR(unreachable=C)
   route(D).precursors = [C, A]
   filter out C (broken)
   send_rerr(A, unreachable=D, seq=1)
B --RERR--> [broadcast/unicast A]
A.handle_rerr:
   route_invalidate(D)
   route(D).precursors = [B]
   send_rerr(B, unreachable=D)  <-- mas B ja sabe, ele que invalidou
   (B handle_rerr: route(D) ja INVALID, no_op)

A continua tendo pending_data se a app continuou enviando.
Em ticks subsequentes:
   retry_route_discovery_for_pending(D):
     no route to D? send_rreq(D), backoff exponencial
   se C voltar fisicamente, B descobre por HELLO/RREQ e tabela e refeita.
```

---

## Timers, limites e calibragem

Defaults em [aodv_en_limits.h](../firmware/components/aodv_en/include/aodv_en_limits.h):

| Constante | Default | Faz o que |
|---|---|---|
| `NEIGHBOR_TIMEOUT_MS` | 15000 | janela sem `last_seen` antes de remover vizinho |
| `ROUTE_LIFETIME_MS` | 30000 | lifetime instalado em rota nova; tambem janela de validade da fila pendente |
| `RREQ_CACHE_TIMEOUT_MS` | 10000 | janela do cache de duplicatas |
| `ACK_TIMEOUT_MS` | 1000 | timeout para retransmitir DATA com `ACK_REQUIRED` |
| `RREQ_RETRY_COUNT` | 3 | retries de RREQ e ACK (compartilhado) |
| `LINK_FAIL_THRESHOLD` | 3 | falhas seguidas para marcar vizinho INACTIVE |
| `MAX_HOPS` | 16 | limite de saltos do protocolo |
| `TTL_DEFAULT` | 16 | TTL inicial em RREQ e DATA |
| `NEIGHBOR_TABLE_SIZE` | 16 | maximo de vizinhos |
| `ROUTE_TABLE_SIZE` | 32 | maximo de rotas |
| `RREQ_CACHE_SIZE` | 64 | maximo de duplicatas memorizadas |
| `PEER_CACHE_SIZE` | 8 | maximo de peers ESP-NOW registrados |
| `PENDING_DATA_QUEUE_SIZE` | 4 | maximo de DATA em fila |
| `MAX_PRECURSORS` | 4 | precursores por rota |
| `CONTROL_PAYLOAD_MAX` | 128 | maior frame de controle |
| `DATA_PAYLOAD_MAX` | 1024 | maior payload de DATA |

### Calibragem para diferentes cenarios

- **Cadeias linear (TC-001 a TC-005)**: defaults funcionam.
- **Malhas densas (>10 nos)**: aumentar `NEIGHBOR_TABLE_SIZE` e `ROUTE_TABLE_SIZE`. Considerar tambem aumentar `RREQ_CACHE_SIZE` para que o aumento de RREQs concorrentes nao cause descarte util.
- **Cenarios com flap**: aumentar `LINK_FAIL_THRESHOLD` para 5; aumentar `ROUTE_SWITCH_MIN_SEQ_GAIN` para 3; ajuda a estabilizar.
- **Cenarios moveis (caminhada)**: reduzir `NEIGHBOR_TIMEOUT_MS` para 8000; aumentar frequencia de `HELLO`.

---

## Telemetria e diagnostico

Cada no expoe via `aodv_en_stack_get_overview` os contadores em `aodv_en_stack_stats_t`:

| Contador | Significado |
|---|---|
| `rx_frames` | total de frames recebidos validados |
| `tx_frames` | total de frames emitidos pelo `emit_frame` callback |
| `forwarded_frames` | frames repassados pelo no (RREQ, RREP, DATA, ACK) |
| `delivered_frames` | DATA entregue ao callback `on_data` (no como destino) |
| `ack_received` | ACK recebido pela aplicacao |
| `route_discoveries` | RREQ emitidos como origem |
| `route_repairs` | RERR emitidos |
| `route_discovery_retries` | RREQ reemitidos pelo retry da fila pendente |
| `duplicate_rreq_drops` | RREQ ignorado por estar no cache |
| `pending_data_queued` | DATA enviado para a fila pendente |
| `pending_data_flushed` | itens drenados da fila ao instalar rota |
| `pending_data_dropped` | itens descartados (timeout ou backpressure) |
| `ack_retry_sent` | retransmissoes de DATA com ACK_REQUIRED |
| `ack_timeout_drops` | DATA desistido apos esgotar retries |
| `link_fail_events` | callbacks `on_link_tx_result(false)` |
| `route_invalidations_link_fail` | rotas invalidadas pelo cascading de falha de vizinho |

#### Metricas derivadas

- `PDR` = `delivered_frames / DATA_emitidos_pela_app`
- `overhead de controle` = `tx_frames - delivered_frames - forwarded_frames(DATA)`
- `latencia de descoberta` = `t(RREP recebido em A) - t(send_data com sem rota em A)` - capturada com `discovery_windows` em [tools/extract_monitor_metrics.py](../firmware/tools/extract_monitor_metrics.py)

---

## Limites conhecidos

Estas sao limitacoes da v1, ja mapeadas como itens de v2 na [spec v1](aodv-en-spec-v1.md):

1. **Orcamento de retry compartilhado**: `RREQ` e `ACK` retries usam o mesmo `rreq_retry_count`. Em cenarios onde a malha precisa varias descobertas mas o ACK e barato, esse acoplamento pode descartar `DATA` cedo demais. (resolver na v2 separando os dois)
2. **Wrap-around de sequence number**: comparacao por `>` simples. Em janelas longas (~50 dias com seq incrementando 1 por ms) ha risco teorico de overflow. Para o TCC nao e problema. (v2: comparacao serial)
3. **Multiplos destinos por RERR**: v1 carrega 1 destino por `RERR`. Se uma falha invalida `N` rotas, emite `N` RERR. RFC 3561 permite agregacao. (v2)
4. **Capacidade fixa de precursores**: `MAX_PRECURSORS = 4` por rota. Em malhas densas o array pode encher e novos precursores serao ignorados (`AODV_EN_ERR_FULL`). (v2: politica de eviction)
5. **Sem articulation point detection**: ainda nao implementado, ver [features/articulation-point-planejado.md](features/articulation-point-planejado.md).
6. **Metrica = hop_count**: RSSI nao entra no calculo da metrica. (v2: metrica composta)
7. **Sem RREP-ACK** (RFC 3561 secao 5.7): `RREP` enviado em unicast nao tem confirmacao explicita; perda silenciosa exige novo `RREQ`.
8. **App layer hardcoded broadcast HEALTH_REQ**: a app `app_proto_example` emite `HEALTH_REQ` periodico. Isso aparece como trafego no dashboard mas **nao e parte do AODV-EN**.

---

## Referencia rapida

### Funcoes-chave do nucleo

```c
// inicializacao
aodv_en_status_t aodv_en_node_init(node, config, self_mac);
void aodv_en_node_set_callbacks(node, callbacks);

// loop periodico (chamar a cada ~50-100 ms)
void aodv_en_node_tick(node, now_ms);

// controle
aodv_en_status_t aodv_en_node_send_hello(node, now_ms);

// dados (com confirmacao opcional)
aodv_en_status_t aodv_en_node_send_data(
    node, destination_mac, payload, payload_len, ack_required, now_ms);

// recepcao do transporte
aodv_en_status_t aodv_en_node_on_recv(
    node, link_src_mac, frame, frame_len, rssi, now_ms);

// resultado de TX (callback do driver, opcional mas recomendado)
aodv_en_status_t aodv_en_node_on_link_tx_result(
    node, next_hop, success, now_ms, &invalidated_count);
```

### Codigos de status

```c
AODV_EN_OK              =  0
AODV_EN_NOOP            =  1   // operacao reconhecida mas sem efeito (e.g. RREQ duplicado)
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

### Eventos esperados nos logs (TC-002)

```
boot:
  I (xxxx) aodv_en_proto: node=NODE_A self_mac=... channel=6 network_id=0xA0DE0001
  I (xxxx) aodv_en_proto: ESP-NOW version=2

trafego normal:
  I (xxxx) aodv_en_proto: HELLO send status=0
  I (xxxx) aodv_en_proto: DATA queued to route        <-- send_data com rota valida
  I (xxxx) aodv_en_proto: ACK received from <C> for seq=N

descoberta:
  I (xxxx) aodv_en_proto: DATA queued while route discovery is in progress

falha:
  W (xxxx) aodv_en_proto: ESP-NOW send fail to <MAC>
  W (xxxx) aodv_en_proto: invalidated 2 route(s) via <MAC> after link failures
  W (xxxx) aodv_en_proto: DATA send status=-2          <-- AODV_EN_ERR_FULL

snapshot periodico:
  I (xxxx) aodv_en_proto: routes=3 neighbors=2 tx=120 rx=95 delivered=0
  I (xxxx) aodv_en_proto: route[0] dest=<C> via=<B> hops=2 metric=2 state=2 expires=470000
  I (xxxx) aodv_en_proto: route[1] dest=<B> via=<B> hops=1 metric=1 state=2 expires=470000
```

### Onde olhar quando algo da errado

| Sintoma | Provavel causa | Onde investigar |
|---|---|---|
| `routes=0` por mais de 10s | HELLO nao esta chegando, canal/network_id divergente | comparar Kconfig dos nos, fisica do link |
| `duplicate_rreq_drops` cresce mas `route_discoveries` parado | RREQ nao retorna; destino unreachable ou sem RREP | logs de C/B, falha no caminho reverso |
| `pending_data_dropped` > 0 | rota nao chegou a tempo; backpressure ou timeout | aumentar `route_lifetime_ms` ou `pending_data_queue_size` |
| `ack_timeout_drops` alto | enlace pessimo ou dest fora de alcance; route flap | investigar `link_fail_events` e `route_invalidations_link_fail` |
| flapping de rota | duas rotas com metric quase identica | considerar aumentar `ROUTE_SWITCH_MIN_*_GAIN` |
| reset constante | watchdog ou stack overflow no task | `idf.py monitor` para ver guru meditation |

---

## Apendice: snippet de uso minimo

```c
#include "aodv_en.h"

static aodv_en_stack_t stack;

static uint32_t my_now_ms(void *ctx) {
    (void)ctx;
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static aodv_en_status_t my_tx(void *ctx,
                              const uint8_t next_hop[AODV_EN_MAC_ADDR_LEN],
                              const uint8_t *frame, size_t frame_len, bool broadcast) {
    (void)ctx;
    const uint8_t bcast[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
    const uint8_t *target = broadcast ? bcast : next_hop;
    return (esp_now_send(target, frame, frame_len) == ESP_OK)
        ? AODV_EN_OK : AODV_EN_ERR_STATE;
}

static void my_on_data(void *ctx, const uint8_t orig[6],
                       const uint8_t *payload, uint16_t len) {
    ESP_LOGI("APP", "got %u bytes from %02X:%02X:..", len, orig[0], orig[1]);
}

static void my_on_ack(void *ctx, const uint8_t from[6], uint32_t seq) {
    ESP_LOGI("APP", "ACK seq=%u", (unsigned)seq);
}

void app_main(void) {
    // ... esp_wifi + esp_now boilerplate ...

    aodv_en_config_t cfg;
    aodv_en_config_set_defaults(&cfg);
    cfg.network_id = 0xA0DE0001;

    uint8_t self_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, self_mac);

    aodv_en_adapter_t adapter = {
        .now_ms = my_now_ms, .tx_frame = my_tx,
    };
    aodv_en_app_callbacks_t cb = {
        .on_data = my_on_data, .on_ack = my_on_ack,
    };
    aodv_en_stack_init(&stack, &cfg, self_mac, &adapter, &cb);

    while (1) {
        aodv_en_stack_tick(&stack);
        // periodicamente: aodv_en_stack_send_hello(&stack);
        // sob demanda:   aodv_en_stack_send_data(&stack, dest_mac, payload, len, true);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

---

## Referencias internas

- [aodv-en-spec-v1.md](aodv-en-spec-v1.md) - definicao normativa
- [aodv-base-invariantes.md](aodv-base-invariantes.md) - invariantes herdados do AODV
- [aodv-en-estruturas-dados.md](aodv-en-estruturas-dados.md) - layout das tabelas e mensagens
- [features/enfilaremento-dos-dados.md](features/enfilaremento-dos-dados.md) - fila pendente
- [features/precursores.md](features/precursores.md) - precursores e RERR direcionado
- [features/articulation-point-planejado.md](features/articulation-point-planejado.md) - feature de v2
- [tests](tests/) - casos de teste com criterios de aprovacao
- RFC 3561: <https://datatracker.ietf.org/doc/html/rfc3561>

## Referencias de codigo

- [firmware/components/aodv_en/include/aodv_en.h](../firmware/components/aodv_en/include/aodv_en.h) - API publica `aodv_en_stack_*`
- [firmware/components/aodv_en/include/aodv_en_node.h](../firmware/components/aodv_en/include/aodv_en_node.h) - core API
- [firmware/components/aodv_en/include/aodv_en_messages.h](../firmware/components/aodv_en/include/aodv_en_messages.h) - layout no fio
- [firmware/components/aodv_en/include/aodv_en_limits.h](../firmware/components/aodv_en/include/aodv_en_limits.h) - constantes
- [firmware/components/aodv_en/src/aodv_en_node.c](../firmware/components/aodv_en/src/aodv_en_node.c) - logica principal
- [firmware/components/aodv_en/src/aodv_en_routes.c](../firmware/components/aodv_en/src/aodv_en_routes.c) - tabela de rotas + precursores
- [firmware/components/aodv_en/src/aodv_en_neighbors.c](../firmware/components/aodv_en/src/aodv_en_neighbors.c) - vizinhos + EMA RSSI
- [firmware/components/aodv_en/src/aodv_en.c](../firmware/components/aodv_en/src/aodv_en.c) - facade `stack_*`
- [sim/aodv_en_sim.c](../sim/aodv_en_sim.c) - simulacao de referencia 3 nos
