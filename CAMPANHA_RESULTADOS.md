# Resultados da campanha HW (10 ESP32 reais)

Dados reais coletados via telemetria in-band (STATSREP/NEIGHREP), metricas por
`tcc_metrics.py` (delta-on-host), agregacao media/desvio/IC95 sobre as repeticoes.
Energia = estimativa de datasheet (rotulada, nao medida). Per-rep logs e prints
ficam em `results/` (gitignored); aqui so o agregado (dado da tese).

## C1 Linear (cadeia multi-hop) — 10 nos

- Topologia: 10 nos (EB80 origem+coletor + 9 reporters). Destino D61C alcancado
  em 3-4 saltos (rota VALID via 0F:EC/DD:4C). TX = 2 dBm. Casa/corredor com paredes.
- Origem manda DATA 1 pkt/s (payload 32 B) para D61C; RTT medido na origem.

PDR principal = ENTREGA (delivered distinto do destino D61C / enviados na origem,
padrao MANET, justo p/ os dois algos). ACK ida-e-volta = secundario (confiabilidade
bidirecional; flood penalizado pois ACK volta por flood lossy).

### aodv-en — 30 reps x 60 s (rep-seconds)

| Metrica | Media | IC95 | desvio |
|---|---|---|---|
| **PDR entrega (%)** | **94.13** | 1.41 | 3.95 |
| PDR ACK ida-e-volta (%) | 91.74 | 4.16 | 11.63 |
| Latencia one-way (ms) | 45.87 | 3.50 | 9.78 |
| NRL (control/entregue) | 18.05 | 4.69 | 13.10 |
| Energia (J) | 39.35 | 0.64 | 1.79 |

- PDR de entrega tem menos ruido (IC95 1.41) que o ACK (IC95 4.16): a rota AODV
  entrega de forma estavel; a variancia do ACK vem do caminho de volta.
- Print da rede escolhido p/ TCC: rep09 (lat 35.7 ms) — alternativa rep28 (rota 4 saltos).
- Arquivo bruto: `results/campaign-C1-aodv-en.json`.

### flooding — 30 reps x 60 s (EM ANDAMENTO)

- Esperado: entrega ~10% (broadcast storm + sem ARQ + links fracos a 2 dBm;
  ttl_drop=0 -> perda e de link, nao de TTL). Contraste com aodv ~94% mostra que
  o flooding nao sobrevive ao multi-hop sem retransmissao/rota.

## C2 Arvore / C3 Mesh / C4 Falha — PENDENTES
