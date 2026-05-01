# AODV-EN Spec v0

## Estado

- estado: `OBSOLETA`
- substituida por: [aodv-en-spec-v1.md](aodv-en-spec-v1.md)

Esta versao foi mantida apenas para registro historico do escopo inicial de MVP. Toda referencia normativa do projeto deve apontar para a `v1`. Em caso de conflito, a `v1` prevalece.

Mudancas mais relevantes da `v0` para a `v1`:

- `HELLO` deixa de ser opcional e passa a ser parte do MVP
- `RERR` ganha semantica de precursores (RFC 3561 secao 6.2)
- fila de `DATA` pendente passa a ser parte do nucleo
- regras de `sequence number` em `RREP` alinhadas com RFC 3561 secao 6.6.1
- `metric = hop_count` fixado para `v1`; `RSSI` fica como ponto de evolucao
- adicao explicita de criterios de aprovacao de `v1` baseados nos casos de teste de bancada

## Objetivo

Este documento define a versao inicial do protocolo `AODV-EN`, uma adaptacao do AODV para redes mesh multi-hop construidas sobre ESP-NOW e ESP32.

Esta especificacao serve como base de implementacao do TCC. Ela nao pretende ser definitiva, mas precisa ser suficientemente completa para orientar o desenvolvimento do firmware e dos testes.

## Premissas

- todos os nos do testbed sao ESP32 com suporte a ESP-NOW v2
- todos os nos operam no mesmo canal Wi-Fi
- todos os nos compartilham o mesmo `network_id`
- a rede e ad hoc, sem coordenador central
- o protocolo de roteamento e reativo
- o transporte subjacente e ESP-NOW

## Decisoes de alto nivel

### Uso de ESP-NOW v2

O protocolo sera implementado sobre ESP-NOW v2.

Entretanto:

- mensagens de controle devem permanecer compactas
- o maior payload disponivel nao deve ser usado sem necessidade
- o protocolo nao deve depender de pacotes gigantes para funcionar

### Separacao de planos

O protocolo sera modelado em dois planos:

- plano de controle
- plano de dados

Plano de controle:

- `HELLO`
- `RREQ`
- `RREP`
- `RERR`
- `ACK`

Plano de dados:

- `DATA`

### Tabelas locais

Cada no mantera quatro estruturas principais:

- tabela de vizinhos
- tabela de rotas
- cache de duplicatas de `RREQ`
- cache de peers ativos ESP-NOW

## Estruturas de dados

### 1. Tabela de vizinhos

Responsavel por manter informacao de nos de um salto.

Campos minimos:

- `mac[6]`
- `last_seen_ms`
- `avg_rssi`
- `link_fail_count`
- `peer_cached`
- `active`

Responsabilidades:

- registrar nos ouvidos diretamente
- permitir flooding controlado para vizinhos ativos
- oferecer informacao local de qualidade de enlace
- apoiar politica de cache de peers

### 2. Tabela de rotas

Responsavel por manter informacao de encaminhamento.

Campos minimos:

- `destination[6]`
- `next_hop[6]`
- `hop_count`
- `dest_seq_num`
- `lifetime_ms`
- `valid`
- `metric`

Responsabilidades:

- decidir o proximo salto para um destino
- armazenar rotas reversas e diretas
- invalidar rotas expiradas ou quebradas

### 3. Cache de duplicatas de RREQ

Responsavel por suprimir mensagens repetidas.

Campos minimos:

- `originator[6]`
- `rreq_id`
- `created_at_ms`

### 4. Cache de peers ESP-NOW

Responsavel por limitar quantos peers ficam registrados no driver.

Campos minimos:

- `mac[6]`
- `last_used_ms`
- `pinned`

Politica inicial:

- LRU simples
- numero pequeno de peers ativos
- adicao sob demanda

## Tamanhos iniciais recomendados

Para o testbed do TCC:

- `neighbor_table_size = 16`
- `route_table_size = 32`
- `rreq_cache_size = 64`
- `peer_cache_size = 8`

Esses tamanhos sao suficientes para o ambiente inicial de testes e podem ser ajustados por configuracao.

## Tipos de mensagem

## 1. HELLO

Objetivo:

- manter conhecimento minimo de vizinhanca
- atualizar `last_seen`
- atualizar `RSSI`

Uso:

- opcional no MVP
- pode ser substituido por observacao passiva no inicio

Campos:

- `node_mac`
- `node_seq_num`
- `timestamp_ms`

## 2. RREQ

Objetivo:

- descobrir rota para um destino

Campos obrigatorios:

- `protocol_version`
- `message_type`
- `network_id`
- `originator_mac`
- `destination_mac`
- `sender_mac`
- `originator_seq_num`
- `destination_seq_num`
- `rreq_id`
- `hop_count`
- `ttl`

Regras:

