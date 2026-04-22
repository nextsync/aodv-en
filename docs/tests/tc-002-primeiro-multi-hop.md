---
id: TC-002
title: Primeiro multi-hop em cadeia (2 saltos)
status: ATIVO
version: 1.0
last_updated: 2026-04-21
owner: firmware
scope: hardware
applies_to: [esp32, esp-now, aodv-en]
---

# TC-002 - Primeiro multi-hop em cadeia (2 saltos)

## 1. Objetivo

Validar, em hardware real, que a malha faz entrega fim a fim em dois saltos:

- `NODE_A` envia `DATA` para `NODE_C`;
- `NODE_B` atua como intermediario;
- `NODE_A` recebe `ACK` de `NODE_C`.

## 2. Topologia

- `NODE_A <-> NODE_B <-> NODE_C` (cadeia linear)
- `NODE_A` e `NODE_C` devem ficar sem enlace direto
- todos no mesmo canal Wi-Fi e mesmo `network_id`

## 3. Configuracao dos nos

## NODE_A (origem de DATA para C)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_A"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=y`
- `CONFIG_AODV_EN_APP_TARGET_MAC="<MAC do NODE_C>"`
- `CONFIG_AODV_EN_APP_PAYLOAD_TEXT="tc002 multihop"`
- perfil pronto: `firmware/tests/tc002/node_a.defaults`

## NODE_B (intermediario)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_B"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=n`
- perfil pronto: `firmware/tests/tc002/node_b.defaults`

## NODE_C (destino de DATA)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_C"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=n`
- perfil pronto: `firmware/tests/tc002/node_c.defaults`

## Script de build/flash do caso

- `firmware/tests/tc002/build_flash.sh`
- `firmware/monitor_log.sh` (captura global em `firmware/logs/serial/`)
- `firmware/tests/tc002/monitor_log.sh` (wrapper por papel para `NODE_A/B/C`)

## 4. Procedimento

1. Gravar `NODE_A`:
   - `zsh firmware/tests/tc002/build_flash.sh node_a <PORTA_NODE_A> <MAC_NODE_C>`
2. Gravar `NODE_B`:
   - `zsh firmware/tests/tc002/build_flash.sh node_b <PORTA_NODE_B>`
3. Gravar `NODE_C`:
   - `zsh firmware/tests/tc002/build_flash.sh node_c <PORTA_NODE_C>`
4. Posicionar fisicamente os nos em cadeia (`A-B-C`) sem enlace direto entre `A` e `C`.
5. Abrir monitor serial nos tres nos (preferencialmente com captura de log):
   - opcao global:
     - `zsh firmware/monitor_log.sh -p <PORTA_NODE_A> -B build/tc002_node_a -t tc002_run -l node_a`
     - `zsh firmware/monitor_log.sh -p <PORTA_NODE_B> -B build/tc002_node_b -t tc002_run -l node_b`
     - `zsh firmware/monitor_log.sh -p <PORTA_NODE_C> -B build/tc002_node_c -t tc002_run -l node_c`
   - opcao wrapper por papel:
   - `zsh firmware/tests/tc002/monitor_log.sh node_a <PORTA_NODE_A> tc002_run`
   - `zsh firmware/tests/tc002/monitor_log.sh node_b <PORTA_NODE_B> tc002_run`
   - `zsh firmware/tests/tc002/monitor_log.sh node_c <PORTA_NODE_C> tc002_run`
   - (alternativa sem arquivo: `idf.py -B firmware/build/tc002_node_a -p <PORTA_NODE_A> monitor` etc.)
6. Aguardar de 60 a 120 segundos e salvar logs dos tres nos.

## 5. Evidencias esperadas

## NODE_A

- rota para `NODE_C` com `state=2` e `hops=2`
- `ACK received from <MAC do NODE_C> ...`

## NODE_B

- `neighbors >= 2`
- rotas validas para `NODE_A` e `NODE_C` (tipicamente `hops=1`)

## NODE_C

- `DATA deliver from <MAC do NODE_A> ...`
- `delivered > 0`

## 6. Criterio de aprovacao

`PASS` quando todos os itens abaixo forem verdadeiros:

- `NODE_A` estabelece rota para `NODE_C` com `hops=2` por pelo menos 3 janelas de log;
- `NODE_C` registra entrega de `DATA` (`delivered > 0`);
- `NODE_A` recebe `ACK` de `NODE_C`;
- nao ha reboot/restart durante a janela de observacao.

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
  - fase de indisponibilidade seguida de reconvergencia:
    - inicio com `routes=0 neighbors=0` e `DATA queued while route discovery is in progress`
    - evento de fila cheia observado em janela de descoberta prolongada: `DATA send status=-2` (`AODV_EN_ERR_FULL`)
    - recuperacao posterior com ACKs em sequencia (`seq=1..4`) e retorno de rota valida
  - rota ativa para `NODE_C` com 2 saltos via `NODE_B`:
    - `dest=28:05:A5:34:99:34 via=28:05:A5:33:EB:80 hops=2 metric=2 state=2`
  - ACKs fim a fim recebidos no `NODE_A`:
    - `ACK received from 28:05:A5:34:99:34 for seq=1`
    - `ACK received from 28:05:A5:34:99:34 for seq=2`
    - `ACK received from 28:05:A5:34:99:34 for seq=3`
    - `ACK received from 28:05:A5:34:99:34 for seq=4`
    - `ACK received from 28:05:A5:34:99:34 for seq=5`
- Observacao de bancada:
  - sem captura serial simultanea de `NODE_B` e `NODE_C` nesta execucao por restricao de alimentacao/infra da bancada
  - fechamento aceito com evidencias indiretas consistentes do `NODE_A` (rota `hops=2 via NODE_B` + ACKs do `NODE_C`)
