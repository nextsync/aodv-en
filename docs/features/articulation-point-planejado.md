# Feature Planejada: Articulation Point (No de Corte)

## Status

- estado atual: PLANEJADA
- implementacao em codigo: NAO
- objetivo deste arquivo: registrar decisao tecnica, escopo e criterios de aceite antes da implementacao

## Contexto

Em uma rede mesh, um articulation point (no de corte) e um no cuja falha separa a rede em dois ou mais segmentos.

Cenario tipico:

- Segmento A <-> No Ponte <-> Segmento B
- se o no ponte falha, A e B ficam particionados

Esse caso e critico para AODV-EN porque pode aparentar falha geral de entrega, quando na verdade e particionamento topologico.

## Problema a resolver

Quando um no de corte falha:

1. rotas antigas podem continuar sendo tentadas por um intervalo
2. a origem pode repetir descoberta sem saber que houve particao
3. sem telemetria, diagnostico de campo fica dificil

## Objetivos da feature

1. Detectar rapidamente indicios de particionamento por no de corte.
2. Melhorar recuperacao quando o no ponte retorna.
3. Produzir evidencias de rede particionada vs rede recuperada.

## Escopo planejado

### 1. Simulador

Adicionar cenario dedicado no simulador com fases:

1. fase normal (entrega fim-a-fim)
2. falha do no ponte (particao)
3. retries durante particao
4. retorno do no ponte
5. reconvergencia e entrega restaurada

### 2. Firmware

Adicionar sinais operacionais para suspeita de no de corte:

- perda de next-hop com alto impacto em rotas
- repeticao de route discovery sem sucesso de entrega
- recuperacao apos retorno do caminho central

### 3. Telemetria minima

Registrar eventos para analise:

- neighbor_loss
- route_break_by_next_hop
- repeated_route_discovery
- partition_suspected
- partition_recovered

## Fora de escopo agora

- algoritmo formal de deteccao de articulation point por teoria de grafos em runtime
- eleicao de no redundante automatica
- reconfiguracao autonoma de topologia

## Criterios de aceite propostos

1. Em estado normal: PDR >= 95%.
2. Durante particao: PDR entre segmentos isolados = 0%.
3. Deteccao de particao suspeita em ate 5s.
4. Recuperacao apos retorno do no ponte em ate 10s.
5. Telemetria deve registrar transicao normal -> particao -> recuperacao.

## Dependencias tecnicas

Esta feature depende do que ja foi entregue:

- pending DATA queue
- retry de descoberta para pendencias
- ACK timeout e retry

Esses mecanismos reduzem perda imediata e permitem observar comportamento real durante particao.

## Plano de execucao (futuro)

1. etapa 1: ampliar simulador com cenario de no ponte (sem firmware)
2. etapa 2: definir eventos de telemetria no firmware
3. etapa 3: validar em hardware com 3-4 ESP32
4. etapa 4: calibrar thresholds de deteccao e reconvergencia

## Observacao

Este documento registra apenas mapeamento e planejamento.
Nenhuma implementacao de articulation point foi aplicada no codigo ate este momento.
