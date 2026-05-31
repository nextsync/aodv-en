# AODV-EN vs Flooding — comparacao (estado em 2026-05-31)

> Documento vivo. Parte SIM e dado real medido; parte HARDWARE esta bloqueada em
> decisoes do humano (ver results/QUESTIONS.md). NENHUM numero foi inventado.

## 1. Comparacao por SIMULACAO (REAL, deterministica) — disponivel

Fonte: `bash sim/run_sim.sh compare` (sim/compare_sim.c). Os dois nucleos
(aodv_en_node e flood_en_node) rodam na MESMA topologia em grade, origem->destino
em cantos opostos, 10 pacotes DATA com ACK. Conta-se cada transmissao no ar.
Grafico: results/charts/sim-tx-per-delivered.png. CSV: results/m-bench-sim-compare.csv.

| Grid | Nos | Hops | Entrega AODV | Entrega Flood | TX/entrega AODV | TX/entrega Flood |
|------|-----|------|--------------|---------------|-----------------|------------------|
| 2x2  | 4   | 2    | 10/10        | 10/10         | 4.50            | 6.00             |
| 3x3  | 9   | 4    | 10/10        | 10/10         | 17.60           | 16.00            |
| 4x4  | 16  | 6    | 10/10        | 10/10         | 26.70           | 30.00            |
| 5x5  | 25  | 8    | 10/10        | 10/10         | 35.60           | 41.00            |

Leitura: entrega 100% nos dois em todas as escalas. Custo de canal (tx/entrega) tem
cruzamento ~9-11 nos: flooding e competitivo em rede pequena/densa, AODV-EN escala
melhor (menos tx/entrega) conforme cresce. Numeros do flood inalterados apos o fix de
ACK (commit d3d34eb) -> fix neutro no caso single-origin do compare_sim.

## 2. Comparacao por HARDWARE (3 ESP32) — BLOQUEADA

`experiment compare aodv-en flooding` retorna 0 runs: NAO ha metricas HW no ledger,
de proposito. As 4 metricas do TCC (PDR, latencia, NRL, energia) NAO podem vir do log
serial real atual sem decisoes do humano:
- Q3: formulas PDR/NRL + instrumentacao por-pacote (telemetria nao loga seq+t_send na
  origem e seq+t_recv no destino, logo latencia fim-a-fim e incalculavel hoje).
- Q6: constantes do modelo de energia (V, I_tx, I_rx, P_idle, t_tx).
- Q1/Q2: params do flooding (transporte broadcast vs unicast-por-vizinho; TTL=5;
  dedup=100; payload 32B; 1 pkt/s) — divergem do baseline atual.
- extract_monitor_metrics.py (Q4) nao parseia a telemetria do app_flood.

Dados HW REAIS ja coletados (contadores, nao as 4 metricas) ficam em:
- AODV: results/m8-aodv-N2/summary.json
- Flooding: results/m9-flood-metrics.json (pos-fix: N2/N3 ack simetricos)

Quando Q1-Q6 forem decididas: alinhar params do flooding, instrumentar telemetria
por-pacote, estender o extractor, re-coletar (m8/m9) e entao popular o ledger e
re-rodar este compare para a tabela HW.

## 3. Achados de correcao ja aplicados
- fix(flood) d3d34eb: dedup de ACK por (origem-do-DATA, seq). Antes, 2 origens ao mesmo
  destino com seq sobrepostos colidiam e uma ficava sem ACK (HW: N2 ack=0). Pos-fix HW:
  N2 ack=30, N3 ack=30 (simetrico). Relevante para NRL/PDR do flooding multi-origem.
