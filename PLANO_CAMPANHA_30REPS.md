# Plano — Campanha de 30 repetições, 10 ESP32, métricas reais

> Objetivo: cumprir a promessa metodológica do TCC (Quadro 9: 30 repetições, cenários C1–C4,
> média/desvio/**IC95**), com **medições reais** — latência real (RTT/2 na origem, laço fino),
> multi-hop físico verificado por RSSI, baixa interferência caracterizada.
> Branch: `fix/tcc-revisao-escrita`.

## Decisões tomadas (usuário)

- **Topologia**: espalhar os 10 ESPs fisicamente (saltos reais), **não** allowlist por software.
- **Laço da app**: reduzir de 100 ms → **10 ms** (latência fina, real).
- **Escopo**: C1–C4 em hardware, 30 reps × 2 algoritmos.
- **Coleta de stats dos nós distantes**: **telemetria in-band** (cada nó reporta contadores
  pela própria malha até a origem). Só a **origem** precisa de serial.
- **Infra de energia/porta**: computador (origem) + tomada 2 portas + hub 3 portas + 1 avulso;
  resto na serial. Nós distantes no powerbank/tomada.

## Realidade física (inegociável)

- ESP-NOW alcança 100 m+ em visada. Com os nós próximos, **todos se ouvem a 1 salto** (estrela).
- Para multi-hop real: **separação física** (cômodos/andares) + **redução de potência TX**
  (encurta alcance) + **verificação de topologia por RSSI** antes de cada campanha.
- 2.4 GHz nunca é "zero interferência". O honesto é **baixa interferência caracterizada**:
  scan de canal + registro do piso de ruído/RSSI.

## De onde vem cada métrica (define quem precisa de serial)

| Métrica | Fonte | Quem loga |
|---|---|---|
| PDR | acks/enviados na origem | **só origem** |
| Latência | RTT/2 na origem (mesmo clock `esp_timer`, µs) | **só origem** |
| NRL | controle_tx/entregue (rede) | todos → via telemetria in-band |
| Energia | Σ(N_tx·E_tx + N_rx·E_rx + idle) (rede) | todos → via telemetria in-band |

---

## Fase 0 — firmware + harness (SEM ESPs; em andamento)

- [x] **F0.1** Laço da app 100 ms → 10 ms (`APP_LOOP_DELAY_MS` em `app_demo.c:33` e equivalente
  no `app_flood.c`). Latência deixa de ser quantizada em 100 ms.
- [ ] **F0.2** Flood unicast → **broadcast** (baseline canônico): `app_flood.c` `app_emit_frame`
  passa a emitir 1 broadcast por (re)transmissão (decisão registrada anteriormente).
- [ ] **F0.3** **Telemetria in-band de stats**: nova mensagem que, ao fim de cada run, carrega os
  contadores (tx/rx/control/delivered) de cada nó até a origem; a origem loga linha
  `STATSREP node= tx= rx= control= delivered=`. Núcleo + adaptador.
- [x] **F0.4** **Controle de TX power** por config (`esp_wifi_set_max_tx_power`), p/ encurtar
  alcance e viabilizar saltos num espaço menor. Exposto em Kconfig.
- [ ] **F0.5** **Scan de canal** (script): varre os 14 canais 2.4 GHz, reporta ocupação/RSSI,
  sugere o mais limpo. Registra piso de ruído p/ documentação.
- [ ] **F0.6** **Harness de campanha** (script): build-por-nó (NODE_NAME/TARGET_MAC/topologia/
  TX power), flash dos N nós, 30 reps consecutivas/cenário, captura na origem,
  agregação → `tcc_metrics.py` → ledger com média/desvio/**IC95**.
- [ ] **F0.7** **Validar em simulação** (já tem seed RNG real em `sim/campaign_compare.c`):
  rodar C1–C4 × 30 reps em sim, conferir que o pipeline de métricas/IC95 fecha, antes do HW.

## Fase 1 — coleta em hardware (quando usuário espalhar + conectar a origem)

- [ ] **F1.1** Ler os 10 MACs (esptool read-mac) → montar matriz de identidade dos nós.
- [ ] **F1.2** Scan de canal → escolher o mais limpo → registrar.
- [ ] **F1.3** Posicionar os 10 ESPs no layout do cenário; **verificar topologia por RSSI**
  (`live_monitor`) — confirmar que cada nó só ouve os vizinhos previstos. Registrar
  distância + RSSI por enlace (geometria documentada).
- [ ] **F1.4** Para cada cenário C1–C4 e cada algoritmo (AODV-EN, flooding):
  gerar configs → flashar os 10 → rodar **30 reps** → capturar origem + telemetria stats.
- [ ] **F1.5** Agregar → `tcc_metrics.py` (média/desvio/IC95) → ledger → figuras.
- [ ] **F1.6** Fechar itens bloqueados do TCC: objetivo (e) comparação literatura, IC95 no Cap.5,
  conclusão item-a-item, números reais no resumo/abstract/conclusão.

## Esforço estimado (HW)

- 4 cenários × 2 algoritmos = 8 combinações; cada combinação exige reposicionar/reflashar.
- 30 reps × 8 = **240 execuções**. Com run de ~30–60 s + telemetria, é **muitas horas de bancada**.
- Trocar de cenário = re-layout físico dos 10 nós + reflash (TARGET/topologia/TX power).

## Riscos e mitigação

- **Topologia não-determinística** (alcance varia com posição/pessoas/interferência): mitigar com
  TX power baixo + verificação RSSI antes de cada bloco de reps + registro da geometria.
- **Nós distantes sem serial**: telemetria in-band resolve o log; energia por powerbank.
- **Interferência 2.4 GHz**: scan + canal limpo + horário quieto + caracterização do ruído.
- **Reprodutibilidade HW**: "seed" em HW é rótulo de repetição (não há RNG); a variabilidade real
  vem do meio. Documentar como tal; o determinismo de seed fica na simulação.
