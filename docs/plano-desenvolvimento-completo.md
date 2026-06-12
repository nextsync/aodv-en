# Plano de Desenvolvimento Completo do AODV-EN

## Objetivo

Este documento organiza o desenvolvimento completo do `AODV-EN` ate a conclusao do projeto tecnico e do TCC.

Ele serve como:

- roadmap de implementacao
- checklist de validacao
- guia de experimentacao
- trilha de escrita do trabalho final

## Material visual para escrita do TCC

- fluxos consolidados (Mermaid): [fluxos-tcc-aodv-en.md](./fluxos-tcc-aodv-en.md)
- uso recomendado:
- capitulo de metodologia: `F1`, `F3`, `F4`, `F8`
- capitulo de implementacao: `F2`, `F5`, `F6`, `F7`
- capitulo de resultados/discussao: `F4`, `F8`, `F9`

## Legenda de status

- `CONCLUIDO`
- `EM_ANDAMENTO`
- `PENDENTE`
- `OPCIONAL`

## Estado atual resumido

### Ja concluido

- `CONCLUIDO` definicao dos invariantes do AODV em [aodv-base-invariantes.md](/Users/huaksonlima/Documents/tcc/aodv-en/docs/aodv-base-invariantes.md)
- `CONCLUIDO` especificacao inicial do protocolo em [aodv-en-spec-v0.md](/Users/huaksonlima/Documents/tcc/aodv-en/docs/aodv-en-spec-v0.md)
- `CONCLUIDO` modelagem das estruturas de dados em [aodv-en-estruturas-dados.md](/Users/huaksonlima/Documents/tcc/aodv-en/docs/aodv-en-estruturas-dados.md)
- `CONCLUIDO` materializacao dos limites, tipos, mensagens e tabelas do protocolo em `firmware/components/aodv_en/include`
- `CONCLUIDO` implementacao inicial das operacoes de vizinhos, rotas, cache de `RREQ` e cache de peers em `firmware/components/aodv_en/src`
- `CONCLUIDO` implementacao inicial do nucleo do no em [aodv_en_node.h](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/include/aodv_en_node.h) e [aodv_en_node.c](/Users/huaksonlima/Documents/tcc/aodv-en/firmware/components/aodv_en/src/aodv_en_node.c)
- `CONCLUIDO` simulacao local do fluxo `RREQ -> RREP -> DATA -> ACK` em [sim/aodv_en_sim.c](/Users/huaksonlima/Documents/tcc/aodv-en/sim/aodv_en_sim.c)

### O que isso significa na pratica

Hoje o projeto ja tem:

- base teorica
- especificacao inicial
- estruturas centrais do protocolo
- primeiro motor logico do no
- uma simulacao executavel validando descoberta de rota e entrega fim a fim
- um primeiro app ESP-IDF buildavel com integracao inicial a ESP-NOW
- fila pendente de `DATA` com descoberta continua e retentativa com backoff
- tratamento de falha de enlace por feedback de TX e reconvergencia
- modo de flooding de `RREQ` configuravel (`broadcast` ou `unicast_seq`)
- gerenciamento dinamico de peers ESP-NOW com LRU no adapter do app

Hoje o projeto ainda nao tem:

- validacao completa em hardware com captura sincronizada de todos os nos em todos os cenarios
- baseline de flooding
- campanha experimental completa com repeticoes por cenario
- consolidacao final das metricas do TCC (`PDR`, latencia E2E, `NRL`, energia estimada)

## Roadmap macro

| Fase | Status | Objetivo principal | Entregavel |
|---|---|---|---|
| 0. Fundacao conceitual | CONCLUIDO | Definir o que o protocolo e e o que ele nao pode violar | docs base e spec v0 |
| 1. Base estrutural do firmware | CONCLUIDO | Materializar tipos, tabelas e mensagens | headers e modulos iniciais |
| 2. Nucleo logico do protocolo | EM_ANDAMENTO | Fazer o no processar mensagens e usar tabelas locais | `aodv_en_node` + simulacao |
| 3. Endurecimento do nucleo | PENDENTE | Corrigir lacunas de MVP e consolidar regras do protocolo | versao v1 do nucleo |
| 4. Adapter ESP-NOW / ESP-IDF | EM_ANDAMENTO | Conectar o nucleo ao transporte real | adapter inicial + callbacks reais |
| 5. Firmware de teste em ESP32 | EM_ANDAMENTO | Rodar o protocolo em hardware real | app de no buildavel |
| 6. Testes funcionais de rede | PENDENTE | Validar multi-hop, falha e recuperacao | relatorio de testes |
| 7. Baseline de flooding | PENDENTE | Criar comparativo justo para o TCC | protocolo baseline |
| 8. Experimentos e metricas | PENDENTE | Coletar dados para o trabalho | tabelas, logs e resultados |
| 9. Escrita do TCC | PENDENTE | Transformar implementacao e resultados em texto academico | monografia |
| 10. Defesa e demonstracao | PENDENTE | Preparar apresentacao final | slides e demo |

