# Simulacao Local do AODV-EN

Esta pasta contem uma simulacao minima do nucleo do protocolo sem ESP32 real.

## Objetivo

Validar localmente o fluxo:

- `RREQ`
- `RREP`
- `DATA`
- `ACK`

em uma topologia linear de 3 nos:

- `A <-> B <-> C`

## Como rodar

```bash
cd /Users/huaksonlima/Documents/tcc/aodv-en
bash sim/run_sim.sh
```

## O que esperar

- na primeira tentativa de envio, `A` nao tem rota para `C`
- `A` emite `RREQ`
- `B` encaminha
- `C` responde com `RREP`
- a rota `A -> C via B` e instalada
- na segunda tentativa, `DATA` chega em `C`
- `C` responde com `ACK`
- `A` registra a confirmacao
