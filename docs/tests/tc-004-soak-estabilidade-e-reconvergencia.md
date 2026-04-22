---
id: TC-004
title: Soak test de estabilidade com reconvergencia ciclica
status: ATIVO
version: 1.0
last_updated: 2026-04-21
owner: firmware
scope: hardware
applies_to: [esp32, esp-now, aodv-en]
---

# TC-004 - Soak test de estabilidade com reconvergencia ciclica

## 1. Objetivo

Validar estabilidade operacional em janela estendida, com ciclos repetidos de degradacao e recuperacao de rota:

- medir continuidade de entrega/ACK sob oscilacao de enlace;
- medir reconvergencia automatica em cada ciclo;
- quantificar eventos de falha (`ESP-NOW send fail`, `DATA send status=-2`) sem reboot.
- `status=-2` deve ser interpretado como `AODV_EN_ERR_FULL` (fila/buffer de pending DATA cheio).

## 2. Topologia

- `NODE_A <-> NODE_B <-> NODE_C` (cadeia principal)
- preferencialmente com baixa conectividade direta `A <-> C` (para privilegiar caminho multi-hop)
- todos no mesmo canal Wi-Fi e mesmo `network_id`

## 3. Configuracao dos nos

Usar os mesmos perfis do `TC-002`/`TC-003`:

- `NODE_A`: `firmware/tests/tc002/node_a.defaults`
- `NODE_B`: `firmware/tests/tc002/node_b.defaults`
- `NODE_C`: `firmware/tests/tc002/node_c.defaults`
- build/flash: `firmware/tests/tc002/build_flash.sh`

## 4. Ferramentas de captura

- monitor global: `firmware/monitor_log.sh`
- monitor por papel (wrapper): `firmware/tests/tc002/monitor_log.sh`
- logs salvos em: `firmware/logs/serial/`

## 5. Procedimento (passo a passo com tempo)

Duracao total sugerida: `30 minutos`.

1. `T-00:00` a `T-03:00`:
   - gravar os 3 nos e iniciar monitor do `NODE_A` com arquivo:
   - `zsh firmware/monitor_log.sh -p <PORTA_NODE_A> -B build/tc002_node_a -t tc004_soak -l node_a`
   - opcional: iniciar tambem `NODE_B` e `NODE_C`.
2. `T-03:00` a `T-08:00` (baseline estavel):
   - manter topologia estavel;
   - confirmar `DATA queued to route` e `ACK received ...`.
3. `T-08:00` a `T-10:00` (ciclo 1 - degradacao):
   - provocar degradacao do caminho intermediario (ex.: afastar/interferir `NODE_B`);
   - observar descoberta/fila (`DATA queued while route discovery is in progress`).
4. `T-10:00` a `T-14:00` (ciclo 1 - recuperacao):
   - restabelecer caminho;
   - observar retorno de rota e ACK.
5. `T-14:00` a `T-16:00` (ciclo 2 - degradacao):
   - repetir degradacao.
6. `T-16:00` a `T-20:00` (ciclo 2 - recuperacao):
   - restabelecer e observar reconvergencia.
7. `T-20:00` a `T-22:00` (ciclo 3 - degradacao):
   - repetir degradacao.
8. `T-22:00` a `T-26:00` (ciclo 3 - recuperacao):
   - restabelecer e observar reconvergencia.
9. `T-26:00` a `T-30:00` (fechamento):
   - manter estavel e encerrar captura.

## 6. Evidencias esperadas

No `NODE_A`:

- baseline com `ACK received ...` recorrente;
- em degradacao: aumento de `DATA queued while route discovery is in progress`;
- em recuperacao: retorno de `ACK` e rota para `NODE_C` (`hops=2` via `NODE_B` ou `hops=1` quando houver enlace direto temporario);
- ausencia de reboot durante toda a janela.

## 7. Criterio de aprovacao

`PASS` quando todos os itens abaixo forem verdadeiros:

- sem reboot/reset durante os 30 minutos;
- reconvergencia observada nos 3 ciclos apos restabelecimento;
- retorno de `ACK` em todos os ciclos de recuperacao;
- `DATA send status=-2` (fila/buffer cheio) ocorre, no maximo, de forma esporadica e sem travar recuperacao.

`FAIL` quando qualquer item acima nao for atendido.

## 8. Extracao rapida de metricas (log do NODE_A)

Assumindo log em `firmware/logs/serial/node_a_tc004_soak_<timestamp>.log`:

```bash
python3 firmware/tools/extract_monitor_metrics.py \
  firmware/logs/serial/node_a_tc004_soak_<timestamp>.log
```

Arquivos gerados automaticamente:

- `firmware/logs/analysis/node_a_tc004_soak_<timestamp>/summary.json`
- `firmware/logs/analysis/node_a_tc004_soak_<timestamp>/summary.txt`
- `firmware/logs/analysis/node_a_tc004_soak_<timestamp>/*.csv`

Geracao automatica de graficos (PNG):

```bash
python3 firmware/tools/plot_monitor_metrics.py \
  firmware/logs/analysis/node_a_tc004_soak_<timestamp>
```

PNG em:

- `firmware/logs/analysis/node_a_tc004_soak_<timestamp>/plots/`
- guia de leitura: `docs/tests/guia-leitura-graficos-monitor.md`
- formulas das metricas: secao `Formulas e como chegar nos resultados` em `docs/tests/guia-leitura-graficos-monitor.md`

Comparativo entre multiplas execucoes (com marcador pre/pós mudanca de codigo):

```bash
python3 firmware/tools/plot_comparison_metrics.py \
  --run "exec_1::firmware/logs/analysis/node_a_tc004_soak_<ts1>/summary.json" \
  --run "exec_2::firmware/logs/analysis/node_a_tc004_soak_<ts2>/summary.json" \
  --run "exec_pos_fix::firmware/logs/analysis/node_a_tc004_soak_v2_<ts3>/summary.json" \
  --change-index 2 \
  --out-dir firmware/logs/analysis/comparisons/tc004_exec1_exec2_posfix
```

Arquivos comparativos:

- `firmware/logs/analysis/comparisons/<nome>/comparison_metrics.csv`
- `firmware/logs/analysis/comparisons/<nome>/01_core_metrics.png`
- `firmware/logs/analysis/comparisons/<nome>/02_stability_metrics.png`
- `firmware/logs/analysis/comparisons/<nome>/03_route_profile.png`

Extracao manual complementar:

```bash
LOG="firmware/logs/serial/node_a_tc004_soak_<timestamp>.log"
rg -c "ACK received" "$LOG"
rg -c "DATA queued while route discovery is in progress" "$LOG"
rg -c "DATA send status=-2" "$LOG"
rg -c "ESP-NOW send fail to" "$LOG"
rg -c "hops=2" "$LOG"
rg -c "hops=1" "$LOG"
```

## 9. Resultado (preencher a cada execucao)

- Data:
- Firmware SHA:
- MAC `NODE_A`:
- MAC `NODE_B`:
- MAC `NODE_C`:
- Duracao real:
- Resultado: `PASS` ou `FAIL`
- Observacoes:
