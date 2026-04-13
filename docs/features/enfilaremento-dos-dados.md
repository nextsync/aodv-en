# Feature: Pending DATA Queue

## Objetivo

Evitar perda imediata de DATA quando a rota para o destino ainda nao existe no momento do envio.

Antes desta feature, a chamada de envio retornava erro de no route e o pacote precisava ser tratado fora do nucleo. Agora o pacote pode ser armazenado temporariamente enquanto a descoberta de rota ocorre.

## Problema resolvido

Cenario comum:

- no A deseja enviar DATA para no C
- no A ainda nao possui rota valida para C
- descoberta via RREQ/RREP ainda nao terminou

Sem fila pendente, o primeiro DATA era perdido ou exigia retry da aplicacao.

## Comportamento implementado

### Envio com rota ausente

Quando a rota esta ausente ou expirada em aodv_en_node_send_data:

1. DATA e enfileirado na pending queue
2. RREQ e emitido para iniciar descoberta
3. retorno da funcao passa a ser AODV_EN_QUEUED

### Quando a rota chega

Ao receber RREP e instalar rota valida para o destino:

1. o no tenta drenar a pending queue daquele destino
2. cada item e enviado usando a rota recem instalada
3. estatistica de flush e incrementada

### Expiracao da fila

No tick do no:

- itens pendentes expiram quando excedem route_lifetime_ms
- itens expirados sao removidos e contabilizados como dropped

## Estruturas e constantes adicionadas

- AODV_EN_PENDING_DATA_QUEUE_SIZE_DEFAULT = 4
- AODV_EN_PENDING_DATA_QUEUE_SIZE
- novo status AODV_EN_QUEUED

Nova estrutura por item pendente:

- destination_mac
- payload_len
- ack_required
- used
- enqueued_at_ms
- payload

Contadores novos em stats:

- pending_data_queued
- pending_data_flushed
- pending_data_dropped

## Arquivos impactados

- firmware/components/aodv_en/include/aodv_en_limits.h
- firmware/components/aodv_en/include/aodv_en_status.h
- firmware/components/aodv_en/include/aodv_en_node.h
- firmware/components/aodv_en/src/aodv_en_node.c
- firmware/main/main.c
- sim/aodv_en_sim.c

## Evidencia de validacao

A simulacao local foi ajustada para validar o novo contrato:

- primeira tentativa para destino sem rota retorna AODV_EN_QUEUED
- apos descoberta, DATA e entregue e ACK e recebido

Resultado esperado no fluxo:

- route discovery phase inicia com queued
- data phase confirma entrega e ack
- simulation passed

## Limitacoes atuais

- ainda nao existe mecanismo de ACK timeout e retry
- nao existe deduplicacao de payload pendente por destino
- expiracao usa route_lifetime_ms como janela da fila
- fila tem tamanho fixo e pode retornar AODV_EN_ERR_FULL

## Proximos passos recomendados

1. implementar ACK timeout e retry automatico
2. adicionar telemetria de queue occupancy por destino
3. avaliar politica FIFO estrita e backpressure
4. criar cenarios de estresse com fila cheia no simulador