## Alinhamento com base de conhecimento (AODV + TCC)

### Referencias canonicas usadas

- protocolo AODV (normativo): `/home/dioguin/Documentos/base_conhecimento/wiki/domains/networking/sources/rfc-3561-aodv-routing.md`
- comparacao oficial RFC vs implementacao: `/home/dioguin/Documentos/base_conhecimento/wiki/domains/networking/comparisons/rfc-3561-vs-aodv-en.md`
- proposta do TCC (metas do artefato): `/home/dioguin/Documentos/base_conhecimento/wiki/domains/networking/sources/tcc-aodv-en-proposal.md`
- sintese de adaptacao do projeto: `/home/dioguin/Documentos/base_conhecimento/wiki/domains/networking/synthesis/aodv-en-adaptation-synthesis.md`

### O que esta certo hoje

- fluxo reativo do AODV esta preservado (`RREQ`, `RREP`, `RERR`, descoberta sob demanda)
- supressao de duplicata de `RREQ` e controle por `sequence number` seguem o desenho base
- existe `RREQ` em broadcast (estilo RFC) e ele permanece como default
- existe modo adaptado `unicast_seq` para flooding de `RREQ`, alinhado a proposta do TCC para ESP-NOW
- o adapter agora remove/adiciona peers com politica LRU, alinhando com a restricao de peers do ESP-NOW

### O que ainda esta em aberto para fechar com o TCC

- metrica hibrida de rota (hop count + RSSI) foi iniciada no core e ainda precisa de calibracao experimental dos pesos/limiares
- baseline de flooding comparavel (`TTL` + supressao de duplicata) ainda nao esta implementado como modulo de experimento
- pipeline final de resultados (comparativo AODV-EN vs flooding com repeticoes e consolidacao estatistica) ainda esta pendente

### Planos consolidados (A-F)

| Plano | Status | Escopo | Entregavel |
|---|---|---|---|
| A | CONCLUIDO | Modo de flooding `RREQ` configuravel (`broadcast`/`unicast_seq`) | Core com chave de modo e propagacao por vizinhos ativos |
| B | CONCLUIDO | LRU real para peers ESP-NOW no adapter | Adicao/eviccao dinamica de peers com peer broadcast fixo |
| C | EM_ANDAMENTO | Metrica hibrida de roteamento (`hop + RSSI`) | Selecao de rota e criterio de substituicao com custo hibrido |
| D | PENDENTE | Baseline flooding para comparacao justa | Modulo baseline com interface equivalente de envio/medicao |
| E | PENDENTE | Campanha experimental TCC | Execucao por cenarios, repeticoes, tabelas e graficos finais |
| F | PENDENTE | Pacote de escrita e defesa | Capitulo de resultados/discussao e narrativa de ganhos/limites |

## Fase 0. Fundacao conceitual

Status: `CONCLUIDO`

### Entregas

- invariantes do AODV
- especificacao v0 do AODV-EN
- definicao de duas tabelas principais
- definicao de mensagens centrais

### Criterio de conclusao

- o projeto tem uma base teorica coerente
- o nome `AODV-EN` ja esta defendido conceitualmente

## Fase 1. Base estrutural do firmware

Status: `CONCLUIDO`

### Entregas

- limites do protocolo
- tipos centrais
- formatos de mensagem
- wrappers das tabelas
- operacoes iniciais de vizinhos, rotas, `RREQ` cache e peers

### Criterio de conclusao

- a estrutura do protocolo ja existe em C
- as tabelas centrais ja possuem API minima

