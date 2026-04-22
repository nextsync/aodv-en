# Guia de Leitura dos Graficos de Monitor

Este guia documenta como interpretar os PNG gerados por:

```bash
python3 firmware/tools/plot_monitor_metrics.py <analysis_dir>
```

Exemplo de pasta:

- `firmware/logs/analysis/node_a_tc004_soak_<timestamp>/plots/`

## Escopo

- Visao principal focada no `NODE_A` (origem do log serial analisado).
- Os graficos sao derivados dos CSV em `firmware/logs/analysis/<run>/`.
- O mapeamento oficial dos arquivos gerados fica em `plots/manifest.json`.

## Desenho da topologia (Mermaid/DOT)

Para gerar um desenho da topologia observada nos logs analisados:

```bash
python3 firmware/tools/draw_topology.py <analysis_dir> --mode latest
```

Arquivos gerados em `<analysis_dir>/topology/`:

- `topology.mmd` (Mermaid)
- `topology.dot` (Graphviz DOT)
- `topology.svg` (quando o comando `dot` estiver instalado)
- `topology.json` (resumo estruturado de nos, enlaces e snapshot usado)

Modos:

- `--mode latest`: desenha apenas o ultimo snapshot (estado final).
- `--mode observed`: desenha a uniao dos enlaces vistos ao longo da execucao.

## 01 - ACK vs Fail por Minuto

Arquivo:

- `01_ack_vs_fail_per_minute.png`

Pergunta que responde:

- O enlace esta entregando com regularidade ou esta degradando?

Eixos:

- X: minuto da execucao.
- Y: quantidade de eventos no minuto.
- Curvas: `ACK/min` e `ESP-NOW fail/min`.

Leitura pratica:

- Bom: `ACK` consistente e `fail` baixo.
- Alerta: queda de `ACK` junto de pico de `fail`.
- Critico: longos periodos com `ACK=0` e `fail` alto.

CSV de apoio:

- `minute_metrics.csv` (`ack_count`, `send_fail_count`).

## 02 - Fila de DATA e status=-2 por Minuto

Arquivo:

- `02_queue_and_status_per_minute.png`

Pergunta que responde:

- O no esta pressionando fila por falta de rota?

Eixos:

- X: minuto.
- Y esquerdo: eventos de fila (`queued_to_route`, `queued_discovery`).
- Y direito: `status=-2` por minuto.

Leitura pratica:

- Bom: fila oscila, mas `status=-2` e raro.
- Alerta: `queued_discovery` alto por varios minutos.
- Critico: `status=-2` recorrente + baixa taxa de ACK.

Definicao importante:

- `status=-2` corresponde a `AODV_EN_ERR_FULL`, ou seja, **fila/buffer interno de pending DATA cheio**.
- Interpretacao recomendada para relatorio: evento de **pressao de memoria de fila** (capacidade interna do componente), e nao erro generico.

CSV de apoio:

- `minute_metrics.csv` (`queued_route_count`, `queued_discovery_count`, `status_neg2_count`).

## 03 - Timeline do Estado de Rota Alvo

Arquivo:

- `03_route_state_timeline.png`

Pergunta que responde:

- A rota para o destino alvo esta estavel, ausente, ou flapeando?

Eixos:

- X: tempo em segundos.
- Y: estado (`absent`, `hops=1`, `hops=2`, `other`).

Leitura pratica:

- Bom: blocos longos no mesmo estado.
- Alerta: alternancia frequente entre `hops=1` e `hops=2`.
- Critico: trechos longos em `absent`.

CSV de apoio:

- `target_route_series.csv`.
- `target_route_transitions.csv` (quantifica transicoes/flaps).

## 04 - Distribuicao de Estado da Rota Alvo

Arquivo:

- `04_route_state_distribution.png`

Pergunta que responde:

- Qual estado predominou no teste completo?

Eixos:

- X: categorias (`hops=2`, `hops=1`, `absent`).
- Y: numero de snapshots.

Leitura pratica:

- Bom: alta proporcao de rota presente (`hops=1` ou `hops=2`).
- Alerta: proporcao alta de `absent`.
- Uso tipico em relatorio: percentual de tempo com rota disponivel.

CSV de apoio:

- `target_route_series.csv` (`state_label`).

## 05 - Media de Neighbors e Rotas por Minuto

Arquivo:

- `05_neighbors_routes_per_minute.png`

Pergunta que responde:

- O no esta enxergando vizinhos/rotas de forma continua?

Eixos:

- X: minuto.
- Y: media por snapshot no minuto.
- Curvas: `avg_neighbors` e `avg_routes`.

Leitura pratica:

- Bom: curvas acima de zero e sem quedas longas.
- Alerta: quedas repetidas para valores baixos.
- Critico: varios minutos proximos de zero (no "cego").

CSV de apoio:

- `minute_metrics.csv` (`avg_neighbors`, `avg_routes`).

## 06 - Duracao das Janelas de Discovery

Arquivo:

- `06_discovery_window_durations.png`

Pergunta que responde:

- Quanto tempo a reconvergencia leva em cada janela?

Eixos:

- X: indice da janela de discovery.
- Y: duracao em segundos.
- Linhas: media e maximo.

Leitura pratica:

- Bom: duracoes curtas e distribuidas de forma estavel.
- Alerta: cauda longa (algumas janelas muito maiores que a media).
- Critico: maximos altos repetidos (reconvergencia lenta/intermitente).

CSV de apoio:

- `discovery_windows.csv` (`duration_ms`).

