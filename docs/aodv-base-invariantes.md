# AODV Base Invariantes

## Estado

- alinhado com [aodv-en-spec-v1.md](aodv-en-spec-v1.md)
- ultima revisao: 2026-05-01

## Objetivo

Este documento define o nucleo conceitual do AODV que o projeto `aodv-en` deve preservar.

Ele existe para evitar que o protocolo evolua de forma inconsistente ao longo da implementacao do TCC. Sempre que surgir uma nova ideia de arquitetura, mensagem, heuristica ou otimizacao, ela deve ser comparada com estas regras.

Se uma decisao violar um destes invariantes, o resultado deixa de ser uma adaptacao do AODV e passa a ser outro protocolo.

## Escopo

O projeto `aodv-en` e uma adaptacao do AODV para uma rede mesh multi-hop baseada em ESP-NOW e ESP32.

As adaptacoes de plataforma sao permitidas, mas o comportamento essencial do algoritmo deve continuar reconhecivel como AODV.

## Invariantes obrigatorios

### 1. Roteamento reativo

O protocolo deve descobrir rotas sob demanda.

Isso significa:

- a descoberta de rota ocorre quando um no precisa enviar dados e nao possui rota valida
- o protocolo nao deve manter um mapa global da rede atualizado continuamente
- o protocolo nao deve depender de anuncios periodicos de topologia para funcionar

### 2. Descoberta por RREQ e resposta por RREP

O mecanismo central de descoberta de rotas deve continuar sendo:

- `RREQ` para procurar uma rota
- `RREP` para responder a uma rota encontrada

Isso implica:

- a origem emite `RREQ` quando nao encontra rota valida
- o destino ou um no com rota suficientemente valida responde com `RREP`
- a descoberta nao pode ser substituida por flooding puro sem semantica de resposta

### 3. Roteamento hop-by-hop

As rotas devem ser mantidas localmente no formato:

- `destino -> next_hop`

O protocolo nao deve carregar a rota completa dentro de cada pacote de dados, como em source routing.

### 4. Numeros de sequencia

O protocolo deve usar numeros de sequencia para representar o frescor da informacao de roteamento.

Esses numeros sao obrigatorios para:

- comparar rotas
- preferir informacao mais recente
- reduzir risco de loop
- manter coerencia com o AODV

### 5. Rota reversa e rota direta

Durante o processo de descoberta:

- o `RREQ` deve construir rota reversa para a origem
- o `RREP` deve consolidar rota direta para o destino

Essas duas ideias sao centrais no comportamento do AODV.

### 6. Supressao de duplicatas de RREQ

O protocolo deve ser capaz de identificar e descartar `RREQ`s duplicados.

No minimo, isso exige o uso combinado de:

- `originator`
- `rreq_id`

Sem esse controle, a inundacao se torna redundante e pouco controlada.

### 7. Invalidação de rotas

Rotas nao podem ser consideradas validas indefinidamente.

O protocolo deve possuir pelo menos um destes mecanismos:

- expiracao por tempo
- sinalizacao explicita de erro via `RERR`
- ambos

### 8. Ausencia de loop como objetivo de projeto

Toda extensao proposta deve preservar a propriedade de roteamento sem loop, ou no minimo nao introduzir comportamento que contradiga as garantias buscadas pelo AODV.

## Elementos que podem ser adaptados

Os itens abaixo podem ser adaptados ao ESP-NOW sem descaracterizar o AODV:

- enderecamento por MAC em vez de IP
- encapsulamento das mensagens em payload proprietario
- uso de `HELLO`, escuta passiva ou feedback de enlace para manter conectividade local
- adicao de metricas como `RSSI`, desde que `hop_count` e `sequence_number` continuem semanticamente coerentes
- politica de gerenciamento de peers do ESP-NOW
- mecanismos de ACK de aplicacao ou confirmacao fim a fim

## Elementos que nos afastariam do AODV

As decisoes abaixo descaracterizam a proposta se virarem parte central do protocolo:

- remover `RREQ` e `RREP` do fluxo principal de descoberta
- remover numeros de sequencia
- usar source routing como mecanismo principal
- manter topologia global de forma proativa
- substituir tabela de rotas por flooding permanente
- usar apenas heuristicas locais sem semantica de rota reversa e direta

## Interpretacao para o AODV-EN

O `aodv-en` deve ser entendido como:

- uma adaptacao do AODV ao ESP-NOW
- com tabelas locais de vizinhos e de rotas
- com descoberta reativa de rotas
- com manutencao hop-by-hop
- com numeros de sequencia
- com supressao de duplicatas
- com invalidacao temporal ou explicita de rotas
- com precursores por rota para emissao direcionada de `RERR`
- com fila local de `DATA` pendente durante descoberta

## Compatibilidade com extensoes da v1

A v1 do `AODV-EN` adiciona dois mecanismos que precisam ser revisados contra os invariantes acima:

### Fila de `DATA` pendente

A fila armazena um pacote da aplicacao enquanto o `RREQ`/`RREP` correspondente esta em curso. Ela:

- nao substitui a descoberta reativa (o `RREQ` continua sendo emitido)
- nao mantem topologia global, apenas pacotes pendentes do proprio no
- nao introduz mecanismo de roteamento alternativo

Por isso, e compativel com os invariantes 1 (reativo), 2 (descoberta por `RREQ`/`RREP`) e 8 (sem loop).

### Precursores por rota

A v1 mantem, em cada rota direta valida, ate `MAX_PRECURSORS` MACs vizinhos que a usam. Esse conjunto e usado para:

- direcionar `RERR` apenas a quem precisa saber
- preservar a propriedade de `RERR` propagar de forma controlada para tras na arvore de descoberta

Precursores fazem parte da semantica original da RFC 3561 (secao 6.2). Adopta-los aproxima o `AODV-EN` do AODV padrao em vez de afasta-lo, e nao introduz nem descoberta proativa nem topologia global.

## Checklist de validacao

Antes de aceitar uma mudanca arquitetural, verificar:

- a descoberta continua sendo reativa?
- ainda existe `RREQ`?
- ainda existe `RREP`?
- as rotas continuam sendo `destino -> next_hop`?
- os numeros de sequencia continuam presentes?
- `RREQ` duplicado continua sendo descartado?
- ainda existe rota reversa e rota direta?
- ainda existe expiracao ou erro de rota?

Se alguma resposta for "nao", a mudanca deve ser reavaliada.
