# Testes de Hardware - Padrao

Este diretorio guarda casos de teste funcionais de bancada para o `aodv-en`.

## Contexto

Os casos de teste deste diretorio sao a evidencia operacional dos criterios de aprovacao definidos em [aodv-en-spec-v1.md](../aodv-en-spec-v1.md), secao "Criterios de aprovacao da v1".

Mapeamento:

| Caso | Criterios da spec v1 | Status |
|---|---|---|
| `TC-001` | 1 (descoberta e entrega direta) | `ATIVO` |
| `TC-002` | 2 (multi-hop em cadeia) e 5 (`duplicate_rreq_drops`) | `ATIVO` (`PASS` em 2026-04-21 com evidencia indireta) |
| `TC-003` | 3 (reconvergencia) e 7 (`RERR` para precursores) | `ATIVO` |
| `TC-004` | 4 (soak 30 min) e 6 (expiracao de rota) | `ATIVO` |
| `TC-005` | cadeia de 4 nos (3 saltos) | `ATIVO` |

## Convencao de nomes

- Arquivo: `tc-XXX-descricao-curta.md`
- Exemplo: `tc-001-descoberta-e-entrega-direta.md`

## Estrutura minima obrigatoria

Cada caso deve conter:

1. `Objetivo`
2. `Topologia`
3. `Configuracao dos nos`
4. `Procedimento`
5. `Evidencias esperadas`
6. `Criterio de aprovacao`
7. `Resultado (preencher a cada execucao)`

## Casos disponiveis

- `TC-001`: [Descoberta e entrega direta](./tc-001-descoberta-e-entrega-direta.md)
  - Script de execucao: `firmware/tests/tc001/build_flash.sh`
- `TC-002`: [Primeiro multi-hop em cadeia](./tc-002-primeiro-multi-hop.md)
  - Script de execucao: `firmware/tests/tc002/build_flash.sh`
  - Captura de log (global): `firmware/monitor_log.sh`
  - Wrapper por papel (TC-002): `firmware/tests/tc002/monitor_log.sh`
- `TC-003`: [Reconvergencia apos falha de enlace](./tc-003-reconvergencia-apos-falha.md)
  - Script de execucao: `firmware/tests/tc002/build_flash.sh` (mesmo setup de 3 nos)
  - Captura de log (global): `firmware/monitor_log.sh`
  - Wrapper por papel (TC-003): `firmware/tests/tc002/monitor_log.sh`
- `TC-004`: [Soak test de estabilidade com reconvergencia ciclica](./tc-004-soak-estabilidade-e-reconvergencia.md)
  - Script de execucao: `firmware/tests/tc002/build_flash.sh` (mesmo setup de 3 nos)
  - Captura de log (global): `firmware/monitor_log.sh`
- `TC-005`: [Cadeia de 4 nos (3 saltos)](./tc-005-cadeia-4-nos.md)
  - Script de execucao: `firmware/tests/tc005/build_flash.sh`
  - Captura de log (global): `firmware/monitor_log.sh`
  - Wrapper por papel (TC-005): `firmware/tests/tc005/monitor_log.sh`

Guia auxiliar: [Guia de leitura dos graficos de monitor](./guia-leitura-graficos-monitor.md).
