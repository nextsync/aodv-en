# Fluxos para Escrita do TCC - AODV-EN

Este documento consolida os fluxos principais para uso direto no TCC.
Os diagramas estao em Mermaid para facilitar manutencao e exportacao.

## Como usar no TCC

- Capitulo 4 (Metodologia): usar os fluxos F1, F3, F4, F8.
- Capitulo 5 (Implementacao): usar os fluxos F2, F5, F6, F7.
- Capitulo 6 (Resultados e Discussao): usar os fluxos F4, F8, F9.

## F1 - Encadeamento metodologico (DSR)

```mermaid
flowchart LR
    A[Problema: ESP-NOW sem multi-hop nativo] --> B[Objetivos do artefato]
    B --> C[Design AODV-EN]
    C --> D[Implementacao no firmware ESP32]
    D --> E[Demonstracao em cenarios C1..C4]
    E --> F[Avaliacao por metricas]
    F --> G[Comunicacao: TCC, figuras e discussao]
```

## F2 - Arquitetura do artefato implementado

```mermaid
flowchart TB
    APP[App de teste / App de protocolo]
    FACADE[aodv_en_stack_* facade]
    CORE[Core AODV-EN<br/>RREQ RREP RERR DATA ACK]
    ADAPTER[Adapter ESP-NOW<br/>peer mgmt LRU + TX/RX]
    RADIO[ESP-NOW / Wi-Fi driver]
    LOGS[Logs seriais]
    TOOLS[extract + plot + comparison]

    APP --> FACADE
    FACADE --> CORE
    FACADE --> ADAPTER
    ADAPTER --> RADIO
    APP --> LOGS
    LOGS --> TOOLS
```

## F3 - Fluxo da campanha experimental (cenarios e repeticoes)

```mermaid
flowchart TD
    A[Definir cenario C1 C2 C3 C4] --> B[Configurar firmware e parametros]
    B --> C[Flash dos nos e setup fisico]
    C --> D[Executar repeticao r_i]
    D --> E[Capturar logs por no]
    E --> F{r_i atingiu minimo?}
    F -- nao --> D
    F -- sim --> G[Consolidar dataset do cenario]
    G --> H{Proximo cenario?}
    H -- sim --> A
    H -- nao --> I[Fechar base para analise estatistica]
```

## F4 - Pipeline de coleta, extracao e graficos

```mermaid
flowchart LR
    A[monitor_log.sh] --> B[log serial .log]
    B --> C[extract_monitor_metrics.py]
    C --> D[summary.json + CSV]
    D --> E[plot_monitor_metrics.py]
    D --> F[plot_comparison_metrics.py]
    E --> G[Graficos por execucao]
    F --> H[Graficos comparativos]
    G --> I[Secao de resultados]
    H --> I
```

## F5 - Fluxo nominal de descoberta e entrega (AODV-EN)

```mermaid
sequenceDiagram
    participant O as Origem
    participant N as Nos intermediarios
    participant D as Destino

    O->>N: RREQ (sem rota valida)
    N->>N: cria rota reversa + supressao duplicata
    N->>D: RREQ encaminhado
    D-->>N: RREP unicast (rota encontrada)
    N-->>O: RREP no caminho reverso
    O->>N: DATA
    N->>D: DATA encaminhado
    D-->>O: ACK de aplicacao
```

## F6 - Fluxo de falha e reconvergencia

```mermaid
flowchart TD
    A[DATA em rota ativa] --> B{Falha de enlace no next_hop?}
    B -- nao --> A
    B -- sim --> C[Invalidar rotas afetadas]
    C --> D[Gerar/propagar RERR]
    D --> E[Enfileirar DATA pendente]
    E --> F[Disparar nova descoberta RREQ]
    F --> G{Rota recuperada?}
    G -- nao --> H[Backoff + novas tentativas]
    H --> F
    G -- sim --> I[Desenfileirar e reenviar DATA]
    I --> J[ACK volta a subir]
```

## F7 - Fluxo do baseline Flooding (planejado para comparacao)

```mermaid
flowchart TD
    A[Receber pacote] --> B{Duplicata?}
    B -- sim --> X[Descartar]
    B -- nao --> C{Destino local?}
    C -- sim --> D[Entregar payload]
    C -- nao --> E{TTL > 0?}
    E -- nao --> X
    E -- sim --> F[TTL = TTL - 1]
    F --> G[Retransmitir para vizinhos]
    G --> H[Registrar contador de controle]
```

## F8 - Fluxo de calculo das metricas

```mermaid
flowchart LR
    A[Eventos de envio/recepcao] --> B[PDR]
    A --> C[Latencia E2E]
    A --> D[NRL]
    A --> E[Energia estimada]
    B --> F[Media + desvio + IC95]
    C --> F
    D --> F
    E --> F
    F --> G[Comparacao AODV-EN vs Flooding]
```

## F9 - Fluxo de analise estatistica e escrita

```mermaid
flowchart TD
    A[Tabelas e graficos finais] --> B[Verificar premissas dos dados]
    B --> C{Normalidade atendida?}
    C -- sim --> D[Teste parametrico]
    C -- nao --> E[Teste nao parametrico]
    D --> F[Interpretar efeito por metrica]
    E --> F
    F --> G[Aceitar/rejeitar H1..H4]
    G --> H[Escrever resultados]
    H --> I[Escrever discussao, limitacoes e ameacas]
    I --> J[Conclusao conectada aos objetivos]
```
