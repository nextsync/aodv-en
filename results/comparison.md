# AODV-EN vs Flooding — comparacao (2026-05-31)

> Dados REAIS de hardware (3 ESP32) + simulacao. Numeros do ledger
> `.claude/autopilot/experiments.json` via `experiment compare` (data-driven, nao
> de memoria). Energia = ESTIMATIVA datasheet ESP32-WROOM-32 (rotulada, nao medida).

## 1. Hardware — C1 reduzido (3 nos, hub ~1 hop, 1 seed, 60 s, instrumentado)

Setup identico p/ os dois: payload 32 B, 1 pkt/s, 2 origens (N2,N3) -> 1 destino (N1).
Flooding = unicast-por-vizinho (TCC 4.6.1d), TTL=5, dedup=100. AODV-EN = HELLO 2 s.
Metricas reais: PDR=acks/data_sent (origem); latencia=RTT/2 medido na origem (mesmo
clock); NRL=control_tx/entregues (rede); energia=Sigma(tx*Etx+rx*Erx+idle).

| Metrica | AODV-EN | Flooding | delta (flood-aodv) | % |
|---|---|---|---|---|
| PDR (%) | 98.57 | 100.0 | +1.43 | +1.45% |
| Latencia one-way (ms) | 60.0 | 50.0 | -10.0 | -16.67% |
| NRL (controle/dados) | 0.7785 | 0.0 | -0.7785 | -100% |
| Energia (J, estimada) | 12.41 | 12.80 | +0.39 | +3.13% |

Contadores de rede (3 nos): AODV tx=441 rx=562 control=123 entregues=158 ;
Flooding tx=614 rx=1323 control=0 entregues=154.

Grafico: results/charts/m10-compare.png. JSONs: results/m10-{aodv,flood}-metrics.json.
Logs crus: results/m10-{aodv,flood}-N{1,2,3}.log.

### Leitura (regime hub/1-hop)
- **PDR**: flooding 100% vs AODV 98.6% — no hub raso o flooding (todos ouvem) garante
  entrega; AODV perdeu 1 pacote (descoberta/timeout). Ambos altos.
- **Latencia**: flooding um pouco menor (50 vs 60 ms) — sem espera por descoberta de
  rota. (Valores quantizados pelo loop de 100 ms da app; std~0.)
- **NRL**: AODV 0.78 (HELLO+RREQ+RREP) vs flooding 0 (sem controle de roteamento). Pela
  definicao do TCC (controle/dados), o flooding tem overhead de CONTROLE nulo -- mas seu
  custo aparece em outro lugar (rx).
- **Energia / ocupacao de canal**: flooding gasta ~3% mais energia e, sobretudo, gera
  **rx 1323 vs 562** (2.35x): o unicast-para-cada-vizinho faz todos receberem cada copia.
  No hub de 3 nos isso e barato; em rede maior/densa multiplica (ver sim).

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
