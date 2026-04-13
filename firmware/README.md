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

## Como configurar

```bash
cd /Users/huaksonlima/Documents/tcc/aodv-en/firmware
source ~/.zshrc
idfenv
idf.py menuconfig
```

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
