---
id: TC-003
title: Reconvergencia apos falha de enlace intermediario
status: ATIVO
version: 1.0
last_updated: 2026-04-21
owner: firmware
scope: hardware
applies_to: [esp32, esp-now, aodv-en]
---

# TC-003 - Reconvergencia apos falha de enlace intermediario

## 1. Objetivo

Validar, em hardware real, que a malha reconverge apos perda temporaria do enlace intermediario:

- `NODE_A` envia `DATA` para `NODE_C` pela cadeia `A-B-C`;
- durante indisponibilidade de rota, `NODE_A` entra em descoberta/queue;
- apos retorno do caminho, a rota volta e os `ACK` retornam.

## 2. Topologia

- `NODE_A <-> NODE_B <-> NODE_C` (cadeia linear)
- `NODE_A` e `NODE_C` sem enlace direto estavel
- todos no mesmo canal Wi-Fi e mesmo `network_id`

## 3. Configuracao dos nos

## NODE_A (origem de DATA para C)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_A"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=y`
- `CONFIG_AODV_EN_APP_TARGET_MAC="<MAC do NODE_C>"`
- `CONFIG_AODV_EN_APP_PAYLOAD_TEXT="tc002 multihop"`
- perfil usado: `firmware/tests/tc002/node_a.defaults`

## NODE_B (intermediario)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_B"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=n`
- perfil usado: `firmware/tests/tc002/node_b.defaults`

## NODE_C (destino de DATA)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_C"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=n`
- perfil usado: `firmware/tests/tc002/node_c.defaults`

## Script de build/flash do caso

- `firmware/tests/tc002/build_flash.sh`
- `firmware/monitor_log.sh` (captura global em `firmware/logs/serial/`)
- `firmware/tests/tc002/monitor_log.sh` (wrapper por papel para `NODE_A/B/C`)

## 4. Procedimento

1. Gravar os nos com o mesmo fluxo do `TC-002`.
2. Iniciar captura de log, no minimo do `NODE_A`:
   - opcao global:
     - `zsh firmware/monitor_log.sh -p <PORTA_NODE_A> -B build/tc002_node_a -t tc003_reconvergencia -l node_a`
   - opcao wrapper por papel:
   - `zsh firmware/tests/tc002/monitor_log.sh node_a <PORTA_NODE_A> tc003_reconvergencia`
   - opcional: capturar tambem `NODE_B` e `NODE_C` quando houver serial disponivel.
3. Aguardar fase estavel inicial da cadeia `A-B-C`.
4. Provocar indisponibilidade temporaria do caminho intermediario (ex.: queda de enlace/interferencia/posicionamento de bancada).
5. Observar no `NODE_A` fase de descoberta com `DATA` enfileirada e ausencia temporaria de rota.
6. Restabelecer o caminho intermediario.
7. Confirmar reconvergencia com rota valida e retorno de `ACK`.

## 5. Evidencias esperadas

## NODE_A

- durante falha:
  - `DATA queued while route discovery is in progress`
  - janela com `routes=0 neighbors=0` (quando o caminho some)
- apos recuperacao:
  - rota valida para `NODE_C` via `NODE_B` com `hops=2`
  - `ACK received from <MAC do NODE_C> ...`

## NODE_B e NODE_C

- quando disponivel monitor serial:
  - `NODE_B` deve voltar a encaminhar apos reconvergencia
  - `NODE_C` deve voltar a receber `DATA` do `NODE_A`

## 6. Criterio de aprovacao

`PASS` quando todos os itens abaixo forem verdadeiros:

- houve fase de indisponibilidade observavel (descoberta/queue sem rota estavel);
- houve recuperacao automatica da rota para `NODE_C` via `NODE_B` (`hops=2`);
- `ACK` ao `NODE_A` voltou apos a recuperacao;
- nao houve reboot/reset durante a janela observada.

`FAIL` quando qualquer item acima nao for atendido.

## 7. Resultado (preencher a cada execucao)

- Data:
- Firmware SHA:
- MAC `NODE_A`:
- MAC `NODE_B`:
- MAC `NODE_C`:
- Resultado: `PASS` ou `FAIL`
- Observacoes:

### Execucao 2026-04-21 (PASS com evidencia indireta via NODE_A)

- Firmware SHA: `40c43ba-dirty`
- MAC `NODE_A`: `28:05:A5:33:D6:1C`
- MAC `NODE_B`: `28:05:A5:33:EB:80`
- MAC `NODE_C`: `28:05:A5:34:99:34`
- Resultado desta execucao: `PASS` (com evidencia indireta)
- Evidencias confirmadas:
  - fase de indisponibilidade com descoberta em progresso e sem vizinhos/rotas:
    - `DATA queued while route discovery is in progress`
    - `routes=0 neighbors=0 tx=... rx=0 delivered=0`
  - saturacao de fila durante indisponibilidade prolongada:
    - `DATA send status=-2` (`AODV_EN_ERR_FULL`)
  - recuperacao do caminho com ACKs retornando:
    - `ACK received from 28:05:A5:34:99:34 for seq=1`
    - `ACK received from 28:05:A5:34:99:34 for seq=2`
    - `ACK received from 28:05:A5:34:99:34 for seq=3`
    - `ACK received from 28:05:A5:34:99:34 for seq=4`
  - rota restabelecida via intermediario com 2 saltos:
    - `dest=28:05:A5:34:99:34 via=28:05:A5:33:EB:80 hops=2 metric=2 state=2`
  - confirmacao de continuidade apos reconvergencia:
    - `ACK received from 28:05:A5:34:99:34 for seq=5`
    - `ACK received from 28:05:A5:34:99:34 for seq=6`
- Observacao de bancada:
  - sem captura serial simultanea de `NODE_B` e `NODE_C` nesta execucao por restricao de alimentacao/infra da bancada
  - fechamento aceito com evidencias indiretas consistentes do `NODE_A`

### Caso complementar 2026-04-21 (flapping de rota observado)

- Contexto:
  - mesma topologia `A-B-C` e mesmo perfil de envio periodico do `NODE_A`
- Comportamento observado no `NODE_A`:
  - alternancia de rota para `NODE_C` entre:
    - multi-hop via intermediario: `dest=28:05:A5:34:99:34 via=28:05:A5:33:EB:80 hops=2`
    - rota direta temporaria: `dest=28:05:A5:34:99:34 via=28:05:A5:34:99:34 hops=1`
  - janelas sem rota ativa para `NODE_C` (`routes=1`, apenas rota para `NODE_B`) com retorno de `DATA queued while route discovery is in progress`
  - falhas intermitentes de enlace:
    - `ESP-NOW send fail to 28:05:A5:33:EB:80`
    - `ESP-NOW send fail to 28:05:A5:34:99:34`
    - `DATA send status=-2` em janelas prolongadas de instabilidade (`AODV_EN_ERR_FULL`)
  - recuperacao recorrente com retomada de `ACK` e restabelecimento de rota
- Interpretacao tecnica:
  - evidencia de flapping de rota por variacao de qualidade de enlace em bancada (RF dinamico)
  - mecanismo de reconvergencia permaneceu funcional, com retorno automatico de entrega/ACK apos oscilacoes
- Uso no TCC:
  - este caso documenta comportamento realista de malha sem fio em ambiente nao controlado: alternancia de caminho, perda temporaria e autorrecuperacao
