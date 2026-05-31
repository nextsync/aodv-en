# AODV-EN vs Flooding — comparacao (2026-05-31)

> Dados REAIS de hardware (3 ESP32) + simulacao. Numeros do ledger
> `.claude/autopilot/experiments.json` via `experiment compare` (data-driven, nao
> de memoria). Energia = ESTIMATIVA datasheet ESP32-WROOM-32 (rotulada, nao medida).

## 1. Hardware — C1 reduzido (3 nos, hub ~1 hop, MEDIA de 5 seeds, 60 s, instrumentado)

Setup identico p/ os dois: payload 32 B, 1 pkt/s, 2 origens (N2,N3) -> 1 destino (N1).
Flooding = unicast-por-vizinho (TCC 4.6.1d), TTL=5, dedup=100. AODV-EN = HELLO 2 s.
Metricas reais: PDR=acks/data_sent (origem); latencia=RTT/2 medido na origem (mesmo
clock); NRL=control_tx/entregues (rede); energia=Sigma(tx*Etx+rx*Erx+idle).
Numeros = media do ledger experiments.json (5 seeds/algo) via experiment compare.
Seeds 4 e 5 coletadas com boot fresco (reflash + hard-reset) — ver gotcha: contadores
cumulativos exigem captura logo apos boot, senao energia/NRL inflam.

| Metrica | AODV-EN | Flooding | delta (flood-aodv) |
|---|---|---|---|
| PDR (%) | 99.05 | 99.67 | +0.62 |
| Latencia one-way (ms) | 60.0 | 70.1 | +10.1 |
| NRL (controle/dados) | 0.774 | 0.0 | -0.774 |
| Energia (J, estimada) | 10.86 | 11.00 | +0.15 |

Contadores de rede (3 nos, media 5 seeds): AODV tx=467 rx=573 control=140 entregues=181 ;
Flooding tx=512 rx=905 control=0 entregues=167.

Figuras (regenerar c/ 5 seeds via `plot_tcc_figures.py`): docs/img/tcc/fig-hw-{metrics,channel}.png.
JSONs: results/m10-{aodv,flood}[-s2..s5]-metrics.json. Logs crus: results/m10-{aodv,flood}[-s*]-N{1,2,3}.log.
NOTA: o relatorio docs/tcc-trabalho-completo.md ainda esta em 3 seeds; migrar para 5 (prosa+figuras+PDF) e tarefa a parte.

### Leitura (regime hub/1-hop)
- **PDR**: ambos altos (AODV 99.0%, flooding 99.7%); diferenca dentro da variacao. As
  perdas do AODV concentram-se na descoberta inicial de rota (1o pacote), mitigada pela
  fila pendente.
- **Latencia**: AODV menor na media (60 vs 70 ms). O flooding tem MAIOR dispersao entre
  seeds (50–100 ms): em parte das execucoes o ACK volta por 2 saltos, elevando o RTT.
  (Valores quantizados pelo loop de 100 ms da app.)
- **NRL**: AODV 0.77 (HELLO+RREQ+RREP) vs flooding 0 (sem controle de roteamento). Pela
  definicao do TCC (controle/dados), o flooding tem overhead de CONTROLE nulo -- mas seu
  custo aparece em outro lugar (rx).
- **Energia / ocupacao de canal**: energia quase empatada (~1%); o contraste real esta no
  canal: **rx 905 vs 573** (1.6x): o unicast-para-cada-vizinho faz todos receberem cada
  copia. No hub de 3 nos isso e barato; em rede maior/densa multiplica (ver sim).

## 2. Simulacao — escala (grid 4-25 nos), complementar

`bash sim/run_sim.sh compare` (compare_sim.c). Entrega 100% ambos ate o alcance do TTL.
tx/entrega cruza ~9-11 nos: flooding competitivo em rede pequena, AODV-EN escala melhor.
Com TTL=5 (TCC), o flooding NAO alcanca grids de diametro >5 (4x4/5x5 entregam 0) --
limitacao real do TTL; AODV (rotas) ainda entrega. Grafico: results/charts/sim-tx-per-delivered.png.

## 3. Conclusao (com os dados atuais)
No **C1 reduzido (hub, 1 hop)** o flooding empata/ganha em PDR e latencia e tem NRL de
controle nulo, ao custo de muito mais recepcoes (rx 2.35x) e leve aumento de energia.
A vantagem do AODV-EN aparece em **escala/diametro** (sim): confina o trafego a rota,
enquanto o flooding multiplica copias e esbarra no TTL. 

## Caveats
- 1 seed, 1 cenario (C1-3n), hub 1-hop (HW so tem 3 boards). Para o TCC: repetir ~30x
  (alvo perpetuo do autopilot) e rodar C2/C3/C4 + 5-10 nos (sim) p/ media/desvio/IC95.
- Latencia RTT quantizada pelo loop de 100 ms da app (granularidade grosseira).
- Energia e estimativa de datasheet (V=3.3, I_tx=240mA, I_rx=100mA, I_idle=20mA,
  t_pkt=1ms); para fiel, medir com INA219/shunt.
- 2 hops reais exigem separar fisicamente os nos (fora do hub).
