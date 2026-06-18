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

### flooding — 30 reps x 60 s

| Metrica | Media | IC95 | desvio |
|---|---|---|---|
| PDR entrega (%) | 12.68 | 3.37 | 9.41 |
| PDR ACK ida-e-volta (%) | 3.75 | 0.84 | 2.35 |
| Latencia one-way (ms) | 31.65 | 1.68 | 4.63 |
| NRL (controle/entregue) | 0.0 | 0.0 | 0.0 |
| Energia (J) | 28.41 | 0.03 | 0.09 |

- Arquivo bruto: `results/campaign-C1-flooding.json`. ttl_drop=0 -> perda e de
  LINK (storm + sem ARQ a 2 dBm), nao de TTL. So 7 dos 10 nos reportaram
  (telemetria flood lossy) -> overhead real ainda maior que o medido.

## C1 — comparacao aodv-en x flooding (10 nos, 30 reps cada)

| Metrica | aodv-en | flooding | Leitura |
|---|---|---|---|
| **PDR entrega (%)** | **94.13 ±1.41** | **12.68 ±3.37** | AODV entrega ~7.4x mais |
| PDR ACK ida-e-volta (%) | 91.74 ±4.16 | 3.75 ±0.84 | AODV da confirmacao bidirecional confiavel |
| **Overhead (tx/entregue)** | **54.2 ±10.1** | **102.9 ±36.2** | flood ~2x mais caro por pacote (subestimado: 7 nos) |
| Latencia one-way (ms) | 45.87 ±3.50 | 31.65 ±1.68 | flood menor = VIES DE SOBREVIVENCIA (so chegam caminhos curtos) |
| NRL (controle/entregue) | 18.05 ±4.69 | 0.0 | flood sem controle de rota; seu custo esta nos rebroadcasts (ver tx/entregue) |
| Energia estimada (J) | 39.35 ±0.64 | 28.41 ±0.03 | estimada; nos desiguais (9 vs 7), interpretar com cautela |

**Conclusao C1:** AODV-EN domina a entrega multi-hop (94% vs 13%) com overhead ~2x
menor por pacote entregue. O flooding colapsa sem retransmissao/rota a 2 dBm: a
maioria dos pacotes nao sobrevive aos 3-4 saltos. A menor latencia do flood e vies
de sobrevivencia (so os poucos pacotes de caminho curto chegam). NRL classico (so
controle de rota) nao captura o overhead do flood -> usar tx/entregue p/ comparacao justa.

## C2 Arvore / C3 Mesh / C4 Falha — PENDENTES

## C4 Falha / self-healing (HW, 6 nos, apartamento)

- Topologia: 6 ESP32 no apartamento, TX 4 dBm (QDBM=16). Origem+coletor EB80 ->
  destino D61C (N6) em 2 saltos via rele on-path 94:D4 (alternativa B9:EC).
- Falha induzida: o rele 94:D4 silencia o radio 15 s a cada 45 s (33% de
  indisponibilidade), simulando queda e retorno do no. Vizinhos detectam a quebra
  (HELLO/RERR) e o AODV re-rota via B9:EC.
- DIFERENTE do C1 (10 nos, cadeia): cenario menor, com redundancia e falha, focado
  em recuperacao (nao em escala). TX 4 dBm (vs 2 dBm do C1) pelo espaco menor com
  paredes do apartamento -- declarado por cenario.

### aodv-en -- 30 reps x 60 s

| Metrica | Media | IC95 | desvio |
|---|---|---|---|
| PDR de entrega (%) | 62,8 | 11,5 | 32,1 |
| PDR ACK ida-e-volta (%) | 47,6 | 11,5 | 32,1 |
| Latencia one-way (ms) | 60,7 | 14,6 | 40,0 |
| NRL | 5,16 | 1,7 | 4,77 |
| Energia (J, est.) | 9,24 | 1,7 | 4,68 |

- Contraste com C1 sem falha (94,1%): a falha periodica do rele on-path derruba a
  entrega para ~63% e eleva a latencia (re-descoberta de rota). A alta variancia
  (CV ~51%) reflete a natureza estocastica do instante da falha frente ao trafego.
- Figura fig_c4_selfheal: entregas acumuladas de uma repeticao -- os patamares
  coincidem com as quedas do rele e a retomada evidencia a recuperacao (self-healing).
- Resultado honesto do tradeoff eficiencia x resiliencia: o roteamento reativo paga
  um custo de deteccao+recuperacao na falha (que a inundacao, sem rotas, nao tem).

### flooding C4 -- PENDENTE (reflash app_flood TX 4dBm, vitima 94:D4)
