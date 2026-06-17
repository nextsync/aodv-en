#!/usr/bin/env python3
"""mesh_monitor.py - monitor realtime da malha AODV-EN a partir de UM coletor.

O coletor (no na serial) recebe, via telemetria in-band, NEIGHREP/STATSREP de TODA a
rede (mesmo nos nao-vizinhos, roteados multi-hop) e ainda loga rotas, RTT e ACKs.
Este monitor le essa unica serial e serve um dashboard web que mostra, ao vivo:
  - grafo de enlaces com RSSI por aresta (cor = forca do sinal)
  - tabela de nos: contadores (tx/rx/control/delivered) via STATSREP
  - rotas ativas do coletor (dest/via/hops/metric) e RTT medido

Uso:
  mesh_monitor.py --port /dev/cu.usbserial-XXXX [--http-port 8090]
  mesh_monitor.py --file results/telemvalid-EB80.log   (replay de log)
"""

import argparse
import json
import re
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

try:
    import serial
except ImportError:
    serial = None

ANSI = re.compile(r"\x1B\[[0-?]*[ -/]*[@-~]")
RE_SELF = re.compile(r"(?:RSSISELF self|self_mac)=([0-9A-Fa-f:]{17})")
RE_NEIGH = re.compile(r"NEIGHREP node=([0-9A-Fa-f:]{17}) hears=(.*)")
RE_PAIR = re.compile(r"([0-9A-Fa-f:]{17}):(-?\d+)")
RE_STATS = re.compile(r"STATSREP node=([0-9A-Fa-f:]{17}) tx=(\d+) rx=(\d+) control=(\d+) delivered=(\d+)")
RE_ROUTE = re.compile(r"route\[\d+\] dest=([0-9A-Fa-f:]{17}) via=([0-9A-Fa-f:]{17}) hops=(\d+) metric=(\d+) state=(\d+)")
RE_ROUTES_N = re.compile(r"routes=(\d+) neighbors=(\d+) tx=(\d+) rx=(\d+) delivered=(\d+) control=(\d+) acks=(\d+)")
RE_LAT = re.compile(r"LAT seq=(\d+) rtt_ms=(\d+)")
RE_ACK = re.compile(r"ACK received from ([0-9A-Fa-f:]{17}) for seq=(\d+)")
RE_PROBE = re.compile(r"RSSIPROBE src=([0-9A-Fa-f:]{17}) rssi=(-?\d+)")

STATE = {
    "collector": None,
    "nodes": {},      # mac -> {tx,rx,control,delivered,last_seen}
    "links": {},      # "a|b" -> {a,b,rssi,last_seen}
    "routes": [],     # [{dest,via,hops,metric}]
    "rtts": [],       # ultimos RTTs
    "collector_stats": {},
    "updated": 0.0,
}
LOCK = threading.Lock()


def ewma(old, new, a=0.4):
    return new if old is None else round(old * (1 - a) + new * a, 1)


def touch_node(mac, now):
    n = STATE["nodes"].setdefault(mac, {"tx": 0, "rx": 0, "control": 0, "delivered": 0, "last_seen": now})
    n["last_seen"] = now
    return n


def set_link(a, b, rssi, now):
    key = "|".join(sorted([a, b]))
    lk = STATE["links"].setdefault(key, {"a": a, "b": b, "rssi": None, "last_seen": now})
    lk["rssi"] = ewma(lk["rssi"], rssi)
    lk["last_seen"] = now


def feed(line, ticks):
    line = ANSI.sub("", line)
    now = ticks()
    with LOCK:
        ms = RE_SELF.search(line)
        if ms and STATE["collector"] is None:
            STATE["collector"] = ms.group(1).upper()

        m = RE_NEIGH.search(line)
        if m:
            node = m.group(1).upper()
            touch_node(node, now)
            for nb, rssi in RE_PAIR.findall(m.group(2)):
                nb = nb.upper()
                touch_node(nb, now)
                set_link(node, nb, int(rssi), now)
            STATE["updated"] = now
            return
        m = RE_STATS.search(line)
        if m:
            mac = m.group(1).upper()
            n = touch_node(mac, now)
            n["tx"], n["rx"], n["control"], n["delivered"] = (int(m.group(i)) for i in range(2, 6))
            STATE["updated"] = now
            return
        m = RE_PROBE.search(line)
        if m and STATE["collector"]:
            src = m.group(1).upper()
            touch_node(src, now)
            set_link(STATE["collector"], src, int(m.group(2)), now)
            STATE["updated"] = now
            return
        rs = RE_ROUTE.findall(line)
        if rs:
            STATE["routes"] = [{"dest": d.upper(), "via": v.upper(), "hops": int(h), "metric": int(me), "state": int(st)}
                               for (d, v, h, me, st) in rs]
            STATE["updated"] = now
        rsm = RE_ROUTES_N.search(line)
        if rsm:
            STATE["collector_stats"] = {
                "routes": int(rsm.group(1)), "neighbors": int(rsm.group(2)),
                "tx": int(rsm.group(3)), "rx": int(rsm.group(4)),
                "delivered": int(rsm.group(5)), "control": int(rsm.group(6)), "acks": int(rsm.group(7)),
            }
            STATE["updated"] = now
        lm = RE_LAT.search(line)
        if lm:
            STATE["rtts"].append(int(lm.group(2)))
            STATE["rtts"] = STATE["rtts"][-30:]
            STATE["updated"] = now


def snapshot():
    with LOCK:
        now = time.monotonic()
        nodes = []
        for mac, n in STATE["nodes"].items():
            nodes.append({
                "mac": mac, "label": mac[-5:],
                "tx": n["tx"], "rx": n["rx"], "control": n["control"], "delivered": n["delivered"],
                "online": (now - n["last_seen"]) < 12,
                "is_collector": mac == STATE["collector"],
            })
        links = []
        for lk in STATE["links"].values():
            if (now - lk["last_seen"]) < 20:
                links.append({"a": lk["a"], "b": lk["b"], "rssi": lk["rssi"]})
        rtts = STATE["rtts"]
        rtt_mean = round(sum(rtts) / len(rtts), 1) if rtts else None
        return {
            "collector": STATE["collector"],
            "nodes": sorted(nodes, key=lambda x: x["label"]),
            "links": links,
            "routes": STATE["routes"],
            "collector_stats": STATE["collector_stats"],
            "rtt_last": rtts[-1] if rtts else None,
            "rtt_mean": rtt_mean,
            "node_count": len(nodes),
        }


HTML = """<!DOCTYPE html><html lang="pt-BR"><head><meta charset="utf-8">
<title>AODV-EN mesh monitor</title>
<style>
 *{box-sizing:border-box} body{margin:0;font-family:-apple-system,Segoe UI,Roboto,sans-serif;background:#0f172a;color:#e2e8f0}
 header{padding:12px 20px;background:#1e293b;border-bottom:1px solid #334155;display:flex;gap:18px;align-items:center}
 header h1{font-size:16px;margin:0;color:#38bdf8}
 .pill{background:#0f172a;border:1px solid #334155;border-radius:14px;padding:3px 11px;font-size:12px}
 main{display:grid;grid-template-columns:1.4fr 1fr;gap:14px;padding:14px}
 .card{background:#1e293b;border:1px solid #334155;border-radius:10px;padding:12px}
 .card h2{font-size:13px;margin:0 0 8px;color:#94a3b8;text-transform:uppercase;letter-spacing:.04em}
 svg{width:100%;height:380px;background:#0b1220;border-radius:8px}
 table{width:100%;border-collapse:collapse;font-size:12.5px}
 th,td{text-align:left;padding:5px 8px;border-bottom:1px solid #273449}
 th{color:#64748b;font-weight:600}
 .on{color:#4ade80} .off{color:#64748b}
 .col{color:#38bdf8;font-weight:700}
 .metric{display:inline-block;margin-right:16px} .metric b{color:#f1f5f9;font-size:18px}
 t.dim{color:#64748b}
</style></head><body>
<header>
 <h1>AODV-EN · mesh monitor</h1>
 <span class="pill">coletor <b id="coll">—</b></span>
 <span class="pill">nos <b id="ncount">0</b></span>
 <span class="pill">RTT <b id="rtt">—</b> ms</span>
 <span class="pill" id="upd">—</span>
</header>
<main>
 <div class="card"><h2>Topologia (RSSI por enlace)</h2><svg id="g" viewBox="0 0 600 380"></svg>
   <div style="font-size:11px;color:#64748b;margin-top:6px">verde &ge;-75 firme · amarelo -75..-88 borda · vermelho &lt;-88 fraco</div>
 </div>
 <div>
  <div class="card"><h2>Resumo da malha</h2>
    <div class="metric">nos<br><b id="m-nodes">0</b></div>
    <div class="metric">enlaces<br><b id="m-links">0</b></div>
    <div class="metric">rotas<br><b id="m-routes">0</b></div>
    <div class="metric">RTT med<br><b id="m-rtt">—</b></div>
  </div>
  <div class="card" style="margin-top:14px"><h2>Nos (contadores via telemetria)</h2>
    <table><thead><tr><th>no</th><th>tx</th><th>rx</th><th>ctrl</th><th>deliv</th><th>estado</th></tr></thead>
    <tbody id="nt"></tbody></table>
  </div>
  <div class="card" style="margin-top:14px"><h2>Rotas do coletor</h2>
    <table><thead><tr><th>destino</th><th>via</th><th>hops</th><th>metric</th></tr></thead>
    <tbody id="rt"></tbody></table>
  </div>
 </div>
</main>
<script>
const W=600,H=380;
function rssiColor(r){ if(r==null)return '#475569'; if(r>=-75)return '#4ade80'; if(r>=-88)return '#facc15'; return '#f87171'; }
function place(nodes){ const cx=W/2,cy=H/2,R=130,p={}; nodes.forEach((n,i)=>{const a=-Math.PI/2+2*Math.PI*i/nodes.length; p[n.mac]=[cx+R*Math.cos(a),cy+R*Math.sin(a)];}); return p; }
async function tick(){
 let s; try{ s=await (await fetch('/state')).json(); }catch(e){ return; }
 document.getElementById('coll').textContent=s.collector?s.collector.slice(-5):'—';
 document.getElementById('ncount').textContent=s.node_count;
 document.getElementById('rtt').textContent=s.rtt_last??'—';
 document.getElementById('m-nodes').textContent=s.node_count;
 document.getElementById('m-links').textContent=s.links.length;
 document.getElementById('m-routes').textContent=s.routes.length;
 document.getElementById('m-rtt').textContent=s.rtt_mean??'—';
 document.getElementById('upd').textContent=new Date().toLocaleTimeString();
 const pos=place(s.nodes); let g='';
 s.links.forEach(l=>{const A=pos[l.a],B=pos[l.b]; if(!A||!B)return;
   g+=`<line x1="${A[0]}" y1="${A[1]}" x2="${B[0]}" y2="${B[1]}" stroke="${rssiColor(l.rssi)}" stroke-width="2.5"/>`;
   g+=`<text x="${(A[0]+B[0])/2}" y="${(A[1]+B[1])/2-4}" fill="${rssiColor(l.rssi)}" font-size="11" text-anchor="middle">${l.rssi??''}</text>`;});
 s.nodes.forEach(n=>{const P=pos[n.mac]; if(!P)return;
   const col=n.is_collector?'#38bdf8':(n.online?'#4ade80':'#475569');
   g+=`<circle cx="${P[0]}" cy="${P[1]}" r="22" fill="#0b1220" stroke="${col}" stroke-width="3"/>`;
   g+=`<text x="${P[0]}" y="${P[1]+4}" fill="#e2e8f0" font-size="11" text-anchor="middle">${n.label}</text>`;
   if(n.is_collector)g+=`<text x="${P[0]}" y="${P[1]+38}" fill="#38bdf8" font-size="9" text-anchor="middle">coletor</text>`;});
 document.getElementById('g').innerHTML=g;
 document.getElementById('nt').innerHTML=s.nodes.map(n=>`<tr><td class="${n.is_collector?'col':''}">${n.label}${n.is_collector?' ★':''}</td><td>${n.tx}</td><td>${n.rx}</td><td>${n.control}</td><td>${n.delivered}</td><td class="${n.online?'on':'off'}">${n.online?'online':'—'}</td></tr>`).join('');
 document.getElementById('rt').innerHTML=s.routes.length?s.routes.map(r=>`<tr><td>${r.dest.slice(-5)}</td><td>${r.via.slice(-5)}</td><td>${r.hops}</td><td>${r.metric}</td></tr>`).join(''):'<tr><td colspan=4 class=off>sem rotas</td></tr>';
}
setInterval(tick,1000); tick();
</script></body></html>"""


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def do_GET(self):
        if self.path == "/state":
            body = json.dumps(snapshot()).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            body = HTML.encode()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)


def serial_reader(port, baud):
    with serial.Serial(port, baud, timeout=1) as ser:
        while True:
            raw = ser.readline()
            if raw:
                feed(raw.decode("utf-8", errors="replace"), time.monotonic)


def file_replay(path):
    for line in open(path, encoding="utf-8", errors="replace"):
        feed(line, time.monotonic)
        time.sleep(0.02)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", help="serial do coletor")
    ap.add_argument("--file", help="replay de log (em vez de serial)")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--http-port", type=int, default=8090)
    args = ap.parse_args()

    if args.file:
        threading.Thread(target=file_replay, args=(args.file,), daemon=True).start()
    elif args.port:
        if serial is None:
            raise SystemExit("pyserial nao instalado")
        threading.Thread(target=serial_reader, args=(args.port, args.baud), daemon=True).start()
    else:
        raise SystemExit("informe --port ou --file")

    srv = ThreadingHTTPServer(("127.0.0.1", args.http_port), Handler)
    print(f"[mesh_monitor] http://127.0.0.1:{args.http_port}/  (Ctrl-C p/ sair)")
    srv.serve_forever()


if __name__ == "__main__":
    main()
