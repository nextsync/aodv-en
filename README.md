# aodv-en

Projeto de Trabalho de Conclusao de Curso do Bacharelado em Engenharia de Software do IFG Campus Inhumas.

Este repositorio concentra a pesquisa, especificacao e implementacao do `AODV-EN`, uma adaptacao do AODV para redes mesh multi-hop sobre ESP-NOW e ESP32.

## Documentos base

- [docs/aodv-base-invariantes.md](docs/aodv-base-invariantes.md)
- [docs/aodv-en-spec-v0.md](docs/aodv-en-spec-v0.md)
- [docs/aodv-en-estruturas-dados.md](docs/aodv-en-estruturas-dados.md)
- [docs/plano-desenvolvimento-completo.md](docs/plano-desenvolvimento-completo.md)

## Features implementadas

- [docs/features/enfilaremento-dos-dados.md](docs/features/enfilaremento-dos-dados.md)

## Features planejadas (nao implementadas)

- [docs/features/articulation-point-planejado.md](docs/features/articulation-point-planejado.md)

## Base de firmware

Os primeiros tipos do protocolo foram materializados em:

- [firmware/components/aodv_en/include/aodv_en_limits.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_limits.h)
- [firmware/components/aodv_en/include/aodv_en_types.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_types.h)
- [firmware/components/aodv_en/include/aodv_en_messages.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_messages.h)
- [firmware/components/aodv_en/include/aodv_en_tables.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_tables.h)

As operacoes iniciais das tabelas e utilitarios do componente foram iniciadas em:

- [firmware/components/aodv_en/include/aodv_en_status.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_status.h)
- [firmware/components/aodv_en/include/aodv_en_mac.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_mac.h)
- [firmware/components/aodv_en/include/aodv_en_neighbors.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_neighbors.h)
- [firmware/components/aodv_en/include/aodv_en_routes.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_routes.h)
- [firmware/components/aodv_en/include/aodv_en_rreq_cache.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_rreq_cache.h)
- [firmware/components/aodv_en/include/aodv_en_peers.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_peers.h)

O nucleo inicial do no, desacoplado do driver de radio, esta em:

- [firmware/components/aodv_en/include/aodv_en_node.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_node.h)
- [firmware/components/aodv_en/src/aodv_en_node.c](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/src/aodv_en_node.c)

## Simulacao local

Existe uma simulacao simples do fluxo do protocolo em:

- [sim/aodv_en_sim.c](/Users/huaksonlima/Documents/tcc/aodv-en/sim/aodv_en_sim.c)
- [sim/run_sim.sh](/Users/huaksonlima/Documents/tcc/aodv-en/sim/run_sim.sh)
- [sim/README.md](/Users/huaksonlima/Documents/tcc/aodv-en/sim/README.md)

## Firmware ESP32

O primeiro app real em ESP-IDF esta em:

- [firmware/README.md](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/README.md)
- [firmware/main/main.c](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/main/main.c)
- [firmware/main/Kconfig.projbuild](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/main/Kconfig.projbuild)
- [firmware/build.sh](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/build.sh)
- [firmware/flash_monitor.sh](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/flash_monitor.sh)
