# aodv-en

Projeto de Trabalho de Conclusao de Curso do Bacharelado em Engenharia de Software do IFG Campus Inhumas.

Este repositorio concentra a pesquisa, especificacao e implementacao do `AODV-EN`, uma adaptacao do AODV ([RFC 3561](https://datatracker.ietf.org/doc/html/rfc3561)) para redes mesh multi-hop sobre ESP-NOW v2 e ESP32.

## Documentos base

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

## Status atual

Em uma frase: `AODV-EN v1` esta funcionalmente fechado. O caminho critico restante e fechar `TC-002`/`TC-003`/`TC-004` em hardware com captura completa, implementar baseline de flooding, rodar os experimentos e escrever o TCC. Detalhes em [docs/plano-desenvolvimento-completo.md](docs/plano-desenvolvimento-completo.md).