## 07 - Evolucao de TX/RX (Delta no Tempo)

Arquivo:

- `07_tx_rx_delta_over_time.png`

Pergunta que responde:

- O volume transmitido cresce com recepcao proporcional?

Eixos:

- X: tempo em segundos.
- Y: delta acumulado desde o primeiro snapshot.
- Curvas: `tx_delta` e `rx_delta`.

Leitura pratica:

- Bom: crescimento de TX e RX com distancia moderada.
- Alerta: gap crescente de TX >> RX.
- Critico: TX sobe forte com RX quase estagnado (overhead/retries).

CSV de apoio:

- `snapshots.csv` (`tx_delta`, `rx_delta`).

## Checklist rapido de leitura

1. Verificar `01` (saude de entrega: ACK vs fail).
2. Verificar `02` (pressao de fila e `status=-2`).
3. Verificar `03` + `04` (estabilidade e disponibilidade de rota alvo).
4. Verificar `06` (tempo de reconvergencia).
5. Fechar com `07` (eficiencia TX/RX no periodo total).

## Observacao metodologica

- Em bancada real com oscilacao RF, variacao entre `hops=1` e `hops=2` pode ocorrer.
- O objetivo nao e eliminar toda variacao, e sim manter entrega/ACK e reconvergencia dentro de limites aceitaveis.

## Formulas e como chegar nos resultados

Esta secao mostra exatamente como os indicadores sao calculados a partir dos artefatos de analise.

### Fontes de verdade (arquivos)

- `summary.json`: agregados finais da execucao.
- `minute_metrics.csv`: contagens por minuto.
- `target_route_series.csv`: estado da rota alvo por snapshot.
- `target_route_transitions.csv`: transicoes de estado (flaps).
- `discovery_windows.csv`: duracao de janelas de descoberta.

### Notacao usada

- `D_min`: duracao total em minutos (`summary.timeline.duration_min`).
- `D_h`: duracao total em horas (`D_min / 60`).
- `ACK`: total de ACK (`summary.counts.acks`).
- `FAIL`: total de falhas ESP-NOW (`summary.counts.espnow_send_fail`).
- `Q_disc`: total de `data_queued_discovery` (`summary.counts.data_queued_discovery`).
- `FLAP`: total de transicoes de estado da rota alvo (`summary.route_target_analysis.flap_count`).
- `FULL_-2`: total de eventos `status=-2` (`summary.data_send_status_by_code["-2"]`).
- `S_abs`: snapshots sem rota alvo (`summary.route_target_analysis.snapshots_without_target`).
- `S_pres`: snapshots com rota alvo (`summary.route_target_analysis.snapshots_with_target`).
- `S_total = S_abs + S_pres`.

### Formulas principais (comparativo entre execucoes)

1. `ack_per_min`

```text
ack_per_min = ACK / D_min
```

2. `send_fail_per_min`

```text
send_fail_per_min = FAIL / D_min
```

3. `status_neg2_per_min`

```text
status_neg2_per_min = FULL_-2 / D_min
```

Onde `status=-2` significa `AODV_EN_ERR_FULL` (fila/buffer interno cheio).

4. `queued_discovery_per_min`

```text
queued_discovery_per_min = Q_disc / D_min
```

5. `flap_per_hour`

```text
flap_per_hour = FLAP / D_h
```

6. `target_absent_ratio`

```text
target_absent_ratio = S_abs / S_total
```

7. `hops1_ratio` e `hops2_ratio`

```text
hops1_ratio = hops_1_count / S_total
hops2_ratio = hops_2_count / S_total
```

8. `absent_window_max_s`

```text
absent_window_max_s = absent_window_max_ms / 1000
```

(`absent_window_max_ms` vem de `summary.route_target_analysis.absent_window_max_ms`).

### Como o flap e contado

Um flap e cada troca de estado consecutiva em `target_route_series.csv`, por exemplo:

- `hops_2 -> hops_1`
- `hops_1 -> absent`
- `absent -> hops_2`

Cada troca soma `1` em `flap_count`.

### Validacao rapida (manual)

1. Abrir `summary.json`.
2. Copiar `duration_min`, `counts` e `route_target_analysis`.
3. Aplicar as formulas acima.
4. Conferir se bate com:
- `comparison_metrics.csv` (quando houver comparativo),
- ou com os valores exibidos no `summary.txt`.

### Exemplo de reproducao por script

```bash
python3 - <<'PY'
import json
from pathlib import Path

p = Path("firmware/logs/analysis/node_a_tc004_soak_<timestamp>/summary.json")
s = json.loads(p.read_text())

D_min = s["timeline"]["duration_min"]
D_h = D_min / 60.0
ACK = s["counts"]["acks"]
FAIL = s["counts"]["espnow_send_fail"]
Q_disc = s["counts"]["data_queued_discovery"]
FLAP = s["route_target_analysis"]["flap_count"]
S_abs = s["route_target_analysis"]["snapshots_without_target"]
S_pres = s["route_target_analysis"]["snapshots_with_target"]
S_total = S_abs + S_pres
status_neg2 = int(s.get("data_send_status_by_code", {}).get("-2", 0))

print("ack_per_min =", ACK / D_min)
print("send_fail_per_min =", FAIL / D_min)
print("status_neg2_per_min =", status_neg2 / D_min)
print("queued_discovery_per_min =", Q_disc / D_min)
print("flap_per_hour =", FLAP / D_h)
print("target_absent_ratio =", S_abs / S_total if S_total else 0)
PY
```