## Fase 2. Nucleo logico do protocolo

Status: `EM_ANDAMENTO`

### Ja feito

- criacao de `aodv_en_node_t`
- callbacks de emissao e entrega
- processamento basico de `HELLO`, `RREQ`, `RREP`, `RERR`, `DATA` e `ACK`
- simulacao local com 3 nos em linha

### O que ainda falta nesta fase

- politica mais completa de numeros de sequencia
- regras mais refinadas de `RERR`
- melhor separacao entre rota reversa, rota valida e rota expirada
- testes unitarios e de simulacao para casos de erro

### Criterio de conclusao

- o nucleo trata o fluxo principal e os principais casos de falha sem depender do hardware
- existe uma simulacao cobrindo descoberta, entrega, duplicata e falha

## Fase 3. Endurecimento do nucleo

Status: `EM_ANDAMENTO`

### Tarefas concluidas nesta fase

- implementar fila pendente de `DATA` quando `RREQ` e disparado
- implementar controle de retransmissao e timeout de `ACK`
- revisar semantica de `RERR` e invalidacao em cascata por `next_hop`

### Tarefas pendentes nesta fase

- consolidar politica de `sequence number` para origem e destino
- adicionar helpers de serializacao e validacao de frame
- centralizar checagem de TTL, `hop_count` e tamanhos maximos
- criar cenarios de simulacao adicionais:
- `A <-> B <-> C <-> D`
- falha de enlace no meio da rota
- `RREQ` duplicado
- expiracao de rota
- ausencia de `ACK`

### Entregavel

- `AODV-EN core v1`

### Criterio de conclusao

- o nucleo deixa de ser apenas MVP e fica apto para ir ao ESP32 real

## Fase 4. Adapter ESP-NOW / ESP-IDF

Status: `EM_ANDAMENTO`

### Ja feito

- criacao do primeiro app ESP-IDF em `firmware/`
- integracao inicial com `esp_wifi_start`
- integracao inicial com `esp_now_init`
- callback de recepcao conectado ao `aodv_en_node_on_recv`
- callback de emissao do no conectado ao `esp_now_send`
- registro inicial de peer broadcast
- processamento de frames fora do callback do Wi-Fi por fila local
- configuracao de modo de flooding de `RREQ` via `Kconfig`
- integracao de gerenciamento de peers no driver com LRU e limite configuravel
- log de boot com visibilidade de modo de flooding e limite de peers

### Tarefas

- extrair a camada de transporte do `main.c` para um adapter dedicado
- encapsular `esp_wifi_start`, `esp_now_init`, peers e callbacks
- definir configuracao de canal e interface
- adicionar logs estruturados por no
- tratar melhor falhas de `esp_now_send`
- fechar testes de estresse da politica LRU em topologias maiores

### Entregavel

- adapter `aodv_en_espnow` desacoplado do app

### Criterio de conclusao

- o nucleo puro consegue rodar sobre ESP-NOW sem alterar a logica central

## Fase 5. Firmware de teste em ESP32

Status: `EM_ANDAMENTO`

### Ja feito

- projeto `ESP-IDF` criado em `firmware/`
- `main.c` inicial criado
- `menuconfig` inicial criado
- scripts de build e flash criados
- build validado com sucesso e geracao de `aodv_en_firmware.bin`
- envio periodico de `HELLO`
- envio opcional de `DATA` para MAC configurado
- logs iniciais de estatisticas e rotas

### Tarefas

- flashar o firmware em 1 ESP32 e validar boot
- validar `HELLO` no serial de 1 no
- configurar e subir 2 nos
- validar descoberta e troca de dados em hardware
- melhorar observabilidade no serial
- adicionar dumps mais claros de:
- tabela de vizinhos
- tabela de rotas
- estatisticas do no
- preparar perfis de `NODE_A`, `NODE_B` e `NODE_C`

### Entregavel

- app de bancada funcional para os ESP32

### Criterio de conclusao

- pelo menos 2 e depois 3 ESP32 conseguem executar o protocolo real em bancada

## Fase 6. Testes funcionais de rede

Status: `PENDENTE`

### Cenarios minimos

- `2` nos: vizinhanca e envio direto
- `3` nos: primeiro multi-hop
- `4` ou `5` nos: cadeia linear
- falha de enlace no meio
- reestabelecimento de rota
- descarte de `RREQ` duplicado
- expiracao de rota

