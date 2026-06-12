# AODV-EN Spec v1

## Estado

- versao: `1.0`
- estado: `ATIVA`
- supersede: [aodv-en-spec-v0.md](aodv-en-spec-v0.md)
- ultima revisao: 2026-05-01
- escopo: especificacao funcional fechada do protocolo `AODV-EN` para o TCC

## Objetivo

Esta versao consolida o protocolo `AODV-EN` em um unico documento de referencia. Diferente da v0, que era um esboco de MVP, a v1 reflete o que esta realmente implementado em [firmware/components/aodv_en](../firmware/components/aodv_en) e fixa as decisoes funcionais que estavam em aberto.

A intencao e que esta spec sirva como base direta para:

- o capitulo de proposta do TCC
- os criterios de aprovacao dos testes de bancada
- a definicao de o que e v1 e o que fica para versoes futuras

Tudo que nao estiver descrito aqui e considerado fora do escopo de v1.

## Premissas

- todos os nos sao ESP32 com suporte a ESP-NOW v2
- todos os nos operam no mesmo canal Wi-Fi
- todos os nos compartilham o mesmo `network_id`
- a rede e ad hoc, sem coordenador central
- o protocolo de roteamento e reativo
- o transporte subjacente e ESP-NOW

## Versao do protocolo no fio

- `protocol_version = 1`
- todo frame `AODV-EN` valido carrega esse valor no cabecalho
- frames com versao diferente devem ser descartados

## Invariantes herdados

Esta spec preserva integralmente os invariantes definidos em [aodv-base-invariantes.md](aodv-base-invariantes.md):

- roteamento reativo
- descoberta por `RREQ` e resposta por `RREP`
- roteamento hop-by-hop
- numeros de sequencia
- rota reversa e rota direta
- supressao de duplicatas de `RREQ`
- invalidacao de rotas
- ausencia de loop como objetivo de projeto

Qualquer mudanca futura que viole um desses invariantes deixa de ser `AODV-EN` e passa a ser outro protocolo.

## Arquitetura em camadas

O `AODV-EN` e modelado em duas camadas claramente separadas, e isso e importante para o que e ou nao e parte do protocolo:

- camada de roteamento (`AODV-EN`)
- camada de aplicacao (qualquer payload da aplicacao final)

A camada de aplicacao do firmware de teste (com mensagens `HEALTH`, `TEXT`, `CMD` e CLI) e apenas um consumidor da camada de roteamento. Ela nao faz parte do `AODV-EN` e pode ser substituida por qualquer outra app sem afetar a spec.

## Planos do protocolo

O `AODV-EN` opera em dois planos:

Plano de controle:

- `HELLO`
- `RREQ`
- `RREP`
- `RERR`
- `ACK`

Plano de dados:

- `DATA`

## Tabelas locais

Cada no mantem seis estruturas residentes:

1. tabela de vizinhos
2. tabela de rotas (com precursores por rota)
3. cache de duplicatas de `RREQ`
4. cache de peers ESP-NOW
5. fila de `DATA` pendente
6. fila de `ACK` pendente

A definicao detalhada dos campos esta em [aodv-en-estruturas-dados.md](aodv-en-estruturas-dados.md). Esta spec descreve apenas o uso de cada estrutura no protocolo.

## Mensagens

Todas as mensagens compartilham o mesmo cabecalho comum.

### Cabecalho comum

- `protocol_version` (uint8)
- `message_type` (uint8)
- `flags` (uint8)
- `hop_count` (uint8)
- `network_id` (uint32)
- `sender_mac` (6 bytes)

`sender_mac` e o emissor do salto atual e e reescrito a cada repasse. Para identificar a origem fim a fim use `originator_mac` da carga especifica.

### Flags suportadas

- `ACK_REQUIRED` (`0x01`): aplicado em `DATA` quando o destino deve responder com `ACK`
- `ROUTE_REPAIR` (`0x02`): reservado para uso futuro de RREQ-repair; nao consumido em v1

### Tipos de mensagem

