# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Projeto

TCC (IFG Inhumas): `AODV-EN`, adaptacao do AODV (RFC 3561) para mesh multi-hop sobre ESP-NOW v2 e ESP32, comparado a um baseline de flooding controlado (`flood_en`). Spec ativa: `docs/aodv-en-spec-v1.md` (`spec-v0` e OBSOLETA). Runbook de bancada copy-paste: `docs/runbook-bancada.md`. Mapa feature -> arquivo: `docs/aodv-en-mapa-do-codigo.md`.

## Comandos

### Ambiente ESP-IDF (v6.0)

```sh
export ESP_IDF_EXPORT=/Users/huaksonlima/.espressif/v6.0/esp-idf/export.sh
source firmware/idf-env.sh
idf.py --version   # esperado: ESP-IDF v6.0
```

Sem `ESP_IDF_EXPORT`, o fallback `$HOME/esp/esp-idf` nao existe nesta maquina e o source falha. O script exporta `IDF_COMPONENT_MANAGER=0` (componentes so locais) e tem guard `AODV_EN_IDF_ENV_LOADED=1` (shell novo para trocar de IDF). Python das tools: `IDFPY=/Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env/bin/python` (venv ja tem aiohttp, pyserial, matplotlib, markdown — instalar deps com o pip desse venv, nunca `pip --user`).

### Simulacao (valida o core sem hardware — o mais proximo de "rodar testes")

```sh
bash sim/run_sim.sh            # basic: 3 nos, RREQ->RREP->DATA->ACK, termina em "Simulation passed."
bash sim/run_sim.sh large      # 6 nos, propagacao de RERR
bash sim/run_sim.sh 100        # grid 10x10 com parede
bash sim/run_sim.sh 1000       # grid 32x32, lento
bash sim/run_sim.sh flood      # baseline flooding
bash sim/run_sim.sh compare > results/m-bench-sim-compare.csv   # sweep AODV vs flood, CSV no stdout
```

Compila com `cc` do host (sem ESP-IDF), binarios em `/tmp`. `basic`/`large`/`flood` validam por `assert()` — nunca compilar com `-DNDEBUG`. `100`/`1000` validam por strings SUCCESS/FAILED no output, nao por exit code.

### Firmware: build, flash, monitor

```sh
zsh firmware/build.sh                                  # build default em firmware/build/
zsh firmware/flash_monitor.sh /dev/cu.usbserial-XXXX   # flash + monitor (sair: Ctrl-])
```

Perfis por caso de teste (build dirs isolados, ex. `build/tc002_node_a`):

```sh
# ordem: destino primeiro, origem (node_a) por ultimo — node_a precisa do MAC do DESTINO
zsh firmware/tests/tc002/build_flash.sh node_c /dev/cu.usbserial-2130
zsh firmware/tests/tc002/build_flash.sh node_b /dev/cu.usbserial-2120
zsh firmware/tests/tc002/build_flash.sh node_a /dev/cu.usbserial-2110 28:05:A5:34:05:64
```

`tc001` = 2 nos, `tc002` = 3 nos (cobre TC-002/003/004), `tc005` = 4 nos. Flood: `zsh firmware/tests/flood/build_flash.sh <port> <TARGET_MAC>` — sem role, mesma imagem em todos os nos. Todo `build_flash.sh` faz `erase-flash` antes do flash (apaga NVS, lento por design). Scripts sao zsh-specific — invocar com `zsh`.

Ler MAC de uma placa (antes de flashar node_a):

```sh
ls /dev/cu.usbserial-*   # numeros mudam a cada reconexao USB — sempre re-listar
/Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env/bin/esptool --port /dev/cu.usbserial-XXXX --before default_reset --after hard_reset read_mac 2>&1 | grep 'MAC:'
```

Monitor com captura de log (entrada das ferramentas de analise):

```sh
zsh firmware/monitor_log.sh -p <port> -B build/tc002_node_a -t run01 -l node_a   # -> firmware/logs/serial/
```

### Tools e analise

```sh
python3 firmware/tools/live_monitor.py --demo                          # dashboard http://localhost:8765/ sem hardware
$IDFPY firmware/tools/live_monitor.py --port /dev/cu.usbserial-2110:N1 --port ...:N2   # hardware real
$IDFPY firmware/tools/live_monitor.py --replay results/m11-flood-bcast-N1.log:N1 --replay ...:N2 --replay-speed 4   # reproduz logs capturados (aodv ou flood); log sem banner node= exige ARQUIVO:ALIAS:MAC
python3 firmware/tools/extract_monitor_metrics.py firmware/logs/serial/<f>.log         # -> firmware/logs/analysis/<f>/
python3 firmware/tools/draw_topology.py firmware/logs/analysis/<f> --mode latest       # Mermaid/DOT/SVG
python3 firmware/tools/tcc_metrics.py --algo aodv-en --origin N2.log --node N1.log --node N2.log --node N3.log --duration-s 48   # JSON com PDR/latencia/NRL/energia
$IDFPY firmware/tools/plot_graphs_unit.py     # regenera graphs/*.png a partir de results/
$IDFPY firmware/tools/plot_tcc_figures.py     # regenera docs/img/tcc/*.png
python3 docs/build_pdf.py                     # PDF completo (precisa markdown lib + Chrome/Brave)
$IDFPY docs/build_tcc_pdf.py                  # PDF do TCC
```

