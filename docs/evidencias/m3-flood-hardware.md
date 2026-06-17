# M3 — Validacao do flooding controlado (sim + hardware)

Data: 2026-05-30 (sessao autopilot)
Branch: `autopilot/2026-05-30-2129`

## Sim

`bash sim/run_sim.sh flood` → exit 0, "Flood simulation passed."
Cadeia A-B-C sem rotas: entrega 2/2, ACK 2/2, dup_drops>=1 por rodada, ttl_drops=0.

## Hardware

3 ESP32 gravados no modo flooding (`app_flood`, Kconfig `AODV_EN_APP_USE_APP_FLOOD`),
todos com `TARGET_MAC=28:05:A5:34:99:34` (no presente N1):

| Porta | MAC | Papel | Observado |
|---|---|---|---|
| 214420 | 28:05:A5:34:99:34 | target (self==target -> relay) | delivered crescente (24->30), "DATA deliver from 33:D6:1C" e "from 33:EB:80", rebroadcast=0 |
| 214430 | 28:05:A5:33:EB:80 | origem | "flood DATA broadcast" + "ACK received from 34:99:34 seq=47..50" |
| 214440 | 28:05:A5:33:D6:1C | origem | "flood DATA broadcast" + "ACK received from 34:99:34 seq=11..14" |

### Fim a fim — PASS

DATA disseminado por broadcast a partir de N2/N3 -> entregue em N1 (callback on_data)
-> N1 floda ACK de volta -> N2/N3 recebem ACK. Sem rotas, sem RREQ/RREP.

### Ausencia de tempestade de broadcast — PASS

- `dup` (duplicatas descartadas) cresce continuamente: supressao por (originador,seq)
  ativa, impedindo reflood infinito.
- `ttl_drop=0`: todos os links sao 1-hop no hub (TTL nunca esgota); o mecanismo de
  limite por TTL esta presente no codigo para topologias multi-hop.
- `rebroadcast` cresce de forma linear com a carga oferecida (N2~26, N3~28), nao
  exponencial. N1 (destino de ambos os fluxos) nao reencaminha (rebroadcast=0).
- Relacao tx/rx limitada: sem crescimento explosivo.

Trecho de captura (16 s, 3 portas em paralelo):

```
N1: stats tx=24 rx=61 rebroadcast=0 delivered=24 ack=0 dup=37 ttl_drop=0
N1: DATA deliver from 28:05:A5:33:D6:1C: flood baseline
N1: DATA deliver from 28:05:A5:33:EB:80: flood baseline
N2: ACK received from 28:05:A5:34:99:34 for seq=47
N2: stats tx=67 rx=161 rebroadcast=20 delivered=0 ack=14 dup=22 ttl_drop=0
N3: ACK received from 28:05:A5:34:99:34 for seq=13
N3: stats tx=41 rx=67 rebroadcast=28 delivered=0 ack=13 dup=26 ttl_drop=0
```

## Como reproduzir

```
export ESP_IDF_EXPORT=~/.espressif/v6.0/esp-idf/export.sh
zsh firmware/tests/flood/build_flash.sh /dev/cu.usbserial-XXXX 28:05:A5:34:99:34
```

Observacao de bancada: 3 ESPs no mesmo hub ficam todos a 1 hop (sem multi-hop real).
Para exercitar TTL/forward multi-hop e preciso separar fisicamente os nos (ver m4/m5).
