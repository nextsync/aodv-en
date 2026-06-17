# Campanha Comparativa AODV-EN vs Flooding

- Gerado em: `2026-04-26T14:35:22.066918+00:00`
- Total de execucoes: `128`
- Melhor perfil AODV para metrica hibrida: `hop_only`

## Perfis AODV (media global)

| Perfil | PDR | Latencia (ms) | NRL | Energia (mJ) |
|---|---:|---:|---:|---:|
| hop_only | 0.4244 | 216.040 | 21.6819 | 1543.05 |
| hybrid_default | 0.4044 | 232.889 | 22.7952 | 1729.72 |
| hybrid_rssi_bias | 0.3972 | 197.632 | 25.0797 | 1721.96 |

## Comparativo por Cenario (melhor AODV vs Flooding)

| Cenario | AODV PDR | Flooding PDR | AODV Lat(ms) | Flooding Lat(ms) | AODV NRL | Flooding NRL |
|---|---:|---:|---:|---:|---:|---:|
| linear_stable | 0.2050 | 0.1487 | 399.998 | 1.000 | 40.1314 | 32.9460 |
| partial_mesh_failure | 0.4138 | 0.9237 | 144.408 | 1.000 | 20.0229 | 10.2395 |
| partial_mesh_stable | 0.4113 | 0.9263 | 201.343 | 1.000 | 20.2530 | 10.2702 |
| tree_stable | 0.6675 | 0.5012 | 118.411 | 1.000 | 6.3204 | 14.1862 |

## Delta medio (AODV - Flooding)

- `PDR`: `-0.2006`
- `Latencia ms`: `+215.040`
- `NRL`: `+4.7714`
- `Energia mJ`: `-1059.60`

## Leituras para o Plano C/D/E/F

- `Plano C`: perfil hibrido calibrado por ranking global de PDR/latencia/NRL/energia.
- `Plano D`: baseline flooding implementado e comparado no mesmo conjunto de cenarios e seeds.
- `Plano E`: campanha multi-cenario com repeticoes automatizada e reproduzivel.
- `Plano F`: resultado em formato tabela + deltas prontos para inserir em capitulo de resultados/discussao.

