# Plano — Campanha de 30 repetições, 10 ESP32, métricas reais

> Objetivo: cumprir a promessa metodológica do TCC (Quadro 9: 30 repetições, cenários C1–C4,
> média/desvio/**IC95**), com **medições reais** — latência real (RTT/2 na origem, laço fino),
> multi-hop físico verificado por RSSI, baixa interferência caracterizada.
> Branch: `fix/tcc-revisao-escrita`.

## Inventário dos nós (etiquetados, identidade por MAC — porta serial re-enumera)

| Nome | MAC | Papel típico |
|---|---|---|
| **N1** | `28:05:A5:33:EB:80` | origem/ponta |
| **N2** | `28:05:A5:33:D6:1C` | relay/meio |
| **N3** | `28:05:A5:34:99:34` | relay/meio |
| **N4** | `28:05:A5:33:B9:EC` | destino/ponta (cadeia 3 saltos A->B->C->D) |

(Os 10 ESPs finais serão enumerados do mesmo modo. Identificar sempre por MAC.)

### Topologia C1 (cadeia) CONFIRMADA por RSSI (TX=2 dBm)

Medição real com `rssi_probe.py` (origem N1=EB80 na serial, N2/N3 espalhados):

| Enlace | RSSI médio | recepções | estado |
|---|---|---|---|
| N1 ↔ N2 (~5 m) | −75 dBm | 30/35 | **firme** ✅ |
| N1 ↔ N3 (pontas, ~10 m) | −88,5 dBm | **4/35** | **quebrado** (89% perda) |
| N2 ↔ N3 | −74 dBm | 29/30 | firme ✅ |

→ As pontas (N1, N3) **não se ouvem**; N3 só alcança N1 via N2 = **cadeia A→B→C, 2 saltos
reais**, multi-hop nativo por atenuação física (sem allowlist). É o C1 do TCC.

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
- [x] **F0.2** Flood unicast → **broadcast** (baseline canônico): `app_flood.c` `app_emit_frame`
  passa a emitir 1 broadcast por (re)transmissão (decisão registrada anteriormente).
- [x] **F0.3** **Telemetria in-band de stats** (arquitetura decidida via workflow — Abordagem A,
  **zero alteração de núcleo**): ao fim da janela, cada nó não-origem envia seus contadores como
  **DATA de aplicação normal** destinada ao MAC da origem (reusa roteamento existente — provado
  por `aodv_en_node_send_ack`/`flood_en_send_ack`). Payload = magic 0x53 + 4×uint32 htonl
  (tx/rx/control/delivered). A origem desserializa em `app_deliver_data` e loga
  `STATSREP node=<mac> tx= rx= control= delivered=`. Parser `tcc_metrics.py` agrega por STATSREP
  (fallback `--node` preservado). **3 mitigações de contaminação** (obrigatórias): (1) snapshot
  dos contadores ANTES do envio; (2) envio só na drenagem pós-corte; (3) `ack_required=false`.
  Toca: `app_demo.c`, `app_flood.c`, `Kconfig.projbuild` (REPORT_TO_MAC), `tcc_metrics.py`.
- [x] **F0.4** **Controle de TX power** por config (`esp_wifi_set_max_tx_power`), p/ encurtar
  alcance e viabilizar saltos num espaço menor. Exposto em Kconfig.
- [~] **F0.5** (OPCIONAL, nao bloqueia) **Scan de canal** (script): varre os 14 canais 2.4 GHz, reporta ocupação/RSSI,
  sugere o mais limpo. Registra piso de ruído p/ documentação.
- [x] **F0.6** **Harness de campanha** (script): build-por-nó (NODE_NAME/TARGET_MAC/topologia/
  TX power), flash dos N nós, 30 reps consecutivas/cenário, captura na origem,
  agregação → `tcc_metrics.py` → ledger com média/desvio/**IC95**.
- [x] **F0.7** **Validar em simulação** (já tem seed RNG real em `sim/campaign_compare.c`):
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