| Tipo | Codigo | Plano | Direcao | Estrategia |
|---|---|---|---|---|
| `HELLO` | 0 | controle | broadcast | periodico |
| `RREQ` | 1 | controle | broadcast | sob demanda |
| `RREP` | 2 | controle | unicast | resposta de descoberta |
| `RERR` | 3 | controle | unicast (precursores) ou broadcast (fallback) | sob falha |
| `DATA` | 4 | dados | unicast | sob demanda |
| `ACK` | 5 | controle | unicast | resposta de `DATA` com `ACK_REQUIRED` |

### HELLO

Campos alem do cabecalho:

- `node_mac`
- `node_seq_num`
- `timestamp_ms`

Regras:

- enviado periodicamente em broadcast por todos os nos
- intervalo padrao recomendado: `5000 ms`
- ao receber, o no atualiza a tabela de vizinhos e instala uma rota direta de 1 salto valida para `node_mac`
- `HELLO` por si so nao incrementa `self_seq_num` do no emissor (o numero divulgado e o atual)

### RREQ

Campos alem do cabecalho:

- `originator_mac`
- `destination_mac`
- `originator_seq_num`
- `destination_seq_num`
- `rreq_id`
- `ttl`

Regras de emissao:

- a origem incrementa `self_seq_num` ao emitir um novo `RREQ`
- a origem incrementa `next_rreq_id` para gerar um identificador unico
- `destination_seq_num` e o ultimo conhecido pela origem; pode ser `0` se desconhecido
- `ttl` inicial usa `ttl_default` (16)

Regras de recepcao:

- se o par `(originator_mac, rreq_id)` estiver no cache de duplicatas, descartar e contabilizar `duplicate_rreq_drops`
- se nao estiver, registrar no cache e:
  - instalar ou atualizar rota reversa para `originator_mac` no estado `REVERSE`
  - se for o destino: emitir `RREP`
  - caso contrario: repassar com `hop_count++` e `ttl--`, descartando se `ttl <= 1`

Politica de evicao do cache de duplicatas:

- entradas expiram naturalmente apos `rreq_cache_timeout_ms` (default `10000`)
- quando o cache esta cheio e uma entrada nova precisa ser inserida, a mais antiga por `created_at_ms` e substituida
- isso prefere descartar memoria de descobertas antigas em vez de bloquear novas, e evita deadlock em janelas de stress

### RREP

Campos alem do cabecalho:

- `originator_mac` (origem da descoberta original)
- `destination_mac` (destino que esta respondendo)
- `destination_seq_num`
- `lifetime_ms`

Regras de emissao pelo destino (RFC 3561, secao 6.6.1):

- se `requested_dest_seq > self_seq_num` da do destino: `self_seq_num = requested_dest_seq`
- se `requested_dest_seq == self_seq_num`: `self_seq_num++`
- caso contrario: manter `self_seq_num`
- emitir `RREP` para o `next_hop` da rota reversa para `originator_mac`

Regras de repasse intermediario:

- so repassa quando existe rota reversa para `originator_mac`
- instala rota direta para `destination_mac` no estado `VALID`, com lifetime `lifetime_ms`
- atualiza precursores conforme RFC 3561, secao 6.6.2:
  - vizinho de quem recebeu o `RREP` e precursor para `destination_mac`
  - vizinho para quem repassa o `RREP` tambem e precursor para `destination_mac`
  - vizinho de quem recebeu o `RREP` e precursor para `originator_mac` na rota reversa
- ao instalar rota direta valida, drena fila de `DATA` pendente para esse destino

### RERR

Campos alem do cabecalho:

- `unreachable_destination_mac`
- `unreachable_dest_seq_num`

Regras de emissao:

- emitido quando ocorre falha de transmissao para um `next_hop` ou quando `DATA` chega sem rota valida
- contem exatamente um destino inalcancavel (decisao de v1; multiplos destinos ficam para versoes futuras)
- proximo passo logico:
  - se a rota tem precursores: enviar `RERR` em unicast para cada precursor, exceto para o `next_hop` quebrado
  - se a rota nao tem precursores: enviar `RERR` em broadcast como fallback

