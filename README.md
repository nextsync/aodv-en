# aodv-en

Projeto de Trabalho de Conclusao de Curso do Bacharelado em Engenharia de Software do IFG Campus Inhumas.

Este repositorio concentra a pesquisa, especificacao e implementacao do `AODV-EN`, uma adaptacao do AODV ([RFC 3561](https://datatracker.ietf.org/doc/html/rfc3561)) para redes mesh multi-hop sobre ESP-NOW v2 e ESP32.

> **TCC concluido.** A monografia completa (94 paginas) esta em
> [tcc_latex/Template_Pacheco_TCC.pdf](tcc_latex/Template_Pacheco_TCC.pdf).
> Autores: Huakson Lima e Diogo dos Reis Almeida (IFG Campus Inhumas).

## Resultados (avaliacao experimental)

O AODV-EN foi avaliado em **hardware real** (ESP32-WROOM-32, ESP-NOW v2, ESP-IDF v6.0.0)
contra um baseline de *flooding* controlado (broadcast + TTL + supressao de duplicatas),
em tres cenarios, 30 repeticoes cada:

| Cenario | Topologia | PDR de entrega (AODV-EN vs flooding) | Achado |
|---|---|---|---|
| **C1** | cadeia, 10 nos, 3-4 saltos, 2 dBm | **94,1% vs 13,9%** | o roteamento domina o regime multi-hop |
| **C3** | malha densa, 10 nos, 5 saltos, 2 dBm | **91,4% vs 77,8%** | vantagem **dependente da topologia**: na malha o flooding recupera via redundancia |
| **C4** | falha induzida, 6 nos, 4 dBm | **70,6%** sob queda ciclica de rele | **auto-recuperacao** (*self-healing*) por rota alternativa |

Em custo por pacote entregue (razao das medias), no C1 o AODV-EN gasta **0,69 J/pacote**
contra **3,75 J/pacote** do flooding (~5x menos energia por entrega util) e cerca de 20%
menos transmissoes por entrega. Metodologia, tabelas, figuras e testes estatisticos
(Welch, Cohen, Mann-Whitney) no Capitulo 5 da monografia.

### Reproduzir os resultados

- dados brutos por repeticao: `results/campaign-{C1,C3,C4}-{aodv-en,flooding}.json`
- metricas (delta no host): [firmware/tools/tcc_metrics.py](firmware/tools/tcc_metrics.py)
- coleta da campanha: [firmware/tools/campaign.py](firmware/tools/campaign.py)
- figuras da tese (deterministas dos JSONs): [plot_c1_figures.py](firmware/tools/plot_c1_figures.py), [plot_c3_figures.py](firmware/tools/plot_c3_figures.py), [plot_c4_extra.py](firmware/tools/plot_c4_extra.py), [plot_c4_selfheal.py](firmware/tools/plot_c4_selfheal.py)
- *toolchain*: ESP-IDF v6.0.0 sobre ESP-NOW v2, em modulos ESP32-WROOM-32

## Documentos base

- [docs/aodv-base-invariantes.md](docs/aodv-base-invariantes.md)
- [docs/aodv-en-spec-v0.md](docs/aodv-en-spec-v0.md)
- [docs/aodv-en-estruturas-dados.md](docs/aodv-en-estruturas-dados.md)
- [docs/plano-desenvolvimento-completo.md](docs/plano-desenvolvimento-completo.md)
- [docs/fluxos-tcc-aodv-en.md](docs/fluxos-tcc-aodv-en.md)
- [docs/aodv-en-completo.pdf](docs/aodv-en-completo.pdf) - **toda a documentacao em um PDF unico** (~148 paginas, ordem top-down). Regerar com `python3 docs/build_pdf.py`.
- [docs/aodv-en-spec-v1.md](docs/aodv-en-spec-v1.md) - especificacao funcional fechada (`v1`, ativa)
- [docs/aodv-en-funcionamento.md](docs/aodv-en-funcionamento.md) - guia didatico do funcionamento, com layout de bytes, traces e worked examples
- [docs/aodv-en-mapa-do-codigo.md](docs/aodv-en-mapa-do-codigo.md) - guia de estudo do codigo: onde cada feature esta implementada, mapa por arquivo
- [docs/runbook-bancada.md](docs/runbook-bancada.md) - passo a passo de bancada: identificar ESPs, build, flash, monitor, analise (cole-e-execute)
- [docs/aodv-base-invariantes.md](docs/aodv-base-invariantes.md) - invariantes do AODV que o projeto preserva
- [docs/aodv-en-estruturas-dados.md](docs/aodv-en-estruturas-dados.md) - layout das tabelas, mensagens e configuracao
- [docs/plano-desenvolvimento-completo.md](docs/plano-desenvolvimento-completo.md) - roadmap de fases e backlog
- [docs/aodv-en-spec-v0.md](docs/aodv-en-spec-v0.md) - `OBSOLETA`, mantida apenas para registro historico

## Features

### Integradas na v1

- [docs/features/enfilaremento-dos-dados.md](docs/features/enfilaremento-dos-dados.md) - fila de `DATA` pendente durante descoberta
- [docs/features/precursores.md](docs/features/precursores.md) - precursores e `RERR` direcionado (RFC 3561 secao 6.2)

### Planejadas para v2

- [docs/features/articulation-point-planejado.md](docs/features/articulation-point-planejado.md) - detecao de no de corte

## Casos de teste

- [docs/tests](docs/tests) - casos de teste de bancada com mapeamento direto para os criterios da spec v1

| Caso | Topologia | Status |
|---|---|---|
| `TC-001` | 2 nos diretos | `ATIVO` |
| `TC-002` | cadeia de 3 (`A <-> B <-> C`) | `ATIVO` (`PASS` 2026-04-21) |
| `TC-003` | 3 nos com falha intermediaria | `ATIVO` |
| `TC-004` | 3 nos sob ciclos por 30 min | `ATIVO` |
| `TC-005` | cadeia de 4 (`A <-> B <-> C <-> D`) | `ATIVO` |

## Biblioteca AODV-EN

A camada de roteamento e implementada como componente ESP-IDF reutilizavel em [firmware/components/aodv_en](firmware/components/aodv_en). Ela expoe um adapter de transporte injetavel, o que permite rodar o mesmo nucleo na simulacao em C e no firmware ESP32.

### Headers principais

- [firmware/components/aodv_en/include/aodv_en.h](firmware/components/aodv_en/include/aodv_en.h) - API `aodv_en_stack_*` consumida pela app
- [firmware/components/aodv_en/include/aodv_en_node.h](firmware/components/aodv_en/include/aodv_en_node.h) - nucleo do no (uso interno)
- [firmware/components/aodv_en/include/aodv_en_messages.h](firmware/components/aodv_en/include/aodv_en_messages.h) - layout das mensagens no fio
- [firmware/components/aodv_en/include/aodv_en_types.h](firmware/components/aodv_en/include/aodv_en_types.h) - tipos centrais e configuracao
- [firmware/components/aodv_en/include/aodv_en_limits.h](firmware/components/aodv_en/include/aodv_en_limits.h) - limites e timers default
- [firmware/components/aodv_en/include/aodv_en_status.h](firmware/components/aodv_en/include/aodv_en_status.h) - codigos de status

### Modulos

- vizinhos: [aodv_en_neighbors.h](firmware/components/aodv_en/include/aodv_en_neighbors.h) / [.c](firmware/components/aodv_en/src/aodv_en_neighbors.c)
- rotas (com precursores): [aodv_en_routes.h](firmware/components/aodv_en/include/aodv_en_routes.h) / [.c](firmware/components/aodv_en/src/aodv_en_routes.c)
- cache de `RREQ`: [aodv_en_rreq_cache.h](firmware/components/aodv_en/include/aodv_en_rreq_cache.h) / [.c](firmware/components/aodv_en/src/aodv_en_rreq_cache.c)
- cache de peers: [aodv_en_peers.h](firmware/components/aodv_en/include/aodv_en_peers.h) / [.c](firmware/components/aodv_en/src/aodv_en_peers.c)
- nucleo do no: [aodv_en_node.h](firmware/components/aodv_en/include/aodv_en_node.h) / [.c](firmware/components/aodv_en/src/aodv_en_node.c)

## Firmware ESP32

App ESP-IDF de bancada em [firmware/main](firmware/main). Suporta dois modos selecionaveis por Kconfig:

- `app_demo`: legado, envia `HELLO` e `DATA` periodicos
- `app_proto_example`: protocolo de aplicacao com `HEALTH/TEXT/CMD` e CLI serial

### Pontos de entrada

- [firmware/README.md](firmware/README.md)
- [firmware/main/main.c](firmware/main/main.c)
- [firmware/main/app_proto_example.c](firmware/main/app_proto_example.c)
- [firmware/main/Kconfig.projbuild](firmware/main/Kconfig.projbuild)

### Scripts

- [firmware/build.sh](firmware/build.sh)
- [firmware/flash_monitor.sh](firmware/flash_monitor.sh)
- [firmware/monitor_log.sh](firmware/monitor_log.sh) - captura serial em `firmware/logs/serial/`
- [firmware/idf-env.sh](firmware/idf-env.sh) - bootstrap do ESP-IDF

### Perfis por papel

- [firmware/tests/tc001](firmware/tests/tc001) - `TC-001`
- [firmware/tests/tc002](firmware/tests/tc002) - `TC-002`, `TC-003`, `TC-004`
- [firmware/tests/tc005](firmware/tests/tc005) - `TC-005` (cenario de 4 nos)

### Ferramentas de analise

- [firmware/tools/draw_topology.py](firmware/tools/draw_topology.py) - gera topologia (Mermaid, Graphviz DOT, SVG) a partir dos logs

### Dashboard ao vivo

- [firmware/tools/live_monitor.py](firmware/tools/live_monitor.py) - levanta um dashboard web (`http://localhost:8765/`) com topologia animada, metricas em tempo real e timeline de eventos. Funciona com hardware real (varias portas seriais em paralelo) ou em `--demo` sem hardware. Detalhes em [firmware/README.md](firmware/README.md#dashboard-ao-vivo-real-time).

## Simulacao local

Simulacao em C que valida o fluxo `RREQ -> RREP -> DATA -> ACK` usando o mesmo nucleo do firmware, com adapter mock.

- [sim/aodv_en_sim.c](sim/aodv_en_sim.c) - cenario base (3 nos em linha)
- [sim/aodv_en_sim_100.c](sim/aodv_en_sim_100.c) - cenario com mais nos
- [sim/aodv_en_sim_1000.c](sim/aodv_en_sim_1000.c) - cenario de stress
- [sim/aodv_en_sim_large.c](sim/aodv_en_sim_large.c) - cenario alternativo
- [sim/run_sim.sh](sim/run_sim.sh)
- [sim/README.md](sim/README.md)

## Baseline de flooding e comparacao

Baseline comparativo ao AODV-EN: flooding controlado (broadcast com TTL + supressao de duplicatas por `(origem, sequencia)`). E um **componente ESP-IDF independente** (`flood_en`), com wire format, tipos e API proprios — nao depende do componente `aodv_en`.

- nucleo (componente proprio): [firmware/components/flood_en/include/flood_en.h](firmware/components/flood_en/include/flood_en.h) / [src/flood_en.c](firmware/components/flood_en/src/flood_en.c)
- app de bancada: [firmware/main/app_flood.c](firmware/main/app_flood.c) (modo Kconfig `AODV_EN_APP_USE_APP_FLOOD`)
- sim baseline: `bash sim/run_sim.sh flood`
- sweep comparativo (grid 2x2..5x5, CSV): `bash sim/run_sim.sh compare`
- graficos: [firmware/tools/plot_compare.py](firmware/tools/plot_compare.py)
- evidencias e analise: [docs/evidencias](docs/evidencias) (validacao do dashboard, flooding em hardware, AODV-EN vs flooding, metricas e graficos)

## Status atual

`AODV-EN v1` esta funcionalmente fechado; o baseline de flooding foi implementado, validado
(sim + hardware) e comparado ao AODV-EN; e a campanha experimental em hardware (cenarios C1,
C3 e C4, 30 repeticoes cada) foi concluida e consolidada na monografia
([tcc_latex/Template_Pacheco_TCC.pdf](tcc_latex/Template_Pacheco_TCC.pdf)). Como trabalho
futuro declarado: medicao fisica de energia (INA219), o cenario C2 (arvore), o flooding sob
falha (C4) e a avaliacao isolada da metrica hibrida e da politica LRU de peers. Roadmap e
historico em [docs/plano-desenvolvimento-completo.md](docs/plano-desenvolvimento-completo.md).
