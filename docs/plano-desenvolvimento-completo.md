# Plano de Desenvolvimento Completo do AODV-EN

## Estado

- alinhado com [aodv-en-spec-v1.md](aodv-en-spec-v1.md)
- ultima revisao: 2026-05-01

## Objetivo

Este documento organiza o desenvolvimento do `AODV-EN` ate a conclusao do projeto tecnico e do TCC.

Ele serve como:

- roadmap de implementacao
- checklist de validacao
- guia de experimentacao
- trilha de escrita do trabalho final

## Legenda de status

- `CONCLUIDO`
- `EM_ANDAMENTO`
- `PENDENTE`
- `OPCIONAL`

## Estado atual resumido

### Ja concluido

- `CONCLUIDO` invariantes do AODV em [aodv-base-invariantes.md](aodv-base-invariantes.md)
- `CONCLUIDO` `Spec v1` consolidada em [aodv-en-spec-v1.md](aodv-en-spec-v1.md), substituindo a v0
- `CONCLUIDO` modelagem das estruturas em [aodv-en-estruturas-dados.md](aodv-en-estruturas-dados.md)
- `CONCLUIDO` materializacao dos limites, tipos, mensagens e tabelas em `firmware/components/aodv_en/include`
- `CONCLUIDO` operacoes de vizinhos, rotas, cache de `RREQ` e cache de peers em `firmware/components/aodv_en/src`
- `CONCLUIDO` nucleo do no com `HELLO`, `RREQ`, `RREP`, `RERR`, `DATA`, `ACK`
- `CONCLUIDO` simulacao local do fluxo `RREQ -> RREP -> DATA -> ACK` em [sim/aodv_en_sim.c](../sim/aodv_en_sim.c)
- `CONCLUIDO` fila pendente de `DATA` com retry de descoberta e expiracao ([features/enfilaremento-dos-dados.md](features/enfilaremento-dos-dados.md))
- `CONCLUIDO` precursores e `RERR` direcionado ([features/precursores.md](features/precursores.md))
- `CONCLUIDO` lib `AODV-EN` consumivel (adapter `aodv_en_stack_*`) em [aodv_en.h](../firmware/components/aodv_en/include/aodv_en.h)
- `CONCLUIDO` app de bancada `app_proto_example` com CLI e mensagens `HEALTH/TEXT/CMD`
- `CONCLUIDO` perfis pre-prontos `node_a`, `node_b`, `node_c`, `node_d` em `firmware/tests/`
- `CONCLUIDO` casos `TC-001`, `TC-002`, `TC-003`, `TC-004`, `TC-005` documentados em [tests](tests/)
- `CONCLUIDO` `TC-002` validado em hardware em 2026-04-21 (`PASS` com evidencias indiretas)

### Ainda nao concluido

- multi-hop em hardware com captura simultanea dos 3 nos (`TC-002` com captura completa)
- reconvergencia em hardware (`TC-003`)
- soak de 30 minutos em hardware (`TC-004`)
- baseline de flooding e experimentos comparativos
- escrita do TCC

## Roadmap macro

| Fase | Status | Objetivo principal | Entregavel |
|---|---|---|---|
| 0. Fundacao conceitual | `CONCLUIDO` | Definir o que o protocolo e e o que ele nao pode violar | invariantes + spec v1 |
| 1. Base estrutural do firmware | `CONCLUIDO` | Materializar tipos, tabelas e mensagens | headers e modulos |
| 2. Nucleo logico do protocolo | `CONCLUIDO` | Processar mensagens, manter rotas e filas | `aodv_en_node` + sim |
| 3. Endurecimento do nucleo | `CONCLUIDO` | Fechar o nucleo conforme spec v1 | `AODV-EN core v1` |
| 4. Adapter ESP-NOW / ESP-IDF | `EM_ANDAMENTO` | Conectar o nucleo ao transporte real | adapter desacoplado |
| 5. Firmware de teste em ESP32 | `EM_ANDAMENTO` | Rodar o protocolo em hardware real | app + perfis por papel |
| 6. Testes funcionais de rede | `EM_ANDAMENTO` | Validar TC-001 a TC-004 com captura completa | relatorio de testes |
| 7. Baseline de flooding | `PENDENTE` | Criar comparativo justo para o TCC | protocolo baseline |
| 8. Experimentos e metricas | `PENDENTE` | Coletar dados para o trabalho | tabelas, logs, graficos |
| 9. Escrita do TCC | `PENDENTE` | Transformar implementacao e resultados em texto academico | monografia |
| 10. Defesa e demonstracao | `PENDENTE` | Preparar apresentacao final | slides e demo |

## Fase 0. Fundacao conceitual

Status: `CONCLUIDO`

### Entregas

- invariantes do AODV
- spec v1 do `AODV-EN` (substitui a v0 obsoleta)
- definicao das tabelas principais e mensagens
- decisoes explicitas sobre `HELLO`, precursores, fila pendente, metrica e escopo de v2

### Criterio de conclusao

- o projeto tem uma base teorica coerente
- a spec v1 fecha as decisoes funcionais que estavam em aberto

## Fase 1. Base estrutural do firmware

Status: `CONCLUIDO`

### Entregas

- limites e tipos centrais
- formatos de mensagem (`__attribute__((packed))`)
- wrappers das tabelas
- operacoes iniciais de vizinhos, rotas, cache de `RREQ` e peers

## Fase 2. Nucleo logico do protocolo

Status: `CONCLUIDO`

### Entregas

- `aodv_en_node_t` com filas pendentes, callbacks de emissao e entrega
- processamento de `HELLO`, `RREQ`, `RREP`, `RERR`, `DATA`, `ACK`
- precursores e `RERR` direcionado
- simulacao local do fluxo principal e dos cenarios de retry e late-join

## Fase 3. Endurecimento do nucleo

Status: `CONCLUIDO`

### Entregas que fecharam a fase

- fila pendente de `DATA` com retry de descoberta e expiracao
- timeout e retry de `ACK` com fila dedicada
- alinhamento das regras de `sequence number` em `RREP` com a RFC 3561 secao 6.6.1
- semantica de `RERR` direcionada a precursores
- helpers de validacao de header e tamanho de frame
- centralizacao de `TTL` e `hop_count` em `aodv_en_validate_*` e nos handlers
- regra de troca de rota com histerese para evitar flapping
- novos cenarios na simulacao local cobrindo descoberta, retry, late-join e ACK timeout

### Itens reconhecidos como `OPCIONAL` para v1

- separar `rreq_retry_count` de `ack_retry_count` (hoje compartilham orcamento)
- tratamento explicito de wrap-around de `sequence number`
- politica de substituicao de precursor quando o array enche

Esses itens nao bloqueiam v1 e ficam mapeados na trilha de v2 da spec.

## Fase 4. Adapter ESP-NOW / ESP-IDF

Status: `EM_ANDAMENTO`

### Ja feito

- app ESP-IDF buildavel em `firmware/`
- integracao com `esp_wifi_start` e `esp_now_init`
- callback de recepcao conectado ao `aodv_en_stack_on_recv`
- callback de emissao conectado ao `esp_now_send`
- registro de peer broadcast
- processamento de frames fora do callback do Wi-Fi por fila local

### Tarefas restantes

- extrair a camada de transporte do `main`/`app_proto_example` para um adapter dedicado `aodv_en_espnow`
- encapsular `esp_wifi_start`, `esp_now_init`, peers e callbacks no adapter
- integrar `peer_cache` ao driver com politica explicita de adicao/remocao
- padronizar logs estruturados por no
- tratar melhor falhas de `esp_now_send` (mapear para `aodv_en_stack_on_link_tx_result`)
- documentar configuracao de canal e interface

### Entregavel

- adapter `aodv_en_espnow` desacoplado, com a app reduzida a glue code

### Criterio de conclusao

- substituir o adapter por um mock nao exige mudar a app

## Fase 5. Firmware de teste em ESP32

Status: `EM_ANDAMENTO`

### Ja feito

- projeto `ESP-IDF` em `firmware/`
- `app_demo` (legacy) e `app_proto_example` (atual) com CLI
- envio periodico de `HELLO`
- envio opcional de `DATA` para MAC configurado
- logs de estatisticas e rotas
- perfis `NODE_A`, `NODE_B`, `NODE_C`, `NODE_D` prontos
- build validado com `aodv_en_firmware.bin`
- evidencia de funcionamento em hardware via `TC-002`

### Tarefas restantes

- captura serial simultanea dos 3 nos no `TC-002`
- consolidar dumps periodicos (vizinhos, rotas, stats) em formato facil de parsear
- definir convencao de log para alimentar [tools/draw_topology.py](../firmware/tools/draw_topology.py)
- executar `TC-005` em hardware com 4 nos

### Entregavel

- app de bancada com observabilidade boa o suficiente para reproduzir os criterios de aprovacao da spec v1

## Fase 6. Testes funcionais de rede

Status: `EM_ANDAMENTO`

### Cenarios obrigatorios para v1

| Caso | Topologia | Cobre criterios da spec |
|---|---|---|
| `TC-001` | 2 nos (`A <-> B`) | 1 |
| `TC-002` | 3 nos (`A <-> B <-> C`) | 2, 5 |
| `TC-003` | 3 nos com falha intermediaria | 3, 7 |
| `TC-004` | 3 nos sob ciclos por 30 min | 4, 6 |
| `TC-005` | 4 nos (`A <-> B <-> C <-> D`) | 2 (estendido para 3 saltos) |