Regras de recepcao:

- invalida rota local para `unreachable_destination_mac`
- propaga `RERR` recursivamente para os precursores da rota invalidada, exceto para o emissor recebido

### ACK

Campos alem do cabecalho:

- `originator_mac` (no que originou o `ACK`, ou seja, o destino do `DATA`)
- `destination_mac` (alvo do `ACK`, ou seja, a origem do `DATA`)
- `ack_for_sequence`

Regras:

- emitido pelo destino quando recebe `DATA` com flag `ACK_REQUIRED`
- segue a tabela de rotas como qualquer trafego
- a origem usa `ack_for_sequence` para casar com a entrada em `pending_ack` e cancelar retransmissao

### DATA

Campos alem do cabecalho:

- `originator_mac`
- `destination_mac`
- `sequence_number`
- `ttl`
- `payload_length`
- `payload[payload_length]`

Regras:

- a origem aloca um `sequence_number` monotonico
- consulta a tabela de rotas; se a rota nao for valida, segue o fluxo de fila pendente (ver secao "Fila de DATA pendente")
- intermediarios decrementam `ttl` e atualizam `sender_mac`
- se `ttl <= 1` no encaminhamento, descartar e emitir `RERR`
- se nao houver rota valida no encaminhamento, descartar e emitir `RERR`
- o destino entrega o payload ao callback de aplicacao e, se a flag `ACK_REQUIRED` estiver setada, responde com `ACK`

## Numeros de sequencia

O protocolo mantem dois contadores monotonicos por no:

- `self_seq_num`: numero de sequencia de roteamento do proprio no (usado em `HELLO`, `RREQ` como originator e em `RREP` como destination)
- `next_rreq_id`: identificador unico de `RREQ` emitidos pelo no (usado para deduplicacao)

Regras minimas de v1:

- `self_seq_num` so cresce
- e incrementado:
  - ao emitir um novo `RREQ` (a origem)
  - ao responder a um `RREQ` cujo `destination_seq_num` ja seja maior ou igual ao `self_seq_num` do destino, conforme RFC 3561 secao 6.6.1
- `next_rreq_id` so cresce e e usado para chave `(originator, rreq_id)` no cache de duplicatas

Comparacao de frescor:

- a comparacao em v1 e por `>` simples sobre `uint32_t`
- isso e suficiente para a janela de tempo dos cenarios do TCC; tratamento de wrap-around (logica `(a - b) < 2^31`) fica como ponto de evolucao futura

## Selecao e substituicao de rotas

O upsert de rota usa um criterio de "substituir somente se a candidata for melhor", com histerese para evitar flapping entre `next_hops`:

Ordem de prioridade:

1. estado: candidata `VALID` substitui `INVALID` ou `REVERSE`
2. histerese: se ambas sao `VALID` mas o `next_hop` mudou, exigir ganho forte em pelo menos um destes:
   - `seq_gain >= ROUTE_SWITCH_MIN_SEQ_GAIN`
   - `metric_gain >= ROUTE_SWITCH_MIN_METRIC_GAIN`
   - `hop_gain >= ROUTE_SWITCH_MIN_HOP_GAIN`
   - `lifetime_gain >= ROUTE_SWITCH_MIN_LIFETIME_GAIN_MS`
3. `dest_seq_num` maior vence
4. com `dest_seq_num` igual: `metric` menor vence
5. com `metric` igual: `hop_count` menor vence
6. com tudo igual: maior `expires_at_ms` vence

Quando o `next_hop` e o mesmo (refresh de rota), os precursores existentes sao preservados. Quando o `next_hop` muda, os precursores sao zerados.

## Metrica de roteamento em v1

- `metric = hop_count` para todas as rotas instaladas em v1
- `RSSI` e mantido na tabela de vizinhos para diagnostico e para criterios futuros de v2, mas nao entra no calculo de `metric`
- valor especial `AODV_EN_ROUTE_METRIC_INFINITY` (`0xFFFF`) marca rotas invalidadas

