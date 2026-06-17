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

# Timeouts derivados do intervalo de report (sem numeros magicos):
# online enquanto < ONLINE_WINDOWS reports ausentes; stale ate STALE_WINDOWS; link ate LINK_WINDOWS.
CFG = {"report_s": 4.0, "online_windows": 5, "stale_windows": 11, "link_windows": 7}

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
        on_s = CFG["report_s"] * CFG["online_windows"]
        st_s = CFG["report_s"] * CFG["stale_windows"]
        lk_s = CFG["report_s"] * CFG["link_windows"]
        for mac, n in STATE["nodes"].items():
            age = now - n["last_seen"]
            nodes.append({
                "mac": mac, "label": mac[-5:],
                "tx": n["tx"], "rx": n["rx"], "control": n["control"], "delivered": n["delivered"],
                # janelas de report ausentes toleradas antes de offline (deriva de --report-interval),
                # absorvendo perdas best-effort do report multi-hop sem falso-offline.
                "online": age < on_s,
                "stale": on_s <= age < st_s,
                "last_seen_s": round(age, 1),
                "is_collector": mac == STATE["collector"],
            })
        links = []
        for lk in STATE["links"].values():
            if (now - lk["last_seen"]) < lk_s:
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
 .on{color:#4ade80} .off{color:#64748b} .st{color:#facc15}
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
 <div class="card"><h2>Topologia (RSSI por enlace)</h2><svg id="g" viewBox="0 0 600 380" preserveAspectRatio="xMidYMid meet"></svg>
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
function rssiColor(r){ if(r==null)return '#475569'; if(r>=-75)return '#4ade80'; if(r>=-88)return '#facc15'; return '#f87171'; }
// hops por no: BFS sobre os enlaces a partir do coletor (sem hardcode de saltos)
function hopsFrom(nodes,links,coll){
 const adj={}; nodes.forEach(n=>adj[n.mac]=[]);
 links.forEach(l=>{ if(adj[l.a]&&adj[l.b]){adj[l.a].push(l.b);adj[l.b].push(l.a);} });
 const hp={}; if(coll==null) return hp; hp[coll]=0; let q=[coll];
 while(q.length){ const u=q.shift(); (adj[u]||[]).forEach(v=>{ if(hp[v]===undefined){hp[v]=hp[u]+1;q.push(v);} }); }
 return hp;
}
// layout em CAMADAS por hops (escala com N e com profundidade; nada fixo)
function place(nodes,links,coll){
 const hp=hopsFrom(nodes,links,coll);
 const byHop={}; let maxHop=0;
 nodes.forEach(n=>{ const h=(hp[n.mac]!==undefined?hp[n.mac]:99); (byHop[h]=byHop[h]||[]).push(n.mac); if(h!==99&&h>maxHop)maxHop=h; });
 const cols=Math.max(1,maxHop+1)+ (byHop[99]?1:0);
 const colW=170, rowH=90;
 const W=Math.max(600,cols*colW+80), p={}, dims={W:W};
 let maxRows=1; Object.values(byHop).forEach(a=>{ if(a.length>maxRows)maxRows=a.length; });
 const H=Math.max(380,maxRows*rowH+60); dims.H=H;
 const order=Object.keys(byHop).map(Number).sort((a,b)=>a-b);
 order.forEach((h,ci)=>{ const macs=byHop[h]; const x=60+ci*((W-120)/Math.max(1,cols-1||1));
   macs.forEach((m,ri)=>{ const y=60+(H-120)*((ri+0.5)/macs.length); p[m]=[x,y,h]; }); });
 return {p,dims,hp,maxHop};
}
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
 const {p:pos,dims,hp,maxHop}=place(s.nodes,s.links,s.collector);
 const N=s.node_count;
 const R=Math.max(9,Math.min(22,260/Math.max(4,N)));      // raio do no escala com N
 const fz=Math.max(7,Math.min(12,R*0.5));                  // fonte escala com raio
 const showRssi = s.links.length<=24;                      // suprime texto de RSSI em grafos densos
 const svg=document.getElementById('g'); svg.setAttribute('viewBox',`0 0 ${dims.W} ${dims.H}`);
 let g='';
 // guias de camada (hops)
 for(let h=0;h<=maxHop;h++){ const anyX=Object.values(pos).find(P=>P[2]===h); if(anyX) g+=`<text x="${anyX[0]}" y="20" fill="#334155" font-size="10" text-anchor="middle">${h===0?'coletor':h+' hop'}</text>`; }
 s.links.forEach(l=>{const A=pos[l.a],B=pos[l.b]; if(!A||!B)return;
   g+=`<line x1="${A[0]}" y1="${A[1]}" x2="${B[0]}" y2="${B[1]}" stroke="${rssiColor(l.rssi)}" stroke-width="${Math.max(1.2,R*0.12)}"/>`;
   if(showRssi) g+=`<text x="${(A[0]+B[0])/2}" y="${(A[1]+B[1])/2-4}" fill="${rssiColor(l.rssi)}" font-size="${fz-1}" text-anchor="middle">${l.rssi??''}</text>`;});
 s.nodes.forEach(n=>{const P=pos[n.mac]; if(!P)return;
   const col=n.is_collector?'#38bdf8':(n.online?(n.stale?'#facc15':'#4ade80'):'#475569');
   g+=`<circle cx="${P[0]}" cy="${P[1]}" r="${R}" fill="#0b1220" stroke="${col}" stroke-width="${Math.max(1.5,R*0.13)}"><title>${n.label} · ${P[2]===99?'sem rota':P[2]+' hops'} · visto ha ${n.last_seen_s}s</title></circle>`;
   g+=`<text x="${P[0]}" y="${P[1]+fz*0.35}" fill="#e2e8f0" font-size="${fz}" text-anchor="middle">${n.label}</text>`;});
 svg.innerHTML=g;
 document.getElementById('nt').innerHTML=s.nodes.map(n=>{const st=n.online?(n.stale?'stale':'online'):'offline';const cls=n.online?(n.stale?'st':'on'):'off';return `<tr><td class="${n.is_collector?'col':''}">${n.label}${n.is_collector?' ★':''}</td><td>${n.tx}</td><td>${n.rx}</td><td>${n.control}</td><td>${n.delivered}</td><td class="${cls}" title="visto ha ${n.last_seen_s}s">${st}</td></tr>`;}).join('');
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
    ap.add_argument("--report-interval", type=float, default=4.0,
                    help="intervalo (s) do report dos nos; deriva os timeouts online/stale/link")
    args = ap.parse_args()
    CFG["report_s"] = args.report_interval

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