### O que registrar

- logs por no
- tabelas antes e depois
- latencia aproximada
- numero de mensagens de controle
- sucesso ou falha da entrega

### Entregavel

- relatorio funcional de bancada

### Criterio de conclusao

- o protocolo esta comprovadamente operando em hardware real

## Fase 7. Baseline de flooding

Status: `PENDENTE`

### Tarefas

- implementar um baseline simples de flooding
- manter a mesma interface de app
- coletar as mesmas metricas
- garantir comparacao justa de cenarios

### Entregavel

- modulo baseline `flooding`

### Criterio de conclusao

- existe uma referencia experimental comparavel ao AODV-EN

## Fase 8. Experimentos e metricas

Status: `PENDENTE`

### Metricas minimas

- `PDR`
- latencia fim a fim
- overhead de controle
- numero total de transmissoes
- tempo para descoberta de rota
- tempo para recuperacao apos falha

### Cenarios de experimento

- topologia linear
- topologia em arvore
- malha parcial com rota alternativa
- cenario com falha induzida

### Tarefas

- definir formato de log exportavel
- executar repeticoes por cenario
- consolidar resultados em tabela
- gerar graficos
- comparar `AODV-EN` vs `Flooding`

### Observacao importante

Se nao houver medicao real de energia, tratar consumo energetico como:

- estimativa
- inferencia por numero de transmissoes
- ou limitacao explicitamente declarada

### Entregavel

- conjunto de resultados experimentais do TCC

### Criterio de conclusao

- existe base empirica suficiente para sustentar a discussao academica

## Fase 9. Escrita do TCC

Status: `PENDENTE`

### Estrutura sugerida

- introducao
- problema e motivacao
- objetivos geral e especificos
- fundamentacao teorica
- redes ad hoc e mesh
- ESP-NOW
- AODV e RFC 3561
- trabalhos relacionados
- proposta do `AODV-EN`
- arquitetura e design
- implementacao
- metodologia experimental
- resultados
- discussao
- limitacoes
- conclusao
- referencias
- apendices

### Tarefas

- escrever a descricao formal do protocolo
- descrever tabelas, mensagens e fluxos
- documentar arquitetura do firmware
- detalhar metodologia de experimento
- consolidar resultados e interpretacoes
- revisar linguagem academica e citacoes

### Entregavel

- versao final da monografia

### Criterio de conclusao

- texto pronto para banca, com resultados e discussao coerentes

## Fase 10. Defesa e demonstracao

Status: `PENDENTE`

### Tarefas

- preparar slides
- preparar roteiro de fala
- preparar demo com 2 a 5 nos
- preparar plano B em video
- preparar respostas para perguntas provaveis da banca

### Perguntas que precisamos conseguir responder

- por que isso ainda e AODV?
- por que usar duas tabelas?
- por que ESP-NOW v2?
- como o protocolo descobre e mantem rotas?
- onde ele simplifica a RFC?
- qual foi o ganho frente ao flooding?
- quais sao as limitacoes do projeto?

### Entregavel

- pacote de defesa

## Caminho critico

Se quisermos manter foco, o caminho critico do projeto e:

1. endurecer o nucleo do protocolo
2. integrar o nucleo ao ESP-NOW real
3. fazer os primeiros testes com ESP32
4. implementar o baseline de flooding
5. rodar os experimentos
6. escrever resultados e discussao
7. preparar defesa

## Backlog imediato recomendado

Os proximos passos mais importantes agora sao:

1. implementar o Plano C (metrica hibrida `hop + RSSI`) com rastreio no log
2. implementar o Plano D (baseline flooding comparavel ao AODV-EN)
3. executar campanha do Plano E com repeticoes por cenario e coleta padronizada
4. consolidar analise final do ganho (PDR, latencia, NRL e energia estimada)
5. fechar Plano F com capitulo de resultados, discussao e narrativa de defesa

## Definicao de pronto do projeto

O projeto pode ser considerado completo quando:

- o `AODV-EN` roda em ESP32 real
- a descoberta e o encaminhamento multi-hop funcionam
- existe comparacao com baseline de flooding
- os resultados estao coletados e analisados
- o TCC esta escrito
- a apresentacao final esta preparada