Decisao explicita de v1: nao adotar metrica composta com `RSSI`. Esse experimento fica para uma versao futura para nao misturar duas variaveis quando comparar com o baseline de flooding.

## Estados de rota

- `INVALID`: rota conhecida, mas nao usavel; nao gera trafego, podera ser substituida por candidata `VALID`
- `REVERSE`: rota construida durante recepcao de `RREQ`, valida para servir como caminho de volta de `RREP` e `ACK`, mas nao para `DATA` direto
- `VALID`: rota direta utilizavel para `DATA`

Regra de uso para repasse de `DATA`: somente rotas `VALID` e nao expiradas.

## Expiracao

Sao expirados pelo tick periodico do no:

- vizinhos: por `neighbor_timeout_ms` (default `15000`) sem serem ouvidos
- rotas: por `expires_at_ms` ja passado, com remocao do registro
- entradas do cache de `RREQ`: por `rreq_cache_timeout_ms` (default `10000`)
- entradas pendentes de `DATA`: por `route_lifetime_ms` desde o `enqueued_at_ms`
- entradas pendentes de `ACK`: por `retries_left = 0`

Quando uma rota usada por uma fila pendente expira ou e invalidada, o no nao precisa esperar a janela total do retry: ao detectar quebra de `next_hop` o protocolo dispara descoberta nova para os destinos com pendencia (`aodv_en_node_trigger_discovery_for_pending_destinations`).

## Fila de DATA pendente

A fila de pendentes e parte de v1 (ver tambem [features/enfilaremento-dos-dados.md](features/enfilaremento-dos-dados.md)).

Comportamento:

1. ao chamar `send_data` sem rota valida ou com rota expirada:
   - o `DATA` e enfileirado em `pending_data`
   - um `RREQ` e emitido para `destination_mac`
   - o status retornado e `AODV_EN_QUEUED`
2. ao instalar uma rota valida para um destino com pendencia:
   - todos os itens daquele destino sao drenados em ordem
3. no tick do no, para destinos com pendencia ainda sem rota:
   - o `RREQ` e reemitido respeitando backoff exponencial sobre `ack_timeout_ms`, com teto de `10000 ms`
4. expiracao:
   - se `now_ms - enqueued_at_ms >= route_lifetime_ms`, item descartado e contabilizado em `pending_data_dropped`

Backpressure quando a fila esta cheia:

- preferencia por reciclar a entrada mais antiga do mesmo destino (preserva diversidade entre destinos)
- caso nao exista, recicla a mais antiga global
- a entrada substituida e contabilizada como `pending_data_dropped`

## Confiabilidade fim a fim

O `send callback` do ESP-NOW so confirma entrega de 1 salto. Ele nao e suficiente para a aplicacao saber se o `DATA` chegou ao destino final.

Por isso o `AODV-EN` mantem `ACK` fim a fim no plano de controle:

- `ACK` e opt-in via flag `ACK_REQUIRED` em `DATA`
- a origem rastreia `pending_ack` por `(destination_mac, sequence_number)`
- timeout: `ack_timeout_ms` (default `1000 ms`)
- retries: limite compartilhado com `rreq_retry_count` (default `3`); apos esgotar, descarta com `ack_timeout_drops`
- se na hora do retry a rota voltou a ser invalida, o no dispara novo `RREQ` antes de retransmitir

Limitacao reconhecida: o orcamento de retentativas e o mesmo para `RREQ` e `ACK`. Separar essas duas politicas e um trabalho de evolucao.

## Manutencao de vizinhanca

A vizinhanca e mantida por duas vias complementares:

1. observacao passiva: qualquer `on_recv` valido faz `neighbor_touch` no `link_src_mac` com `RSSI` real
2. anuncio ativo: `HELLO` periodico em broadcast

Em v1, ambos sao usados. `HELLO` ativo facilita reconvergencia em cadeia, especialmente em TC-002, TC-003 e TC-004.

Suavizacao de `RSSI`:

- `last_rssi` registra o ultimo valor visto
- `avg_rssi` aplica EMA com peso fixo: `avg = (3 * avg_anterior + rssi_atual) / 4`
- quando o vizinho vem de `INACTIVE`, o EMA e re-inicializado com o `rssi` atual em vez de continuar do valor antigo

