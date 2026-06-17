# Runbook de Bancada - AODV-EN

## Para quem e este documento

Voce esta com `N` ESP32 na mesa, quer flashar o firmware `AODV-EN` em cada um, validar um caso de teste (`TC-001`/`TC-002`/`TC-005`) e ver a malha funcionando. Este runbook leva voce do "ESP plugado na USB" ate "rota e ACK aparecendo no log" sem precisar consultar outros documentos durante a sessao.

Para a teoria do protocolo, ver [aodv-en-funcionamento.md](aodv-en-funcionamento.md). Para os criterios formais de aprovacao, ver [aodv-en-spec-v1.md](aodv-en-spec-v1.md). Para os casos de teste documentados, ver [tests/](tests/).

---

## Sumario

1. [Setup unico (uma vez por maquina)](#1-setup-unico-uma-vez-por-maquina)
2. [Bootstrap de cada sessao](#2-bootstrap-de-cada-sessao)
3. [Identificar os ESP32 conectados](#3-identificar-os-esp32-conectados)
4. [Folha "minha bancada"](#4-folha-minha-bancada)
5. [Build do firmware](#5-build-do-firmware)
6. [Fluxo geral de um caso de teste](#6-fluxo-geral-de-um-caso-de-teste)
7. [TC-001 - 2 nos diretos (passo a passo)](#7-tc-001---2-nos-diretos-passo-a-passo)
8. [TC-002 - 3 nos em cadeia](#8-tc-002---3-nos-em-cadeia)
9. [TC-003 - reconvergencia](#9-tc-003---reconvergencia)
10. [TC-004 - soak de 30 minutos](#10-tc-004---soak-de-30-minutos)
11. [TC-005 - cadeia de 4 nos](#11-tc-005---cadeia-de-4-nos)
12. [Monitor: dashboard vs texto](#12-monitor-dashboard-vs-texto)
13. [Analise pos-execucao](#13-analise-pos-execucao)
14. [Troubleshooting](#14-troubleshooting)
15. [Cheatsheet de comandos](#15-cheatsheet-de-comandos)

---

## 1. Setup unico (uma vez por maquina)

Esses passos voce faz **uma vez** quando preparar a estacao de trabalho.

### 1.1 ESP-IDF

Esta maquina tem `ESP-IDF v6.0` em `/Users/huaksonlima/.espressif/v6.0/esp-idf` e a virtualenv Python em `/Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env`.

Para **bootstrap automatizado** sem brigar com `pyenv`, foi criado um wrapper:

```bash
cat /tmp/aodv-en-idf-env.sh
```

Conteudo:
```bash
export PATH="/Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env/bin:$PATH"
export ESP_IDF_EXPORT="/Users/huaksonlima/.espressif/v6.0/esp-idf/export.sh"
source "$ESP_IDF_EXPORT" >/dev/null 2>&1 || true
```

> **Persistencia**: como `/tmp` e volatil, copie esse arquivo para `~/aodv-en-idf-env.sh` ou para o repo (`firmware/idf-env-local.sh`) se quiser que ele sobreviva a reinicializacao do macOS.

### 1.2 Dependencias do dashboard

O `live_monitor.py` precisa de `aiohttp` e `pyserial`. **Atencao**: quando voce sourceia o `idf-env.sh`, o `python3` do PATH muda para o da venv do ESP-IDF (`idf6.0_py3.14_env`). Se voce instalar com `pip install --user` no python global, a venv do IDF nao enxerga.

Solucao recomendada: instale **dentro da venv do ESP-IDF**:

```bash
/Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env/bin/pip install aiohttp pyserial
```

Verificacao:
```bash
source /tmp/aodv-en-idf-env.sh
python3 -c "import aiohttp, serial; print(aiohttp.__version__, serial.__version__)"
# saida esperada: 3.13.5 3.5
```

Necessario apenas se voce for usar o `live_monitor.py` (dashboard real-time).

### 1.3 Repo

Trabalhar em:

```
/Users/huaksonlima/Documents/tcc/aodv-en
```

Branch ativa: `develop`.

---

## 2. Bootstrap de cada sessao

Toda nova sessao de bancada comeca com esses 3 comandos:

```bash
# 1) navegar pro repo
cd /Users/huaksonlima/Documents/tcc/aodv-en

# 2) checar branch e estado
git status
git branch --show-current

# 3) carregar ESP-IDF
source /tmp/aodv-en-idf-env.sh
idf.py --version    # esperado: ESP-IDF v6.0
```

Se `idf.py --version` falhar, refaca o passo 1.1.

---

## 3. Identificar os ESP32 conectados

### 3.1 Listar portas seriais

```bash
ls /dev/cu.usbserial-*
```

Saida esperada (com 3 ESPs):

```
/dev/cu.usbserial-2110
/dev/cu.usbserial-2120
/dev/cu.usbserial-2130
```

> Cada ESP32 com chip CP2102/CH340 aparece como `cu.usbserial-XXXX`. Os numeros mudam entre conexoes USB; sempre relista antes de uma sessao.

### 3.2 Ler MAC sem flashar

Para cada porta, leia o MAC. Use o `esptool` ja instalado:

```bash
ESPTOOL=/Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env/bin/esptool

"$ESPTOOL" --port /dev/cu.usbserial-2110 --before default_reset --after hard_reset read_mac 2>&1 | grep "MAC:" | head -1
"$ESPTOOL" --port /dev/cu.usbserial-2120 --before default_reset --after hard_reset read_mac 2>&1 | grep "MAC:" | head -1
"$ESPTOOL" --port /dev/cu.usbserial-2130 --before default_reset --after hard_reset read_mac 2>&1 | grep "MAC:" | head -1
```

Saida esperada (cada linha):

```
MAC:                28:05:a5:34:03:50
MAC:                28:05:a5:33:b9:ec
MAC:                28:05:a5:34:05:64
```

> **Por que ler MAC antes de flashar**: para flashar `NODE_A` precisamos do MAC do destino (`NODE_B` em `TC-001`, `NODE_C` em `TC-002`, `NODE_D` em `TC-005`). Lendo todos os MACs antes, voce nao precisa do ciclo flash-anotar-reflash.

### 3.3 Atribuir papeis

Atribua cada porta a um papel (`NODE_A`, `NODE_B`, etc.) e **anote**. Convencao recomendada:

| Papel | Comportamento |
|---|---|
| `NODE_A` | origem; envia `DATA` periodico para o destino |
| `NODE_B` | intermediario (relay) ou destino direto, depende do TC |
| `NODE_C` | intermediario ou destino, depende do TC |
| `NODE_D` | destino final em `TC-005` |

Nao importa qual ESP fisico vira qual papel; apenas seja consistente dentro da sessao.

---

## 4. Folha "minha bancada"

Preencha esta tabela no inicio de cada sessao. Cole numa nota local; nao precisa ficar versionada.

```
data: __________________
sessao: ________________

| papel  | porta                    | MAC                  | observacoes |
|--------|--------------------------|----------------------|-------------|
| NODE_A | /dev/cu.usbserial-2110   | 28:05:A5:34:03:50    | origem      |
| NODE_B | /dev/cu.usbserial-2120   | 28:05:A5:33:B9:EC    | relay       |
| NODE_C | /dev/cu.usbserial-2130   | 28:05:A5:34:05:64    | destino     |
| NODE_D | _____________            | _____________        | _________   |
```

Esses MACs sao os atuais. Os do `TC-002 PASS de 2026-04-21` eram outros (`33:D6:1C`, `33:EB:80`, `34:99:34`); essa rotacao de hardware e normal.

---

## 5. Build do firmware

O `build_flash.sh` de cada caso de teste ja faz build implicito antes de flashar. Mas em muitos cenarios voce **deve** rodar um build limpo manualmente, especialmente:

- depois de pull/checkout que mexeu em codigo;
- depois de mudancas em headers (cache de dependencias do ESP-IDF nao percebe sempre);
- na primeira vez que o repo e clonado.

```bash
cd /Users/huaksonlima/Documents/tcc/aodv-en
source /tmp/aodv-en-idf-env.sh
rm -rf firmware/build
zsh firmware/build.sh
```

Tempo esperado: 60 a 120 segundos (primeira vez); 5 a 15 segundos (incremental).

Saida final esperada:

```
Generated /Users/huaksonlima/Documents/tcc/aodv-en/firmware/build/aodv_en_firmware.bin
aodv_en_firmware.bin binary size 0xbf190 bytes. Smallest app partition is 0x100000 bytes. ...
```

> **Importante**: `build.sh` builda em `firmware/build/`. Os scripts `tc00X/build_flash.sh` buildam em `firmware/build/tc00X_node_X/` (separados por papel/teste para evitar conflito de Kconfig defaults).

---

## 6. Fluxo geral de um caso de teste

Independente do TC, a sequencia e:

```
1. bootstrap da sessao (secao 2)
2. identificar ESPs e atribuir papeis (secao 3, 4)
3. flashar destino primeiro (NODE_B/C/D conforme TC)
4. flashar intermediarios
5. flashar origem (NODE_A) com TARGET_MAC do destino
6. posicionar fisicamente os nos conforme topologia do TC
7. abrir monitor (dashboard ou texto) em todos os nos simultaneamente
8. observar logs por 30-180s
9. avaliar criterios de PASS
10. salvar logs e analisar (secao 13)
```

---

## 7. TC-001 - 2 nos diretos (passo a passo)

**Objetivo**: validar descoberta + entrega + ACK em 1 salto. Os dois nos ficam dentro do alcance direto um do outro.

**Hardware necessario**: 2 ESP32. Vamos usar `2110` (NODE_B, destino) e `2120` (NODE_A, origem). Anote os MACs no formulario da secao 4.

### Passo 1 - Flash NODE_B (destino, sem target)

```bash
cd /Users/huaksonlima/Documents/tcc/aodv-en
source /tmp/aodv-en-idf-env.sh
zsh firmware/tests/tc001/build_flash.sh node_b /dev/cu.usbserial-2110
```

Tempo: ~30s a primeira vez, ~10s incremental.

Saida final esperada:

```
ok: node_b gravado em /dev/cu.usbserial-2110
monitor:
  idf.py -B firmware/build/tc001_node_b -p /dev/cu.usbserial-2110 monitor
```

### Passo 2 - Flash NODE_A com MAC do NODE_B

Use o MAC que voce leu no passo 3.2. Para esta bancada, o NODE_B e `28:05:A5:34:03:50`.

```bash
zsh firmware/tests/tc001/build_flash.sh node_a /dev/cu.usbserial-2120 28:05:A5:34:03:50
```

### Passo 3 - Abrir monitor

**Opcao A - dashboard real-time** (recomendado para impressao visual):

```bash
python3 firmware/tools/live_monitor.py \
    --port /dev/cu.usbserial-2110:NODE_B \
    --port /dev/cu.usbserial-2120:NODE_A
# abrir http://localhost:8765/
```

**Opcao B - monitor texto com captura de log** (recomendado para analise pos-execucao):

Abra dois terminais:

```bash
# terminal 1
zsh firmware/monitor_log.sh -p /dev/cu.usbserial-2110 -B build/tc001_node_b -t tc001_run -l node_b
# terminal 2
zsh firmware/monitor_log.sh -p /dev/cu.usbserial-2120 -B build/tc001_node_a -t tc001_run -l node_a
```

Logs salvos em `firmware/logs/serial/`.

### Passo 4 - Avaliar PASS

Logs esperados em **NODE_A**:

```
I (xxx) aodv_en_proto: node=NODE_A self_mac=28:05:A5:33:B9:EC channel=6 network_id=0xA0DE0001
I (xxx) aodv_en_proto: ESP-NOW version=2
I (xxx) aodv_en_proto: routes=1 neighbors=1 tx=N rx=N delivered=0
I (xxx) aodv_en_proto: route[0] dest=28:05:A5:34:03:50 via=28:05:A5:34:03:50 hops=1 metric=1 state=2 expires=...
I (xxx) aodv_en_proto: ACK received from 28:05:A5:34:03:50 for seq=1
I (xxx) aodv_en_proto: ACK received from 28:05:A5:34:03:50 for seq=2
...
```

Logs esperados em **NODE_B**:

```
I (xxx) aodv_en_proto: node=NODE_B self_mac=28:05:A5:34:03:50 ...
I (xxx) aodv_en_proto: DATA deliver from 28:05:A5:33:B9:EC: tc001 hello
I (xxx) aodv_en_proto: routes=1 neighbors=1 tx=N rx=N delivered=N
```

Criterio formal em [tests/tc-001-descoberta-e-entrega-direta.md](tests/tc-001-descoberta-e-entrega-direta.md).

### Passo 5 - Encerrar

- Ctrl+] para sair do `idf.py monitor`
- Ctrl+C para parar o `live_monitor.py`

Logs ficam em `firmware/logs/serial/`. Veja secao 13 para extrair metricas.

---

## 8. TC-002 - 3 nos em cadeia

**Objetivo**: multi-hop em 2 saltos `A-B-C`. `B` faz relay; `A` e `C` sem enlace direto.

**Hardware**: 3 ESP32.

### Passo 1 - Flash dos 3 nos

```bash
# destino primeiro (sem target)
zsh firmware/tests/tc002/build_flash.sh node_c /dev/cu.usbserial-2130

# intermediario (sem target)
zsh firmware/tests/tc002/build_flash.sh node_b /dev/cu.usbserial-2120

# origem com MAC do destino (NODE_C = 28:05:A5:34:05:64)
zsh firmware/tests/tc002/build_flash.sh node_a /dev/cu.usbserial-2110 28:05:A5:34:05:64
```

### Passo 2 - Posicionamento fisico

```
   sala 1            corredor          sala 2
   [NODE_A]----------[NODE_B]----------[NODE_C]
   (origem)          (relay)           (destino)
```

`NODE_A` e `NODE_C` devem ficar **fora do alcance direto** (parede entre eles, ou afastamento de 8-15m). `NODE_B` no meio.

### Passo 3 - Monitor

Dashboard com 3 portas:

```bash
python3 firmware/tools/live_monitor.py \
    --port /dev/cu.usbserial-2110:NODE_A \
    --port /dev/cu.usbserial-2120:NODE_B \
    --port /dev/cu.usbserial-2130:NODE_C
```

Ou texto, em 3 terminais (use o wrapper):

```bash
zsh firmware/tests/tc002/monitor_log.sh node_a /dev/cu.usbserial-2110 tc002_run
zsh firmware/tests/tc002/monitor_log.sh node_b /dev/cu.usbserial-2120 tc002_run
zsh firmware/tests/tc002/monitor_log.sh node_c /dev/cu.usbserial-2130 tc002_run
```

### Passo 4 - Avaliar PASS

Aguarde 60-120s.

`NODE_A` deve mostrar:

```
route[0] dest=<MAC_C> via=<MAC_B> hops=2 metric=2 state=2 expires=...
ACK received from <MAC_C> for seq=N
```

`NODE_C` deve mostrar:

```
DATA deliver from <MAC_A>: tc002 multihop
```

Criterio formal em [tests/tc-002-primeiro-multi-hop.md](tests/tc-002-primeiro-multi-hop.md).

---

## 9. TC-003 - reconvergencia

**Objetivo**: validar recuperacao apos falha de enlace intermediario.

**Setup identico ao TC-002** (mesmos perfis `firmware/tests/tc002/*.defaults`).

### Procedimento

1. Inicie o TC-002 normalmente (passos 1-3 da secao 8).
2. Aguarde 30s ate ver `ACK` chegando regularmente em `NODE_A`.
3. **Provoque a falha**: tire `NODE_B` da fonte de energia (USB) **OU** afaste fisicamente para perder o enlace `B<->C`.
4. Observe que `NODE_A` deve passar a logar `DATA queued while route discovery is in progress`, `ESP-NOW send fail`, e eventualmente `invalidated N route(s) via <MAC_B>`.
5. **Recupere**: religue ou aproxime `NODE_B`.
6. Aguarde nova descoberta. `NODE_A` deve voltar a logar `ACK received` em ate 30s apos a recuperacao.

Criterio formal em [tests/tc-003-reconvergencia-apos-falha.md](tests/tc-003-reconvergencia-apos-falha.md).

---

## 10. TC-004 - soak de 30 minutos

**Objetivo**: estabilidade sob ciclos repetidos de degradacao/recuperacao.

**Setup identico ao TC-002**.

### Procedimento

Use `firmware/monitor_log.sh` para capturar logs longos:

```bash
# em 3 terminais, simultaneamente:
zsh firmware/monitor_log.sh -p /dev/cu.usbserial-2110 -B build/tc002_node_a -t tc004_soak -l node_a
zsh firmware/monitor_log.sh -p /dev/cu.usbserial-2120 -B build/tc002_node_b -t tc004_soak -l node_b
zsh firmware/monitor_log.sh -p /dev/cu.usbserial-2130 -B build/tc002_node_c -t tc004_soak -l node_c
```

Cronograma sugerido (30 min):

| Tempo | Acao |
|---|---|
| 0-3 min | flash + boot |
| 3-8 min | baseline estavel |
| 8-10 min | ciclo 1: degradar (afastar B) |
| 10-14 min | ciclo 1: recuperar |
| 14-16 min | ciclo 2: degradar |
| 16-20 min | ciclo 2: recuperar |
| 20-22 min | ciclo 3: degradar |
| 22-26 min | ciclo 3: recuperar |
| 26-30 min | observacao final |

Criterio formal em [tests/tc-004-soak-estabilidade-e-reconvergencia.md](tests/tc-004-soak-estabilidade-e-reconvergencia.md).

---

## 11. TC-005 - cadeia de 4 nos

**Objetivo**: 3 saltos `A-B-C-D`. Estende `TC-002` para validar a malha alem de 2 hops.

**Hardware**: 4 ESP32 (voce esta com 3 hoje, entao este caso fica para quando o quarto chegar).

### Passo 1 - Flash dos 4 nos

```bash
# destino primeiro
zsh firmware/tests/tc005/build_flash.sh node_d /dev/cu.usbserial-XXXD

# intermediarios
zsh firmware/tests/tc005/build_flash.sh node_c /dev/cu.usbserial-XXXC
zsh firmware/tests/tc005/build_flash.sh node_b /dev/cu.usbserial-XXXB

# origem com MAC do NODE_D
zsh firmware/tests/tc005/build_flash.sh node_a /dev/cu.usbserial-XXXA <MAC_D>
```

### Passo 2 - Posicionamento

```
[A]---d1---[B]---d2---[C]---d3---[D]
```

`A` nao deve alcancar `C` nem `D` diretamente. `B` nao deve alcancar `D` diretamente.

### Passo 3 - Monitor

```bash
python3 firmware/tools/live_monitor.py \
    --port /dev/cu.usbserial-XXXA:NODE_A \
    --port /dev/cu.usbserial-XXXB:NODE_B \
    --port /dev/cu.usbserial-XXXC:NODE_C \
    --port /dev/cu.usbserial-XXXD:NODE_D
```

### Passo 4 - PASS

`NODE_A`:
```
route[0] dest=<MAC_D> via=<MAC_B> hops=3 metric=3 state=2
ACK received from <MAC_D> for seq=N
```

`NODE_D`:
```
DATA deliver from <MAC_A>: tc005 chain4
```

Criterio formal em [tests/tc-005-cadeia-4-nos.md](tests/tc-005-cadeia-4-nos.md).

---

## 12. Monitor: dashboard vs texto

Voce **so pode usar um deles por porta**. Eles competem pelo mesmo `/dev/cu.usbserial-XXXX`.

| Caracteristica | `live_monitor.py` (dashboard) | `idf.py monitor` / `monitor_log.sh` (texto) |
|---|---|---|
| Visualizacao | grafo animado, painel, eventos | linhas de log brutas |
| Captura em arquivo | nao automatica | sim em `firmware/logs/serial/` |
| Ideal para | demo, defesa, ver topologia | extracao de metricas, soak test |
| Resets/panic | mostra mas nao detalha | mostra stack trace completo |
| Multi-no | sim, todos no mesmo browser | um terminal por no |

### Quando usar dashboard

- Demonstracao para banca
- Sanity check de "esta funcionando?"
- Visualizar fluxo de `DATA` e `ACK`
- Detectar quebra de link visualmente

### Quando usar texto + captura

- Coletar dados para analise (`extract_monitor_metrics.py`)
- Soak test de 30+ minutos
- Debug de panic/guru meditation
- Validacao formal dos criterios de PASS

### Como rodar dashboard com hardware real

```bash
cd /Users/huaksonlima/Documents/tcc/aodv-en
python3 firmware/tools/live_monitor.py \
    --port /dev/cu.usbserial-2110:NODE_A \
    --port /dev/cu.usbserial-2120:NODE_B \
    --port /dev/cu.usbserial-2130:NODE_C
# abrir http://localhost:8765/
```

Encerrar com Ctrl+C no terminal do `live_monitor.py`.

### Como rodar dashboard sem hardware (demo)

```bash
python3 firmware/tools/live_monitor.py --demo
```

Util para mostrar a interface antes de plugar os ESPs.

---

## 13. Analise pos-execucao

Apos uma sessao com captura de log via `monitor_log.sh`, os arquivos estao em `firmware/logs/serial/`.

### 13.1 Extrair metricas

Para cada arquivo `.log`:

```bash
python3 firmware/tools/extract_monitor_metrics.py firmware/logs/serial/node_a_tc002_run_20260501-101533.log
```

Saida em `firmware/logs/analysis/<basename>/`:
- `summary.json` e `summary.txt`
- CSVs: `events`, `snapshots`, `routes`, `target_route_series`, `ack_events`, `discovery_windows`, `minute_metrics`...

### 13.2 Gerar graficos

```bash
python3 firmware/tools/plot_monitor_metrics.py firmware/logs/analysis/node_a_tc002_run_20260501-101533
```

Gera 7 PNGs em `plots/`: ACK vs Fail por minuto, fila por minuto, timeline da rota, distribuicao de hops, etc.

### 13.3 Comparar runs (uso tipico TC-004)

```bash
python3 firmware/tools/plot_comparison_metrics.py \
    --run baseline::firmware/logs/analysis/node_a_run1/summary.json \
    --run pos-fix::firmware/logs/analysis/node_a_run2/summary.json \
    --change-index 1
```

### 13.4 Desenhar topologia observada

```bash
python3 firmware/tools/draw_topology.py \
    firmware/logs/analysis/node_a_tc002_run_20260501-101533 \
    firmware/logs/analysis/node_b_tc002_run_20260501-101533 \
    firmware/logs/analysis/node_c_tc002_run_20260501-101533 \
    --mode latest
```

Gera Mermaid + DOT + SVG (se `dot` instalado) em `topology/`.

---

## 14. Troubleshooting

### "command not found: idf.py" depois de source

Voce sourceou em uma sub-shell. Sourceie de novo no terminal atual:

```bash
source /tmp/aodv-en-idf-env.sh
```

### "CMakeLists.txt not found in project directory"

Voce esta chamando `idf.py` de fora da pasta `firmware/`. Os scripts `tc00X/build_flash.sh` ja fazem `cd "$FW_DIR"` antes de invocar `idf.py`. Se voce estiver chamando `idf.py` direto, garanta:

```bash
cd /Users/huaksonlima/Documents/tcc/aodv-en/firmware
idf.py -B build/algo build
```

### "ModuleNotFoundError: No module named 'aiohttp'" no live_monitor

Quando o `idf-env.sh` esta sourceado, `python3` aponta para a venv do ESP-IDF, nao para o python do `pyenv`. Instale `aiohttp` direto na venv do IDF:

```bash
/Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env/bin/pip install aiohttp pyserial
```

### "Permission denied" na porta serial

Outro processo esta usando a porta. Verifique se `live_monitor.py` ou `idf.py monitor` ainda esta aberto em outro terminal:

```bash
lsof /dev/cu.usbserial-2110
```

Mate o processo culpado ou feche o terminal.

### "Failed to connect to ESP32: No serial data received"

- Verifique cabos USB (alguns sao "charge only" e nao tem dados)
- Pressione e segure o botao `BOOT` do ESP32 enquanto inicia o flash
- Tente um baud menor: `--baud 115200` em vez do `460800` default

### Build falha com `aodv_en_node.h: No such file`

Cache de build velho referenciando os stubs deletados em uma versao anterior. Solucao:

```bash
rm -rf firmware/build firmware/build/tc00*_*
zsh firmware/build.sh
```

### `routes=0 neighbors=0` por mais de 30s

Os nos nao estao se vendo. Possiveis causas:

- canal Wi-Fi diferente entre os nos: confira `CONFIG_AODV_EN_APP_WIFI_CHANNEL` em cada `.defaults`
- `network_id` diferente: idem
- distancia fisica grande ou obstaculo metalico
- `app_proto_example` vs `app_demo`: confira que os 2 estao no mesmo modo

### "ESP-NOW send fail" toda hora

- Vizinho fora de alcance ou desligado
- Buffer do ESP-NOW lotado: investigue se ha nos com `tx` muito alto e fica reciclando peers do cache
- Interferencia (usar canal diferente do Wi-Fi de casa, idealmente canal 1 ou 11 limpo)

### `DATA send status=-2` (= `AODV_EN_ERR_FULL`)

Fila pendente de `DATA` cheia. Significa que o no esta tentando enviar mais rapido do que a malha consegue descobrir rotas. Comportamento esperado durante quebras prolongadas. Para o TCC, contar como evidencia de que o backpressure funcionou.

### Reset constante / guru meditation

Use `idf.py monitor` (nao o dashboard) para ver o stack trace completo. Ferramenta `idf.py monitor` decodifica enderecos automaticamente.

### Esquecimento - "qual MAC era qual no?"

Releia direto via `esptool` na porta:

```bash
ESPTOOL=/Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env/bin/esptool
"$ESPTOOL" --port /dev/cu.usbserial-XXXX read_mac
```

Ou olhe a primeira linha de log do no apos boot - tem `node=NODE_X self_mac=...`.

---

## 15. Cheatsheet de comandos

```bash
# === SETUP ===
cd /Users/huaksonlima/Documents/tcc/aodv-en
source /tmp/aodv-en-idf-env.sh

# === DESCOBRIR HARDWARE ===
ls /dev/cu.usbserial-*
ESPTOOL=/Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env/bin/esptool
"$ESPTOOL" --port /dev/cu.usbserial-XXXX read_mac

# === BUILD LIMPO ===
rm -rf firmware/build && zsh firmware/build.sh

# === FLASH POR TC ===
# TC-001 (2 nos)
zsh firmware/tests/tc001/build_flash.sh node_b /dev/cu.usbserial-2110
zsh firmware/tests/tc001/build_flash.sh node_a /dev/cu.usbserial-2120 <MAC_B>

# TC-002/003/004 (3 nos)
zsh firmware/tests/tc002/build_flash.sh node_c /dev/cu.usbserial-2130
zsh firmware/tests/tc002/build_flash.sh node_b /dev/cu.usbserial-2120
zsh firmware/tests/tc002/build_flash.sh node_a /dev/cu.usbserial-2110 <MAC_C>

# TC-005 (4 nos)
zsh firmware/tests/tc005/build_flash.sh node_d /dev/cu.usbserial-XXXD
zsh firmware/tests/tc005/build_flash.sh node_c /dev/cu.usbserial-XXXC
zsh firmware/tests/tc005/build_flash.sh node_b /dev/cu.usbserial-XXXB
zsh firmware/tests/tc005/build_flash.sh node_a /dev/cu.usbserial-XXXA <MAC_D>

# === MONITOR DASHBOARD ===
python3 firmware/tools/live_monitor.py \
    --port /dev/cu.usbserial-2110:NODE_A \
    --port /dev/cu.usbserial-2120:NODE_B \
    --port /dev/cu.usbserial-2130:NODE_C
# http://localhost:8765/

# === MONITOR TEXTO + LOG ===
zsh firmware/monitor_log.sh -p /dev/cu.usbserial-XXXX -B build/tc002_node_a -t run01 -l node_a

# === ANALISE ===
python3 firmware/tools/extract_monitor_metrics.py firmware/logs/serial/<arquivo>.log
python3 firmware/tools/plot_monitor_metrics.py firmware/logs/analysis/<basename>
python3 firmware/tools/draw_topology.py firmware/logs/analysis/<basename> --mode latest

# === LIMPEZA ===
rm -rf firmware/build firmware/logs/serial firmware/logs/analysis
```

---

## Anexo - mapping rapido sessao 2026-05-01

3 ESPs detectados nesta sessao:

| Porta | MAC | Sugestao de papel |
|---|---|---|
| `/dev/cu.usbserial-2110` | `28:05:A5:34:03:50` | NODE_A (origem) |
| `/dev/cu.usbserial-2120` | `28:05:A5:33:B9:EC` | NODE_B (relay) |
| `/dev/cu.usbserial-2130` | `28:05:A5:34:05:64` | NODE_C (destino) |

Comandos prontos para colar no terminal:

```bash
cd /Users/huaksonlima/Documents/tcc/aodv-en
source /tmp/aodv-en-idf-env.sh

# TC-002 completo - copia e cola:
zsh firmware/tests/tc002/build_flash.sh node_c /dev/cu.usbserial-2130
zsh firmware/tests/tc002/build_flash.sh node_b /dev/cu.usbserial-2120
zsh firmware/tests/tc002/build_flash.sh node_a /dev/cu.usbserial-2110 28:05:A5:34:05:64

# Dashboard:
python3 firmware/tools/live_monitor.py \
    --port /dev/cu.usbserial-2110:NODE_A \
    --port /dev/cu.usbserial-2120:NODE_B \
    --port /dev/cu.usbserial-2130:NODE_C
```