`live_monitor.py` roda `esptool read_mac` no startup (reseta as placas) salvo `--skip-mac-lookup`. So um processo por porta serial — dashboard e `idf.py monitor` competem. `build_pdf.py`, `build_tcc_pdf.py` e `plot_graphs_unit.py` tem `ROOT` hardcoded para este path.

## Arquitetura

### Um core, dois consumidores

`firmware/components/aodv_en/` e C portavel sem dependencia de ESP-IDF, consumido por dois caminhos distintos:

- **Firmware ESP32**: usa a fachada publica `aodv_en.h` (`aodv_en_stack_*`, handle opaco) injetando `aodv_en_adapter_t { now_ms, tx_frame }` (ligado a ESP-NOW) + `aodv_en_app_callbacks_t { on_data, on_ack }`.
- **Sims (`sim/*.c`)**: `run_sim.sh` compila os MESMOS `src/*.c` do componente direto com `cc` e usa a camada interna `aodv_en_node.h` (`aodv_en_node_t` concreto, `aodv_en_node_callbacks_t`), com radio mock em memoria e relogio virtual. **Mudar layout de `aodv_en_node_t` ou API dos sub-modulos quebra os sims mesmo com `aodv_en.h` intacto.**

Camadas (dependencia estritamente descendente): `aodv_en.h` (fachada, unico calloc do componente) -> `aodv_en_node.c` (toda a logica AODV: RREQ/RREP/RERR/HELLO/DATA/ACK, fila de pendentes, precursores, ~1700 linhas) -> sub-modulos `neighbors`/`routes`/`rreq_cache`/`peers`/`mac` -> headers de dados `messages`/`types`/`tables`/`limits`/`status`.

### Invariantes do core

- **Tempo injetado**: zero leitura de relogio/timers no core; todo entry point recebe `now_ms` (`uint32_t`, monotone). Variantes `_at` da stack recebem tempo explicito.
- **Sem heap no caminho critico**: tabelas/filas sao arrays fixos dimensionados pelos macros de `aodv_en_limits.h` (override via `#ifndef`/`-D`). Os campos de tamanho em `aodv_en_config_t` so ENCOLHEM a capacidade — para sims grandes, passar `-DAODV_EN_ROUTE_TABLE_SIZE=N` no cc.
- **Sem locks**: single-threaded por contrato; o app ESP32 serializa tudo num task loop.
- **Status codes**: negativos = erro; nao-negativos = sucesso (`OK` 0, `NOOP` 1, `QUEUED` 2 — send sem rota enfileira e dispara RREQ).
- `aodv_en_stats_t` (node) e `aodv_en_stack_stats_t` (publica) sao duplicatas intencionais copiadas campo a campo em `src/aodv_en.c` — adicionar estatistica toca 3 lugares.

### flood_en (baseline)

`firmware/components/flood_en/` e componente INDEPENDENTE — zero includes de `aodv_en`, wire format proprio com header byte-identico ao `aodv_en_header_t` de proposito (manter em sincronia deliberadamente). Sem roteamento: TTL + dedupe por (origem, seq). Mesma injecao de transporte/tempo.

### Selecao de app por Kconfig

`firmware/main/Kconfig.projbuild`, choice `AODV_EN_APP_EXAMPLE_MODE`: `APP_DEMO` (default, HELLO/DATA periodicos), `PROTO_EXAMPLE` (HEALTH/TEXT/CMD + CLI serial — e o ramo fallthrough do `#if` em `main.c`), `APP_FLOOD` (baseline). Os tres `.c` compilam sempre; o dispatch e `#if` em `main.c`. Perfis de teste compoem `SDKCONFIG_DEFAULTS` (`sdkconfig.defaults` + `<role>.defaults` + overrides temporarios com o TARGET_MAC) e isolam `SDKCONFIG` dentro do build dir por role. Atencao: `firmware/build.sh` usa o `firmware/sdkconfig` committed — apagar para regenerar dos defaults; os scripts por teste nao tem esse problema.

## Convencoes

- **Commits**: Conventional Commits em portugues ASCII (sem acentos), ex. `feat(aodv):`, `feat(flood):`, `fix(metrics):`, `test(hw):`, `docs(monitor):`.
- **Docs**: portugues com acentos removidos — manter o estilo ao editar.
- **Resultados**: `results/experiments-ledger.json` e o ledger de experimentos; numeros de energia sao ESTIMATIVAS de datasheet, manter rotulados como tal. Contadores do firmware sao cumulativos — capturar metricas logo apos boot limpo (reflash + reset), senao NRL/energia inflam.
- Diagnostico comum: `routes=0 neighbors=0` por >30s = mismatch de canal Wi-Fi/network_id entre `.defaults` ou mismatch de modo de app.