Esses dois campos sao usados apenas para diagnostico em v1. Nenhuma decisao de roteamento depende de `RSSI` em v1 (ver "Metrica de roteamento em v1").

Quando uma transmissao unicast falha repetidamente para um vizinho:

- `link_fail_count` incrementa em `aodv_en_neighbor_note_link_failure`
- ao atingir `link_fail_threshold` (default `3`) o vizinho vai para `INACTIVE`
- todas as rotas que dependem desse `next_hop` sao invalidadas em massa
- precursores de cada rota invalidada recebem `RERR`

## TTL e limites de tamanho

- `TTL` inicial: `16`
- `MAX_HOPS`: `16`
- payload de controle maximo: `128 bytes`
- payload de dados maximo: `1024 bytes`
- frames com tamanho incompativel com o tipo declarado sao descartados em `aodv_en_validate_message_size`

## Adapter de transporte

A camada de roteamento nao conhece ESP-NOW diretamente. Ela depende de duas funcoes injetadas pelo adapter:

- `now_ms(user_ctx) -> uint32_t`: fonte de tempo monotonica em milissegundos
- `tx_frame(user_ctx, next_hop, frame, frame_len, broadcast) -> aodv_en_status_t`: envia um frame para 1 salto, broadcast ou unicast

Esse contrato esta declarado em [aodv_en.h](../firmware/components/aodv_en/include/aodv_en.h) e e o que permite que a mesma logica rode na simulacao local em C e no firmware ESP-IDF sem alterar o nucleo.

## Configuracao por no

Todos os timers, tamanhos de tabela e thresholds sao expostos em `aodv_en_config_t` ([firmware/components/aodv_en/include/aodv_en_types.h](../firmware/components/aodv_en/include/aodv_en_types.h)). Os defaults dessa configuracao estao em [aodv_en_limits.h](../firmware/components/aodv_en/include/aodv_en_limits.h):

| Constante | Default |
|---|---|
| `NEIGHBOR_TIMEOUT_MS` | `15000` |
| `ROUTE_LIFETIME_MS` | `30000` |
| `RREQ_CACHE_TIMEOUT_MS` | `10000` |
| `ACK_TIMEOUT_MS` | `1000` |
| `RREQ_RETRY_COUNT` | `3` |
| `LINK_FAIL_THRESHOLD` | `3` |
| `MAX_HOPS` | `16` |
| `TTL_DEFAULT` | `16` |
| `NEIGHBOR_TABLE_SIZE` | `16` |
| `ROUTE_TABLE_SIZE` | `32` |
| `RREQ_CACHE_SIZE` | `64` |
| `PEER_CACHE_SIZE` | `8` |
| `PENDING_DATA_QUEUE_SIZE` | `4` |
| `MAX_PRECURSORS` (por rota) | `4` |
| `CONTROL_PAYLOAD_MAX` | `128` |
| `DATA_PAYLOAD_MAX` | `1024` |

Esses valores sao calibracao inicial. Cenarios de stress test podem reajusta-los, e a configuracao por no e independente.

## Telemetria minima

A v1 expoe, via `aodv_en_stack_stats_t` em [aodv_en.h](../firmware/components/aodv_en/include/aodv_en.h):

- `rx_frames`, `tx_frames`, `forwarded_frames`, `delivered_frames`
- `ack_received`, `ack_retry_sent`, `ack_timeout_drops`
- `route_discoveries`, `route_repairs`, `route_discovery_retries`
- `duplicate_rreq_drops`
- `pending_data_queued`, `pending_data_flushed`, `pending_data_dropped`
- `link_fail_events`, `route_invalidations_link_fail`

Esses contadores sao a base mensuravel para os experimentos da Fase 8 do plano de desenvolvimento.

## Escopo de v1

Esta dentro de v1:

- `HELLO`, `RREQ`, `RREP`, `RERR`, `DATA`, `ACK`
- tabela de vizinhos com `RSSI` e estado ativo/inativo
- tabela de rotas com estados `INVALID`, `REVERSE`, `VALID` e precursores
- cache de duplicatas de `RREQ`
- cache de peers ESP-NOW
- fila de `DATA` pendente com retry de descoberta e expiracao
- fila de `ACK` pendente com retry e timeout
- regra de troca de rota com histerese
- propagacao de `RERR` para precursores
- adapter de transporte injetavel
- telemetria minima

Esta fora de v1:

- metrica composta com `RSSI`
- multiplos destinos por `RERR`
- detecao de articulation point (ver [features/articulation-point-planejado.md](features/articulation-point-planejado.md))
- separacao do orcamento de retry de `RREQ` e `ACK`
- tratamento explicito de wrap-around de numero de sequencia
- power saving e duty-cycle adaptativo
- fragmentacao de payload alem de `DATA_PAYLOAD_MAX`

## Criterios de aprovacao da v1

Consideramos `AODV-EN v1` aceito quando todos os criterios abaixo forem demonstrados em hardware e/ou simulador:

1. Em 2 nos com enlace direto:
   - HELLO descobre vizinhanca em ate 30 s
   - rota direta `state=VALID, hops=1` instalada
   - `DATA` entregue e `ACK` recebido na origem
2. Em cadeia de 3 nos `A-B-C` sem enlace direto `A<->C`:
   - rota `A->C` com `hops=2` apos descoberta
   - `DATA` chega em `C`
   - `ACK` chega em `A`
3. Reconvergencia em cadeia de 3 nos:
   - apos quebra do enlace intermediario, a rota `VALID` e invalidada por `link_fail_threshold` ou expiracao
   - apos retorno do enlace, nova descoberta restaura entrega em ate 30 s
4. Soak de 30 minutos com ciclos repetidos de degradacao e recuperacao:
   - sem reboot
   - cada ciclo termina com `delivered_frames` e `ack_received` voltando a crescer
5. Cache de duplicatas:
   - `duplicate_rreq_drops > 0` em qualquer cenario com mais de 2 nos sob descoberta concorrente
6. Expiracao de rota:
   - rotas invalidadas saem da tabela apos `route_lifetime_ms`
7. RERR para precursores:
   - quando o caminho central cai, os precursores recebem `RERR` (verificavel por log)
8. Fila pendente:
   - `DATA` enviado antes da rota retorna `AODV_EN_QUEUED` e e drenado apos `RREP`

Os casos de teste correspondentes estao em [tests](tests/):

- `TC-001` cobre 1
- `TC-002` cobre 2 e 5
- `TC-003` cobre 3 e 7
- `TC-004` cobre 4 e 6
- a fila pendente esta coberta pela simulacao em [sim/aodv_en_sim.c](../sim/aodv_en_sim.c)

## Caminho de evolucao para v2

Itens de v2 ja mapeados, sem compromisso de prazo:

- detecao e telemetria de articulation point
- metrica composta `hop_count + RSSI` com hysterese explicita
- `RREP-ACK` opcional (RFC 3561 secao 5.7)
- repair local de rota (`RREQ` com flag `R`)
- compactacao de `RERR` com lista de destinos
- politica de retry separada para `RREQ` e `ACK`
- power saving cooperativo entre vizinhos

## Referencias internas

- [aodv-en-funcionamento.md](aodv-en-funcionamento.md) - guia didatico com layout de bytes, traces e worked examples
- [aodv-base-invariantes.md](aodv-base-invariantes.md) - regras que nao podem ser quebradas
- [aodv-en-estruturas-dados.md](aodv-en-estruturas-dados.md) - layout das tabelas e mensagens
- [features/enfilaremento-dos-dados.md](features/enfilaremento-dos-dados.md) - fila pendente
- [features/precursores.md](features/precursores.md) - precursores e RERR direcionado
- [features/articulation-point-planejado.md](features/articulation-point-planejado.md) - feature de v2
- [plano-desenvolvimento-completo.md](plano-desenvolvimento-completo.md) - roadmap geral
- [tests](tests/) - casos de teste de bancada
