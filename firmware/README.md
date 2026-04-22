# Firmware ESP32 do AODV-EN

Este diretorio contem o primeiro app ESP-IDF executavel do projeto.

## O que ele faz hoje

- inicia Wi-Fi em modo `STA`
- inicia `ESP-NOW`
- cria um no `AODV-EN`
- envia `HELLO` periodico
- processa frames recebidos fora do callback do Wi-Fi
- opcionalmente tenta enviar `DATA` periodico para um MAC configurado
- imprime logs, estatisticas e estado geral no serial

## API da lib (para outras aplicacoes)

Agora o componente oferece uma fachada de integracao em:

- `firmware/components/aodv_en/include/aodv_en.h`

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

Esse modelo separa o protocolo (core) da plataforma (adapter), facilitando portar para outros ambientes sem alterar o nucleo do AODV-EN.

## Como buildar

```bash
cd /Users/huaksonlima/Documents/tcc/aodv-en/firmware
zsh build.sh
```

## Como gravar

```bash
cd /Users/huaksonlima/Documents/tcc/aodv-en/firmware
zsh flash_monitor.sh /dev/cu.usbserial-XXXX
```

## Como monitorar e salvar log em arquivo

```bash
cd /Users/huaksonlima/Documents/tcc/aodv-en
zsh firmware/monitor_log.sh -p /dev/ttyUSB0 -B build/tc002_node_a -t tc004_soak -l node_a
```

Arquivo salvo em:

- `firmware/logs/serial/`

## Como configurar

```bash
cd /Users/huaksonlima/Documents/tcc/aodv-en/firmware
source ./idf-env.sh
idf.py menuconfig
```

`idf-env.sh` e o bootstrap padrao para o firmware. Ele tenta carregar o ESP-IDF nesta ordem:

1. `ESP_IDF_EXPORT` (quando definido)
2. `IDF_PATH/export.sh`
3. `$HOME/esp/esp-idf/export.sh`
4. `idf.py` ja disponivel no `PATH`

## Observacao

Se os scripts ainda nao estiverem com permissao de execucao no seu ambiente, rode-os com `zsh`, como mostrado acima.

Menu:

- `AODV-EN App`

Campos principais:

- modo de exemplo da aplicacao:
  - `app_demo` (legado)
  - `app_proto_example` (health/text/command)
- nome do no
- `network_id`
- canal Wi-Fi
- intervalo de `HELLO`
- habilitacao de `DATA`
- MAC alvo
- texto do payload

Se selecionar `app_proto_example`, ficam disponiveis campos extras:

- `PROTO health interval (ms)`
- `PROTO unicast interval (ms)`
- `PROTO enable periodic unicast text/cmd`
- `PROTO enable serial CLI commands`
- `PROTO target MAC (unicast)`
- `PROTO text payload`
- `PROTO command name`
- `PROTO command args`

Com esse exemplo novo:

- cada no responde automaticamente `HEALTH_REQ` com `HEALTH_RSP`;
- e possivel enviar `TEXT` para um alvo unicast configurado;
- e possivel enviar `CMD_REQ` para outro no, com handlers de exemplo:
  - `ping`
  - `echo`
  - `info`

Se `PROTO enable serial CLI commands` estiver habilitado, voce tambem pode digitar no monitor:

- `help`
- `health all`
- `health <AA:BB:CC:DD:EE:FF>`
- `text <AA:BB:CC:DD:EE:FF> <mensagem>`
- `cmd <AA:BB:CC:DD:EE:FF> <comando> [args]`
- `routes`

## Desenho da topologia (a partir dos logs)

Depois de extrair os metrics de um log com `extract_monitor_metrics.py`, voce pode gerar um desenho da topologia:

```bash
python3 firmware/tools/draw_topology.py \
  firmware/logs/analysis/node_a_tc004_soak_v2_20260421-183532 \
  --mode latest
```

Saidas:

- `topology/topology.mmd` (Mermaid)
- `topology/topology.dot` (Graphviz DOT)
- `topology/topology.svg` (quando `dot` estiver instalado)
- `topology/topology.json` (resumo estruturado)

Para incluir todos os links observados durante a execucao (nao so o ultimo snapshot), use:

```bash
python3 firmware/tools/draw_topology.py \
  firmware/logs/analysis/node_a_tc004_soak_v2_20260421-183532 \
  --mode observed \
  --name topology_observed
```
