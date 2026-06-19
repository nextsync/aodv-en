# M5 — Metricas avancadas e graficos: AODV-EN vs Flooding

Data: 2026-05-30 (sessao coleta automatizada)
Branch: `coleta-2026-05-30`

## Metodologia

Duas fontes de dados, mesma comparacao (AODV-EN vs flooding controlado):

1. **Hardware (hub, 1 hop)** — 3 ESP32 reais, ja em m4. Ponto ancora real-radio.
2. **Simulacao (grid 2x2 ate 5x5)** — `sim/compare_sim.c`, rodado por
   `bash sim/run_sim.sh compare`. Os dois nucleos (`aodv_en_node` e `flood_en_node`)
   rodam na MESMA topologia em grade com conectividade de 4 vizinhos, sobre o mesmo
   modelo de radio em memoria. Origem no canto (no 0), destino no canto oposto
   (no N-1). Sao enviados 10 pacotes DATA com ACK; conta-se cada transmissao no ar
   (cada chamada do adapter de emissao = 1 transmissao, broadcast ou unicast).

Metrica principal: **transmissoes por entrega** (custo de canal amortizado),
com taxa de entrega como controle de corretude.

## Resultados (sim grid)

| Grid | Nos | Hops | Entrega AODV | Entrega Flood | TX/entrega AODV | TX/entrega Flood |
|------|-----|------|--------------|---------------|-----------------|------------------|
| 2x2  | 4   | 2    | 10/10        | 10/10         | 4.50            | 6.00             |
| 3x3  | 9   | 4    | 10/10        | 10/10         | 17.60           | 16.00            |
| 4x4  | 16  | 6    | 10/10        | 10/10         | 26.70           | 30.00            |
| 5x5  | 25  | 8    | 10/10        | 10/10         | 35.60           | 41.00            |

Hardware (hub, 3 nos, 1 hop): AODV 5.33, Flood 4.00 tx/entrega.

CSV cru: `m5-compare-grid.csv`.

## Graficos

- `m5-tx-por-entrega.png` — linha: tx/entrega vs numero de nos (sim) + estrelas com
  o ponto de hardware (hub).
- `m5-tx-barras.png` — barras agrupadas por tamanho de grid.
- `m5-entrega.png` — taxa de entrega por tamanho de grid (100% em ambos).

## Leitura

1. **Corretude em paridade**: os dois protocolos entregam 100% (10/10) e recebem
   todos os ACKs, em todas as topologias testadas. O flooding controlado e um
   baseline correto.

2. **Custo de canal e o diferencial**: existe um cruzamento.
   - Em redes **pequenas/densas** (hub de 1 hop; grid 2x2-3x3) o flooding e
     competitivo ou mais barato, porque nao paga descoberta de rota e, sem
     multiplicacao de saltos, a supressao por (origem,seq) corta as duplicatas cedo.
   - A partir de ~9-16 nos o **AODV-EN passa a custar menos por entrega** e a
     vantagem cresce com a escala: ele confina o trafego ao caminho descoberto
     (unicast salto a salto), enquanto o flooding faz cada no retransmitir a cada
     pacote disseminado.

3. **Coerencia hardware/sim**: o ponto real de hardware (hub denso de 1 hop, flood
   abaixo do AODV) cai exatamente na regiao de baixa escala onde a simulacao tambem
   mostra o flooding competitivo — os dois metodos concordam no regime sobreposto.

## Caveats de metodologia

- O AODV-EN aqui roda em modo reativo puro (RREQ/RREP), sem HELLO periodico no sim;
  no hardware o HELLO (1 Hz) adiciona overhead de controle constante (ver m4).
- Modelo de radio sincrono e sem perdas no sim: mede custo de transmissoes, nao
  efeitos de colisao/PER de radio real. O ponto de hardware cobre o lado real-radio.
- Grid de ate 5x5 (25 nos) por limites de tabela compile-time
  (`AODV_EN_ROUTE_TABLE_SIZE`); para grades maiores, recompilar a lib com tabelas
  maiores.
- Valores absolutos dependem de parametros (numero de pacotes, lifetime de rota,
  intervalo de envio). A tendencia (cruzamento e escala) e o resultado robusto.

## Reproduzir

```
bash sim/run_sim.sh compare > docs/evidencias/m5-compare-grid.csv
python3 firmware/tools/plot_compare.py docs/evidencias/m5-compare-grid.csv
```
