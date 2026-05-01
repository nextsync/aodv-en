# Simulacao Local do AODV-EN

Esta pasta contem simulacoes locais do nucleo do protocolo, sem ESP32 real.

## Objetivo

Validar localmente o fluxo:

- `RREQ`
- `RREP`
- `DATA`
- `ACK`
- `RERR`

reusando o mesmo nucleo do firmware via mock do adapter de transporte.

## Variantes disponiveis

| Variante | Arquivo | Topologia | Foco |
|---|---|---|---|
| `basic` (padrao) | [aodv_en_sim.c](aodv_en_sim.c) | 3 nos `A-B-C` | descoberta, retry de `ACK`, late-join |
| `large` | [aodv_en_sim_large.c](aodv_en_sim_large.c) | 6 nos `A-B-C-D-E-F` | `RERR` propagando na cadeia + reconvergencia |
| `100` | [aodv_en_sim_100.c](aodv_en_sim_100.c) | grade 10x10 | parede central, descoberta em escala |
| `1000` | [aodv_en_sim_1000.c](aodv_en_sim_1000.c) | grade 32x32 | smart city com falhas aleatorias (lento) |

## Como rodar

```bash
bash sim/run_sim.sh           # basic (padrao)
bash sim/run_sim.sh basic
bash sim/run_sim.sh large
bash sim/run_sim.sh 100
bash sim/run_sim.sh 1000
bash sim/run_sim.sh help
```

## O que esperar do `basic`

- na primeira tentativa de envio, `A` nao tem rota para `C`
- `A` emite `RREQ`
- `B` encaminha
- `C` responde com `RREP`
- a rota `A -> C via B` e instalada
- na segunda tentativa, `DATA` chega em `C`
- `C` responde com `ACK`
- `A` registra a confirmacao
- depois exercita perda de `ACK` (com retry) e late-join (descoberta apos retorno do enlace)

## Observacao sobre escala

Os simuladores `100` e `1000` setam `config.route_table_size` e `config.neighbor_table_size` na inicializacao, mas esses campos nao redimensionam os arrays em runtime. Os tamanhos reais sao constantes de compilacao em [firmware/components/aodv_en/include/aodv_en_limits.h](../firmware/components/aodv_en/include/aodv_en_limits.h):

- `AODV_EN_ROUTE_TABLE_SIZE` (default `32`)
- `AODV_EN_NEIGHBOR_TABLE_SIZE` (default `16`)

Para experimentos com tabelas maiores, recompile passando `-DAODV_EN_ROUTE_TABLE_SIZE=N` (e similares) na linha de compilacao. Caso contrario, parte das rotas/vizinhos sera descartada com `AODV_EN_ERR_FULL`.
