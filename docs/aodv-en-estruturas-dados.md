# AODV-EN Estruturas de Dados

## Objetivo

Este documento detalha as estruturas de dados que sustentam a implementacao inicial do `AODV-EN`.

Ele complementa a especificacao v0 e traduz a arquitetura conceitual do protocolo para um modelo de firmware mais concreto, com foco em tabelas locais, cabecalhos de mensagem e limites iniciais de memoria.

## Visao geral

O `AODV-EN` mantera quatro estruturas principais em cada no:

- tabela de vizinhos
- tabela de rotas
- cache de duplicatas de `RREQ`
- cache de peers ativos do ESP-NOW

Essas estruturas sao independentes entre si, embora cooperem durante descoberta de rota, encaminhamento e recuperacao de falhas.

## 1. Tabela de vizinhos

### Papel

Representa os nos de um salto observados diretamente pelo radio.

### Informacao mantida

- `mac`
- `avg_rssi`
- `last_rssi`
- `link_fail_count`
- `state`
- `last_seen_ms`
- `last_used_ms`

### Uso no protocolo

- alimenta flooding controlado
- ajuda a decidir para quem reenviar `RREQ`
- fornece qualidade de enlace local
- apoia o cache de peers do ESP-NOW

## 2. Tabela de rotas

### Papel

Representa caminhos conhecidos do tipo:

- `destination -> next_hop`

### Informacao mantida

- `destination`
- `next_hop`
- `dest_seq_num`
- `expires_at_ms`
- `metric`
- `hop_count`
- `state`

### Estados previstos

- `INVALID`
- `REVERSE`
- `VALID`

### Uso no protocolo

- roteamento hop-by-hop
- instalacao de rota reversa durante `RREQ`
- instalacao de rota direta durante `RREP`
- invalidacao por timeout ou erro

## 3. Cache de duplicatas de RREQ

### Papel

Evita processamento repetido de inundacoes de descoberta.

### Informacao mantida

- `originator`
- `rreq_id`
- `created_at_ms`
- `hop_count`
- `used`

### Uso no protocolo

- detectar `RREQ` ja visto
- limitar retransmissao redundante
- reduzir sobrecarga na malha

## 4. Cache de peers ESP-NOW

### Papel

Mantem um subconjunto pequeno dos vizinhos registrado no driver do ESP-NOW, respeitando o limite de peers ativos.

### Informacao mantida

- `mac`
- `last_used_ms`
- `flags`

### Politica inicial

- LRU simples
- adicao sob demanda
- remocao do menos recentemente usado

## Estruturas do protocolo

## Cabecalho comum

Todos os pacotes do `AODV-EN` compartilham um cabecalho base:

- `protocol_version`
- `message_type`
- `flags`
- `hop_count`
- `network_id`
- `sender_mac`

Esse cabecalho permite:

- identificar a versao do protocolo
- diferenciar mensagens
- transportar estado minimo comum

## Mensagens derivadas

Sobre o cabecalho comum, o protocolo define:

- `HELLO`
- `RREQ`
- `RREP`
- `RERR`
- `ACK`
- `DATA`

Cada mensagem possui apenas os campos necessarios para sua funcao.

## Limites iniciais

Os limites abaixo servem como base do firmware:

- `neighbor_table_size = 16`
- `route_table_size = 32`
- `rreq_cache_size = 64`
- `peer_cache_size = 8`
- `control_payload_max = 128`
- `data_payload_max = 1024`

## Estimativa de memoria

As estruturas propostas sao leves.

Estimativa aproximada:

- vizinho: ~20 a 24 bytes por entrada
- rota: ~24 a 32 bytes por entrada
- duplicata de `RREQ`: ~16 bytes por entrada
- peer cache: ~12 a 16 bytes por entrada

Com os tamanhos iniciais:

- vizinhos: ~320 a 384 bytes
- rotas: ~768 a 1024 bytes
- `RREQ` cache: ~1024 bytes
- peers: ~96 a 128 bytes

Ou seja, o consumo total da base de tabelas permanece em poucos kilobytes, o que e adequado ao ESP32 e deixa a maior parte do custo no radio, na fila de mensagens e nos buffers do Wi-Fi.

## Principios de projeto

- manter tabelas pequenas, previsiveis e explicitas
- evitar alocacao dinamica desnecessaria no caminho critico
- separar claramente vizinhanca de roteamento
- tratar `RREQ` duplicado como estrutura de primeira classe
- desacoplar cache de peers do modelo de vizinhos

## Arquivos relacionados

Os tipos iniciais dessas estruturas foram materializados em:

- `firmware/components/aodv_en/include/aodv_en_limits.h`
- `firmware/components/aodv_en/include/aodv_en_types.h`
- `firmware/components/aodv_en/include/aodv_en_messages.h`
- `firmware/components/aodv_en/include/aodv_en_tables.h`

As operacoes basicas sobre essas tabelas foram iniciadas em:

- `firmware/components/aodv_en/include/aodv_en_mac.h`
- `firmware/components/aodv_en/include/aodv_en_status.h`
- `firmware/components/aodv_en/include/aodv_en_neighbors.h`
- `firmware/components/aodv_en/include/aodv_en_routes.h`
- `firmware/components/aodv_en/include/aodv_en_rreq_cache.h`
- `firmware/components/aodv_en/include/aodv_en_peers.h`