- a origem emite `RREQ` quando nao possui rota valida
- cada no cria ou atualiza rota reversa para a origem
- cada no incrementa `hop_count`
- cada no descarta duplicatas
- o destino responde com `RREP`

## 3. RREP

Objetivo:

- responder a uma descoberta de rota

Campos obrigatorios:

- `protocol_version`
- `message_type`
- `network_id`
- `originator_mac`
- `destination_mac`
- `sender_mac`
- `destination_seq_num`
- `hop_count`
- `lifetime_ms`

Regras:

- volta pelo caminho reverso
- cada intermediario cria ou atualiza rota direta para o destino
- a origem instala a rota final ao receber o `RREP`

## 4. RERR

Objetivo:

- sinalizar rotas quebradas

Campos obrigatorios:

- `protocol_version`
- `message_type`
- `network_id`
- `sender_mac`
- `unreachable_destination_mac`
- `unreachable_dest_seq_num`

Regras:

- emitido quando um `next_hop` falha
- invalida rotas dependentes daquele enlace
- pode disparar nova descoberta se necessario

## 5. DATA

Objetivo:

- transportar dados da aplicacao

Campos obrigatorios:

- `protocol_version`
- `message_type`
- `network_id`
- `originator_mac`
- `destination_mac`
- `sender_mac`
- `sequence_number`
- `hop_count`
- `ttl`
- `payload_length`
- `payload`

Regras:

- segue a tabela de rotas
- cada salto atualiza `sender_mac`
- destino entrega ao handler da aplicacao

## 6. ACK

Objetivo:

- confirmar entrega fim a fim no nivel da aplicacao

Campos obrigatorios:

- `protocol_version`
- `message_type`
- `network_id`
- `originator_mac`
- `destination_mac`
- `ack_for_sequence`

Regras:

- o destino envia `ACK` quando recebe `DATA`
- a origem usa timeout para decidir retransmissao ou falha

## Cabecalho comum

Todas as mensagens devem compartilhar um cabecalho comum.

Campos iniciais recomendados:

- `protocol_version`
- `message_type`
- `flags`
- `network_id`
- `sender_mac`

Campos especificos entram apos o cabecalho comum.

## Regras operacionais

### Descoberta de vizinhos

MVP:

- descoberta passiva por observacao de trafego recebido

Fase seguinte:

- `HELLO` opcional e leve

### Descoberta de rota

Fluxo:

1. aplicacao solicita envio para um destino
2. no consulta tabela de rotas
3. se existir rota valida, envia `DATA`
4. se nao existir, emite `RREQ`
5. intermediarios constroem rota reversa
6. destino retorna `RREP`
7. intermediarios constroem rota direta
8. origem envia `DATA`

### Encaminhamento

Cada no usa:

- tabela de rotas para encontrar `next_hop`
- tabela de vizinhos para decidir se o enlace direto ainda e plausivel

### Duplicatas

`RREQ` duplicado deve ser descartado pelo par:

- `originator_mac`
- `rreq_id`

### Expiracao

Rotas e entradas auxiliares devem expirar por tempo.

Politica inicial:

- vizinho expira se ficar tempo demais sem ser visto
- rota expira apos `lifetime_ms`
- entrada de duplicata expira apos janela curta

## Timers iniciais

Valores iniciais de projeto, sujeitos a calibracao:

- `neighbor_timeout_ms = 15000`
- `route_lifetime_ms = 30000`
- `rreq_cache_timeout_ms = 10000`
- `ack_timeout_ms = 1000`
- `rreq_retry_count = 3`

## Metricas

MVP:

- `hop_count` como criterio principal

Fase seguinte:

- metrica composta com `RSSI`

Diretriz:

- `sequence_number` define frescor
- `hop_count` define preferencia principal no MVP
- `RSSI` entra como extensao controlada, nao como substituto da logica do AODV

## Confiabilidade

O `send callback` do ESP-NOW nao e suficiente para garantir entrega na aplicacao.

Por isso:

- `ACK` fim a fim permanece no protocolo
- timeout e retransmissao ficam na camada do AODV-EN

## Escopo do MVP

O MVP deve implementar:

- tabela de vizinhos
- tabela de rotas
- cache de duplicatas
- `RREQ`
- `RREP`
- `DATA`
- `ACK`
- expiracao basica

O MVP ainda nao precisa implementar:

- `RERR` sofisticado
- metrica hibrida completa
- fragmentacao de payload
- power saving adaptativo

## Criterios de aceite da v0

Consideraremos a implementacao inicial coerente com esta especificacao quando:

- dois nos conseguirem descobrir rota e trocar `DATA`
- tres nos conseguirem fazer relay multi-hop
- a origem instalar rota apos `RREP`
- o destino responder com `ACK`
- pacotes duplicados de `RREQ` forem descartados
- rotas expiradas deixarem de ser usadas
