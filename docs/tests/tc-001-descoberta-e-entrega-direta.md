---
id: TC-001
title: Descoberta de vizinho e entrega direta (1 salto)
status: ATIVO
version: 1.0
last_updated: 2026-04-21
owner: firmware
scope: hardware
applies_to: [esp32, esp-now, aodv-en]
---

# TC-001 - Descoberta de vizinho e entrega direta (1 salto)

## 1. Objetivo

Validar, em hardware real, que dois nos:

- se descobrem via `HELLO`;
- instalam rota valida (`state=2`) com `hops=1`;
- entregam `DATA` fim a fim com `ACK`.

## 2. Topologia

- `NODE_A <-> NODE_B` (alcance direto)
- ambos no mesmo canal Wi-Fi e mesmo `network_id`

## 3. Configuracao dos nos

## NODE_A (origem de DATA)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_A"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=y`
- `CONFIG_AODV_EN_APP_TARGET_MAC="<MAC do NODE_B>"`
- `CONFIG_AODV_EN_APP_PAYLOAD_TEXT="tc001 hello"`
- perfil pronto: `firmware/tests/tc001/node_a.defaults`

## NODE_B (destino de DATA)

- `CONFIG_AODV_EN_APP_NODE_NAME="NODE_B"`
- `CONFIG_AODV_EN_APP_NETWORK_ID=0xA0DE0001`
- `CONFIG_AODV_EN_APP_WIFI_CHANNEL=6`
- `CONFIG_AODV_EN_APP_ENABLE_DATA=n`
- perfil pronto: `firmware/tests/tc001/node_b.defaults`

## Script de build/flash do caso

- `firmware/tests/tc001/build_flash.sh`

## 4. Procedimento

1. Gravar `NODE_A`:
   - `zsh firmware/tests/tc001/build_flash.sh node_a <PORTA_NODE_A> <MAC_NODE_B>`
2. Gravar `NODE_B`:
   - `zsh firmware/tests/tc001/build_flash.sh node_b <PORTA_NODE_B>`
3. Abrir monitor serial em ambos os nos:
   - `idf.py -B firmware/build/tc001_node_a -p <PORTA_NODE_A> monitor`
   - `idf.py -B firmware/build/tc001_node_b -p <PORTA_NODE_B> monitor`
4. Aguardar de 30 a 60 segundos com os dois ligados.
5. Salvar log serial de cada no.

## 5. Evidencias esperadas

## NODE_A

- `neighbors >= 1`
- `routes >= 1`
- rota para MAC do `NODE_B` com `state=2` e `hops=1`
- eventos de `ACK received from ...`

## NODE_B

- `neighbors >= 1`
- `routes >= 1`
- logs `DATA deliver from ...`
- `delivered > 0`

## 6. Criterio de aprovacao

`PASS` quando todos os itens abaixo forem verdadeiros:

- os dois nos mantem rota valida entre si por pelo menos 3 janelas de log consecutivas;
- `NODE_B` registra entrega de `DATA` (`delivered > 0`);
- `NODE_A` recebe `ACK` correspondente ao menos uma vez;
- nao ha reboot/restart durante a janela de observacao.

`FAIL` quando qualquer item acima nao for atendido.

## 7. Resultado (preencher a cada execucao)

- Data: `2026-04-21`
- Firmware SHA: `40c43ba-dirty`
- MAC `NODE_A`: `28:05:A5:33:D6:1C`
- MAC `NODE_B`: `28:05:A5:33:EB:80`
- Resultado: `PASS`
- Observacoes:
  - `NODE_A` enviou `DATA` periodico para `NODE_B` e recebeu `ACK` sequenciais (`seq=1..5` no trecho observado).
  - `NODE_B` registrou entregas repetidas de payload (`DATA deliver ... tc001 hello`) e `delivered` evoluiu de `1` para `17` no trecho observado.
  - Rota permaneceu valida em ambos os nos (`state=2`, `hops=1`, `neighbors=1`, `routes=1`) sem reboot durante a janela de observacao.
