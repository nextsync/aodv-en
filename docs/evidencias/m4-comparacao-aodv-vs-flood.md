# M4 — AODV-EN vs Flooding controlado (mesma topologia, hardware)

Data: 2026-05-30 (sessao coleta automatizada)
Branch: `coleta-2026-05-30`

## Cenario

Mesmos 3 ESP32, mesmo hub (todos a 1 hop entre si), mesmo fluxo:
dois origens (N2 `33:EB:80`, N3 `33:D6:1C`) enviam DATA periodico (a cada 2 s,
`ack_required=true`) para o mesmo destino N1 (`34:99:34`). NETWORK_ID e canal
identicos. Cada modo gravado nos 3 nos e capturado ~16 s de serial.

- AODV-EN: `app_demo` (HELLO 1 Hz + RREQ/RREP + DATA unicast + ACK).
- Flooding: `app_flood` (broadcast com TTL + dedup (origem,seq), sem HELLO/rotas).

Logs crus: `m4-captura-aodv-hub.log`, `m4-captura-flood-hub.log`.

## Resultado (deltas normalizados para 6 entregas no destino)

| Metrica                         | Flooding | AODV-EN |
|---------------------------------|----------|---------|
| Entregas no destino (N1)        | 6        | 6       |
| TX total (3 nos)                | 24       | 32      |
| RX total (3 nos)                | 47       | 48      |
| **TX por entrega**              | **4.00** | **5.33**|
| Duplicatas suprimidas (dedup)   | 23       | n/a     |
| Rebroadcast no destino          | 0        | n/a     |

## Leitura

No **hub de 1 hop**, o flooding controlado teve custo por entrega MENOR que o
AODV-EN. Motivo: o AODV-EN paga overhead de controle constante (HELLO a 1 Hz para
manutencao de vizinhanca + descoberta), enquanto o flooding so coloca DATA/ACK no
ar. Como a topologia e densa e de 1 hop, o flooding nao se multiplica — a supressao
por (origem,seq) descarta as duplicatas imediatamente (23 dups no intervalo) e o
destino nao reencaminha (rebroadcast=0).

**Onde o AODV-EN vence:** topologias multi-hop e de maior escala. La o flooding
multiplica retransmissoes a cada salto (cada vizinho rebroadcasta uma vez), enquanto
o AODV-EN confina o trafego ao caminho descoberto (unicast salto a salto). O hub de
bancada nao consegue exercitar esse regime (todos a 1 hop). A comparacao em funcao
do diametro/escala e feita por simulacao em m5.

## Caveats

- O overhead do AODV-EN aqui e dominado pelo HELLO_INTERVAL=1000 ms; intervalos
  maiores reduzem o custo de controle do AODV-EN no regime estavel.
- Janelas de captura nao perfeitamente alinhadas no tempo entre os dois modos;
  por isso a metrica comparada e normalizada por entrega (TX/entrega), nao absoluta.
- 1 hop apenas: nao mede o custo multi-hop do flooding (ver m5/sim).
