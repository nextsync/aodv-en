---
id: TC-005
title: Cadeia de 4 nos (3 saltos)
status: ATIVO
version: 1.0
last_updated: 2026-05-01
owner: firmware
scope: hardware
applies_to: [esp32, esp-now, aodv-en]
---

# TC-005 - Cadeia de 4 nos (3 saltos)

## 1. Objetivo

Validar, em hardware real, que a malha faz entrega fim a fim em tres saltos:

- `NODE_A` envia `DATA` para `NODE_D`;
- `NODE_B` e `NODE_C` atuam como intermediarios;
- `NODE_A` recebe `ACK` de `NODE_D`.

Este caso estende `TC-002` para cadeias mais longas e ajuda a evidenciar para o TCC que o `AODV-EN` opera alem de 2 saltos.

## 2. Topologia

- `NODE_A <-> NODE_B <-> NODE_C <-> NODE_D` (cadeia linear)
- `NODE_A` e `NODE_D` devem ficar sem enlace direto
- `NODE_A` e `NODE_C` tambem devem ficar sem enlace direto
- `NODE_B` e `NODE_D` tambem devem ficar sem enlace direto
- todos no mesmo canal Wi-Fi e mesmo `network_id`

## 3. Configuracao dos nos

## NODE_A (origem de DATA para D)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_A"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=y`
- `CONFIG_AODV_EN_APP_TARGET_MAC="<MAC do NODE_D>"`
- `CONFIG_AODV_EN_APP_PAYLOAD_TEXT="tc005 chain4"`
- perfil pronto: `firmware/tests/tc005/node_a.defaults`

## NODE_B (intermediario)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_B"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=n`
- perfil pronto: `firmware/tests/tc005/node_b.defaults`

## NODE_C (intermediario)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_C"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=n`
- perfil pronto: `firmware/tests/tc005/node_c.defaults`

## NODE_D (destino de DATA)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_D"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=n`
- perfil pronto: `firmware/tests/tc005/node_d.defaults`

## Script de build/flash do caso

- `firmware/tests/tc005/build_flash.sh`
- `firmware/monitor_log.sh` (captura global em `firmware/logs/serial/`)
- `firmware/tests/tc005/monitor_log.sh` (wrapper por papel para `NODE_A/B/C/D`)

## 4. Procedimento

1. Gravar `NODE_A`:
   - `zsh firmware/tests/tc005/build_flash.sh node_a <PORTA_NODE_A> <MAC_NODE_D>`
2. Gravar `NODE_B`:
   - `zsh firmware/tests/tc005/build_flash.sh node_b <PORTA_NODE_B>`
3. Gravar `NODE_C`:
   - `zsh firmware/tests/tc005/build_flash.sh node_c <PORTA_NODE_C>`
4. Gravar `NODE_D`:
   - `zsh firmware/tests/tc005/build_flash.sh node_d <PORTA_NODE_D>`
5. Posicionar fisicamente os nos em cadeia (`A-B-C-D`) sem enlace direto fora dos vizinhos imediatos.
6. Abrir monitor serial nos quatro nos:
   - `zsh firmware/tests/tc005/monitor_log.sh node_a <PORTA_NODE_A> tc005_run`
   - `zsh firmware/tests/tc005/monitor_log.sh node_b <PORTA_NODE_B> tc005_run`
   - `zsh firmware/tests/tc005/monitor_log.sh node_c <PORTA_NODE_C> tc005_run`
   - `zsh firmware/tests/tc005/monitor_log.sh node_d <PORTA_NODE_D> tc005_run`
7. Aguardar de 60 a 180 segundos e salvar logs dos quatro nos.

## 5. Evidencias esperadas

## NODE_A

- rota para `NODE_D` com `state=2` e `hops=3`
- `ACK received from <MAC do NODE_D> ...`

## NODE_B

- rotas validas para `NODE_A` (`hops=1`) e `NODE_D` (`hops=2`)
- contagem positiva de `forwarded_frames`

## NODE_C

- rotas validas para `NODE_A` (`hops=2`) e `NODE_D` (`hops=1`)
- contagem positiva de `forwarded_frames`

## NODE_D

- `DATA deliver from <MAC do NODE_A> ...`
- `delivered > 0`

## 6. Criterio de aprovacao

`PASS` quando todos os itens abaixo forem verdadeiros:

- `NODE_A` estabelece rota para `NODE_D` com `hops=3` por pelo menos 3 janelas de log;
- `NODE_D` registra entrega de `DATA` (`delivered > 0`);
- `NODE_A` recebe `ACK` de `NODE_D`;
- nao ha reboot/restart durante a janela de observacao.

`FAIL` quando qualquer item acima nao for atendido.

## 7. Resultado (preencher a cada execucao)

- Data:
- Firmware SHA:
- MAC `NODE_A`:
- MAC `NODE_B`:
- MAC `NODE_C`:
- MAC `NODE_D`:
- Resultado: `PASS` ou `FAIL`
- Observacoes:
