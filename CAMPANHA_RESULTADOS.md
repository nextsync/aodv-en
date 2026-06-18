# Resultados da campanha HW (10 ESP32 reais)

Dados reais coletados via telemetria in-band (STATSREP/NEIGHREP), metricas por
`tcc_metrics.py` (delta-on-host), agregacao media/desvio/IC95 sobre as repeticoes.
Energia = estimativa de datasheet (rotulada, nao medida). Per-rep logs e prints
ficam em `results/` (gitignored); aqui so o agregado (dado da tese).

## C1 Linear (cadeia multi-hop) — 10 nos

- Topologia: 10 nos (EB80 origem+coletor + 9 reporters). Destino D61C alcancado
  em 3-4 saltos (rota VALID via 0F:EC/DD:4C). TX = 2 dBm. Casa/corredor com paredes.
- Origem manda DATA 1 pkt/s (payload 32 B) para D61C; RTT medido na origem.

### aodv-en — 30 reps x 60 s (rep-seconds)

| Metrica | Media | IC95 | desvio |
|---|---|---|---|
| PDR (%) | 91.74 | 4.16 | 11.63 |
| Latencia one-way (ms) | 45.87 | 3.50 | 9.78 |
| NRL (control/entregue) | 18.05 | 4.69 | 13.10 |
| Energia (J) | 39.35 | 0.64 | 1.79 |

- Variancia alta e real (reps de 43% a 100% de PDR) = oscilacao de rota no
  multi-hop a 2 dBm; IC95 sobre 30 reps captura.
- Print da rede escolhido p/ TCC: rep09 (PDR 100%, lat 35.7 ms, D61C deliv=927)
  — alternativa rep28 (mostra rota de 4 saltos). `results/campaign-prints/`.
- Arquivo bruto: `results/campaign-C1-aodv-en.json`.

### flooding — PENDENTE (reflash dos 10 p/ app_flood, mesma topologia)

## C2 Arvore / C3 Mesh / C4 Falha — PENDENTES
