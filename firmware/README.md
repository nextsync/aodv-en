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

- nome do no
- `network_id`
- canal Wi-Fi
- intervalo de `HELLO`
- habilitacao de `DATA`
- MAC alvo
- texto do payload
