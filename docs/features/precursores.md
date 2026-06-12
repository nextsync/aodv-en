# Feature: Precursores e RERR direcionado

## Estado

- estado: `INTEGRADO_NA_V1`
- alinhado com [aodv-en-spec-v1.md](../aodv-en-spec-v1.md)
- referencia: RFC 3561, secao 6.2 e secao 6.11

## Contexto

A v0 do `AODV-EN` previa `RERR` como uma sinalizacao simples ao detectar quebra de enlace. Na pratica, isso forcava cada `RERR` a ser broadcast, o que gera trafego desnecessario em redes maiores.

A RFC 3561 resolve esse problema definindo precursores: para cada rota direta, um no mantem a lista dos vizinhos que sao "predecessores" no caminho de descoberta, ou seja, vizinhos que estao usando essa rota. Quando a rota cai, o no envia `RERR` apenas para esses precursores.

## O que foi implementado

### Estrutura

Cada `aodv_en_route_entry_t` carrega:

- `precursor_count` (uint8)
- `precursors[AODV_EN_MAX_PRECURSORS][6]` (default `MAX_PRECURSORS = 4`)

### Quando precursores sao adicionados

No tratamento de `RREP` (RFC 3561 secao 6.6.2):

- vizinho de quem recebemos o `RREP` e adicionado como precursor da rota direta para `destination_mac`
- vizinho para quem repassamos o `RREP` tambem e adicionado como precursor da rota direta para `destination_mac`
- vizinho de quem recebemos o `RREP` e adicionado como precursor da rota reversa para `originator_mac`

No encaminhamento de `DATA`:

- vizinho de quem recebemos o `DATA` e adicionado como precursor da rota direta para `destination_mac`

### Quando RERR usa precursores

No `RERR` direcionado (`aodv_en_node_notify_precursors_of_break`):

1. recupera a rota para `unreachable_mac`
2. se `precursor_count == 0`: emite `RERR` em broadcast como fallback
3. caso contrario: emite `RERR` em unicast para cada precursor, exceto o `next_hop` quebrado

Esse comportamento e disparado em duas situacoes:

- quando `aodv_en_node_handle_rerr` invalida uma rota local
- quando `aodv_en_node_on_link_tx_result` reporta falha de unicast e routes sao invalidados em massa por `next_hop`

### Preservacao em refresh

`aodv_en_route_upsert` preserva os precursores existentes quando uma rota e atualizada com o mesmo `next_hop` (refresh). Quando o `next_hop` muda, os precursores sao reinicializados, porque vizinhos que conheciam o caminho antigo nao sao necessariamente parte do novo.

## Arquivos impactados

- [firmware/components/aodv_en/include/aodv_en_types.h](../../firmware/components/aodv_en/include/aodv_en_types.h)
- [firmware/components/aodv_en/include/aodv_en_routes.h](../../firmware/components/aodv_en/include/aodv_en_routes.h)
- [firmware/components/aodv_en/src/aodv_en_routes.c](../../firmware/components/aodv_en/src/aodv_en_routes.c)
- [firmware/components/aodv_en/src/aodv_en_node.c](../../firmware/components/aodv_en/src/aodv_en_node.c)

## Limitacoes atuais

- `MAX_PRECURSORS = 4` por rota; precursores extras sao ignorados (`AODV_EN_ERR_FULL`) sem politica de substituicao
- nao ha telemetria especifica para precursores (uso indireto via `route_repairs`)
- a remocao de precursores so ocorre quando o `next_hop` muda; nao ha decay temporal

## Pontos de evolucao

- politica de substituicao quando o array de precursores enche (LRU? prioridade por uso?)
- contadores explicitos de `rerr_unicast`, `rerr_broadcast_fallback` e `precursors_added`
- considerar limpeza de precursores cuja rota reversa do precursor para nos ja expirou
