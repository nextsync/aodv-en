# M1 — Validacao do frontend realtime (live_monitor + dashboard)

Data: 2026-05-30 21:35 (sessao autopilot)
Branch: `autopilot/2026-05-30-2129`

## Objetivo

Validar o dashboard realtime (`firmware/tools/live_monitor.py` + Cytoscape.js) com ESPs
reais conectados na serial, via Playwright.

## Setup

3 ESPs conectados via USB serial:

| Porta | Alias monitor | MAC | Papel |
|---|---|---|---|
| `/dev/cu.usbserial-214420` | N1 | `28:05:A5:34:99:34` | NODE_C |
| `/dev/cu.usbserial-214430` | N2 | `28:05:A5:33:EB:80` | NODE_A (origem, has_target) |
| `/dev/cu.usbserial-214440` | N3 | `28:05:A5:33:D6:1C` | NODE_B (relay) |

Comando:

```
$IDFPY firmware/tools/live_monitor.py \
  --port /dev/cu.usbserial-214420:N1 \
  --port /dev/cu.usbserial-214430:N2 \
  --port /dev/cu.usbserial-214440:N3 -v
```

`$IDFPY = /Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env/bin/python` (aiohttp 3.13.5).

## Resultado — PASS

Estado capturado via Playwright em `http://localhost:8765/`:

- WebSocket: **conectado**
- NOS ONLINE: **3** / NOS CONHECIDOS: **3**
- HOPS MAX: **1** (todos vizinhos diretos no mesmo hub)
- ROTAS VALIDAS: **6/6**
- Contadores por no subindo em tempo real:
  - NODE_A `tx 77 rx 151` · 2r 2n
  - NODE_B `tx 74 rx 148` · 2r 2n
  - NODE_C `tx 78 rx 150` · 2r 2n
- Timeline de eventos streaming (`queued_discovery NODE_A` a cada ~2s)
- Grafo de topologia: NODE_C — NODE_A — NODE_B (NODE_A central)

Screenshot: `m1-dashboard-realtime-3nodes.png`

## Observacoes

1. **DATA ENTREGUE=0 / ACK=0**: NODE_A (`33:EB:80`) tem `target_mac` apontando para
   o NODE_D do colega (`28:05:A5:33:B9:EC`), que **nao esta presente** nesta bancada.
   Por isso NODE_A fica em `queued_discovery` perpetuo (RREQ para destino ausente),
   sem DATA entregue nem ACK. Nao e bug do frontend — e topologia (alvo ausente).
   Para ver ACK subindo, reflashar NODE_A com TARGET_MAC de um no presente.

2. **HOPS MAX=1**: os 3 ESPs estao no mesmo hub, todos se enxergam direto. Para
   exercitar multi-hop e preciso separar fisicamente (afastar NODE_C ou NODE_B).

3. **Gotcha `--skip-mac-lookup`**: com esse flag e ESPs ja bootados (sem linha
   `node=` recente), o alias->mac nunca mapeia e o dashboard fica vazio
   ("Nenhum no detectado ainda") mesmo com a serial produzindo logs. Rodar SEM o
   flag (esptool pre-le o MAC e reseta o ESP, gerando boot `node=`) resolve.
   Tambem evitar abrir a mesma porta serial em dois processos (corrompe os bytes:
   `device reports readiness to read but returned no data ... multiple access`).
