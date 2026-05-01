# Firmware ESP32 do AODV-EN

App ESP-IDF de bancada que consome a lib `AODV-EN` ([components/aodv_en](components/aodv_en)) sobre ESP-NOW v2.

A spec funcional do protocolo esta em [../docs/aodv-en-spec-v1.md](../docs/aodv-en-spec-v1.md). Os criterios de aprovacao em hardware estao mapeados em [../docs/tests](../docs/tests).

## O que ele faz hoje

- inicia Wi-Fi em modo `STA`
- inicia `ESP-NOW`
- cria um no `AODV-EN`
- envia `HELLO` periodico
- processa frames recebidos fora do callback do Wi-Fi
- opcionalmente tenta enviar `DATA` periodico para um MAC configurado
- imprime logs, estatisticas e estado geral no serial

## Modos de aplicacao

Selecionavel por `menuconfig` -> `AODV-EN App` -> `Application example mode`:

- `app_proto_example` (padrao): camada de aplicacao com `HEALTH/TEXT/CMD` e CLI serial
- `app_demo` (legado): apenas `HELLO` e `DATA` periodicos

`main.c` faz o branch via `CONFIG_AODV_EN_APP_USE_APP_DEMO` / `CONFIG_AODV_EN_APP_USE_PROTO_EXAMPLE`.

## API da lib (para outras aplicacoes)

A fachada da lib esta em [components/aodv_en/include/aodv_en.h](components/aodv_en/include/aodv_en.h).

Padrao de uso recomendado:

1. Definir um `adapter` com duas funcoes:
   - `now_ms(user_ctx)` para fornecer tempo em ms.
   - `tx_frame(user_ctx, next_hop, frame, len, broadcast)` para enviar frame no radio/transporte.
2. Definir callbacks de app:
   - `on_data(...)` para entrega de payload.
   - `on_ack(...)` para confirmacoes.
3. Inicializar stack:
   - `aodv_en_stack_init(...)`.
4. No loop da aplicacao:
   - chamar `aodv_en_stack_tick_at(..., now_ms)` periodicamente;
   - encaminhar RX por `aodv_en_stack_on_recv_at(...)`;
   - encaminhar resultado TX por `aodv_en_stack_on_link_tx_result_at(...)`.
5. Envio de mensagens:
   - `aodv_en_stack_send_hello_at(...)`
   - `aodv_en_stack_send_data_at(...)`

Esse modelo separa o protocolo (core) da plataforma (adapter), o que permite portar para outros ambientes sem alterar o nucleo do `AODV-EN`. A simulacao em [../sim](../sim) usa exatamente esse contrato com mocks.

## Como buildar

```bash
zsh firmware/build.sh
```

## Como gravar e abrir monitor

```bash
zsh firmware/flash_monitor.sh /dev/cu.usbserial-XXXX
```

## Como gravar e salvar log em arquivo

```bash
zsh firmware/monitor_log.sh -p /dev/ttyUSB0 -B build/tc002_node_a -t tc004_soak -l node_a
```

Arquivos salvos em `firmware/logs/serial/`.

## Casos de teste por papel

- [tests/tc001](tests/tc001) - 2 nos diretos (`TC-001`)
- [tests/tc002](tests/tc002) - 3 nos em cadeia (`TC-002`, `TC-003`, `TC-004`)
- [tests/tc005](tests/tc005) - 4 nos em cadeia (`TC-005`)

Cada diretorio tem `build_flash.sh`, `monitor_log.sh` (wrapper) e `node_X.defaults`.

## Como configurar

```bash
source firmware/idf-env.sh
idf.py menuconfig
```

`idf-env.sh` resolve o ESP-IDF nesta ordem:

1. `ESP_IDF_EXPORT` (quando definido)
2. `IDF_PATH/export.sh`
3. `$HOME/esp/esp-idf/export.sh`
4. `idf.py` ja disponivel no `PATH`

Menu: `AODV-EN App`. Campos principais:

- modo de exemplo (`app_proto_example` ou `app_demo`)
- nome do no
- `network_id`
- canal Wi-Fi
- intervalo de `HELLO`
- habilitacao de `DATA`
- MAC alvo
- texto do payload

Para `app_proto_example`, ficam disponiveis campos extras:

- `PROTO health interval (ms)`
- `PROTO unicast interval (ms)`
- `PROTO enable periodic unicast text/cmd`
- `PROTO enable serial CLI commands`
- `PROTO target MAC (unicast)`
- `PROTO text payload`
- `PROTO command name`
- `PROTO command args`

## CLI serial (modo `app_proto_example`)

Quando `PROTO enable serial CLI commands` estiver habilitado:

- `help`
- `health all`
- `health <AA:BB:CC:DD:EE:FF>`
- `text <AA:BB:CC:DD:EE:FF> <mensagem>`
- `cmd <AA:BB:CC:DD:EE:FF> <comando> [args]`
- `routes`

Comandos embutidos: `ping`, `echo`, `info`.

## Dashboard ao vivo (real-time)

[firmware/tools/live_monitor.py](tools/live_monitor.py) levanta um dashboard web em `http://localhost:8765/` que le multiplas portas seriais em paralelo, parseia os logs e empurra eventos via WebSocket para uma topologia animada (Cytoscape.js) com painel de metricas e timeline.

```bash
# instalar dependencias uma vez
pip install aiohttp pyserial

# modo demo (sem hardware) - util para validar o dashboard
python3 firmware/tools/live_monitor.py --demo

# com hardware: uma --port DEV:ALIAS por no
python3 firmware/tools/live_monitor.py \
    --port /dev/ttyUSB0:NODE_A \
    --port /dev/ttyUSB1:NODE_B \
    --port /dev/ttyUSB2:NODE_C
```

Recursos visuais:
- topologia ao vivo com cores por papel (gateway, relay, leaf) e estado online/offline
- pulse verde animando o caminho `A -> B -> C` quando o `ACK` chega na origem
- flash vermelho em `ESP-NOW send fail` e `invalidated` (link quebrado)
- card de alerta para fila cheia (`status=-2`) e falhas de enlace
- timeline scrollavel com 60 eventos recentes
- painel "Resumo da malha" com nos online, hops max, rotas validas, ACKs e deliveries

A interface esta em [firmware/tools/live_monitor_web/](tools/live_monitor_web/) (HTML + CSS + JS, sem build system).

## Ferramentas de analise

A partir dos logs em `firmware/logs/serial/`:

```bash
python3 firmware/tools/extract_monitor_metrics.py firmware/logs/serial/<arquivo>.log
```

Saida em `firmware/logs/analysis/<basename>/`:
- `summary.json` e `summary.txt`
- CSVs: `events`, `snapshots`, `routes`, `target_route_series`, `ack_events`, `discovery_windows`, `minute_metrics`...

Geracao de graficos:

```bash
python3 firmware/tools/plot_monitor_metrics.py firmware/logs/analysis/<basename>
python3 firmware/tools/plot_comparison_metrics.py --run label1::path1/summary.json --run label2::path2/summary.json
```

Geracao de topologia:

```bash
python3 firmware/tools/draw_topology.py firmware/logs/analysis/<basename> --mode latest
```

Saidas em `topology/`: `topology.mmd` (Mermaid), `topology.dot` (Graphviz), `topology.svg` (quando `dot` estiver instalado), `topology.json` (resumo).

## Observacao sobre permissoes

Se os scripts ainda nao estiverem com permissao de execucao no seu ambiente, rode-os com `zsh`, como mostrado acima.