### Cenarios opcionais

- estresse com fila pendente cheia em simulador
- variantes de simulacao em escala (ver [../sim/README.md](../sim/README.md): `large`, `100`, `1000`)

### O que registrar

- logs por no
- tabelas antes e depois
- latencia aproximada de descoberta e de entrega
- numero de mensagens de controle
- contadores de stats em `aodv_en_stack_stats_t`
- sucesso ou falha da entrega

### Entregavel

- relatorio funcional de bancada com PASS/FAIL por TC e evidencias

### Criterio de conclusao

- os criterios de aprovacao da spec v1 estao demonstrados

## Fase 7. Baseline de flooding

Status: `PENDENTE`

### Tarefas

- implementar baseline simples de flooding usando o mesmo adapter `aodv_en_espnow`
- manter a mesma interface de app (mesmo `send_data`, mesmo callback de entrega)
- coletar as mesmas metricas de `aodv_en_stack_stats_t` (no que se aplica)

### Entregavel

- modulo baseline `flooding` selecionavel por configuracao

### Criterio de conclusao

- existe uma referencia experimental comparavel ao `AODV-EN`

## Fase 8. Experimentos e metricas

Status: `PENDENTE`

### Metricas minimas

- `PDR` (`delivered_frames / DATA enviados pela origem`)
- latencia fim a fim (do envio ao `ACK`)
- overhead de controle (`tx_frames` excluindo `DATA`)
- numero total de transmissoes
- tempo para descoberta de rota
- tempo para recuperacao apos falha

### Cenarios de experimento

- topologia linear (3 e 4 nos)
- topologia em arvore
- malha parcial com rota alternativa
- cenario com falha induzida

### Tarefas

- definir formato de log exportavel
- repeticoes por cenario
- consolidar resultados em tabelas
- gerar graficos
- comparar `AODV-EN` vs `Flooding`

### Observacao

Sem medicao real de energia, tratar consumo energetico como inferencia por numero de transmissoes ou limitacao explicitamente declarada.

### Entregavel

- conjunto de resultados experimentais do TCC

## Fase 9. Escrita do TCC

Status: `PENDENTE`

### Estrutura sugerida

- introducao
- problema e motivacao
- objetivos geral e especificos
- fundamentacao teorica (redes ad hoc/mesh, ESP-NOW, AODV/RFC 3561)
- trabalhos relacionados
- proposta do `AODV-EN` (baseada na spec v1)
- arquitetura e design (baseada no documento de estruturas)
- implementacao (lib + adapter + app)
- metodologia experimental
- resultados
- discussao
- limitacoes
- conclusao
- referencias
- apendices

### Tarefas

- transcrever a spec v1 e o documento de estruturas em texto academico
- detalhar metodologia de experimento
- consolidar resultados e interpretacoes
- revisar linguagem academica e citacoes

### Entregavel

- versao final da monografia

## Fase 10. Defesa e demonstracao

Status: `PENDENTE`

### Tarefas

- preparar slides
- preparar roteiro de fala
- preparar demo com 3 a 4 nos
- preparar plano B em video
- preparar respostas para perguntas provaveis da banca

### Perguntas que precisamos conseguir responder

- por que isso ainda e AODV?
- por que duas tabelas e duas filas?
- por que ESP-NOW v2?
- como o protocolo descobre e mantem rotas?
- onde ele simplifica e onde ele estende a RFC 3561?
- qual foi o ganho frente ao flooding?
- quais sao as limitacoes do projeto?

## Caminho critico

Para fechar o TCC com folga:

1. fechar `TC-002` com captura simultanea dos 3 nos
2. executar e fechar `TC-003` e `TC-004`
3. extrair adapter ESP-NOW dedicado
4. implementar baseline de flooding com a mesma interface
5. executar a bateria de experimentos
6. escrever resultados e discussao
7. preparar defesa

## Backlog imediato recomendado

1. fechar captura serial dos 3 nos no `TC-002`
2. extrair adapter ESP-NOW para um modulo dedicado
3. executar `TC-003` em hardware com log analisavel
4. iniciar baseline de flooding como segundo binario selecionavel por Kconfig
5. preencher campos de "Resultado" dos casos de teste a cada execucao

## Definicao de pronto do projeto

O projeto esta completo quando:

- o `AODV-EN v1` roda em ESP32 real
- os criterios de aprovacao da spec v1 estao demonstrados
- existe comparacao com baseline de flooding
- os resultados estao coletados e analisados
- o TCC esta escrito
- a apresentacao final esta preparada
