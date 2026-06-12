import os, time
from playwright.sync_api import sync_playwright

OUT = "/Users/huaksonlima/Documents/tcc/aodv-en/monitor_prints"
URL = "http://127.0.0.1:8080"
os.makedirs(OUT, exist_ok=True)
for f in os.listdir(OUT):
    if f.endswith(".png"):
        os.remove(os.path.join(OUT, f))

log = []
def full(page, name):
    p = os.path.join(OUT, name)
    try:
        page.screenshot(path=p, full_page=True); log.append((name, os.path.getsize(p)))
    except Exception as e:
        log.append((name, "ERR:" + str(e)[:40]))
def el(page, sel, name, idx=None):
    p = os.path.join(OUT, name)
    try:
        loc = page.locator(sel)
        loc = loc.nth(idx) if idx is not None else loc.first
        loc.screenshot(path=p, timeout=6000); log.append((name, os.path.getsize(p)))
    except Exception as e:
        log.append((name, "ERR:" + str(e)[:40]))

def txt(page, sel):
    try: return page.locator(sel).inner_text(timeout=1500).strip()
    except Exception: return "?"

with sync_playwright() as pw:
    br = pw.chromium.launch(headless=True)
    ctx = br.new_context(viewport={"width": 1500, "height": 1100}, device_scale_factor=2)
    page = ctx.new_page()
    for _ in range(40):
        try: page.goto(URL, timeout=4000); break
        except Exception: time.sleep(2)

    # FASE A: momentos (descoberta -> dados)
    for i in range(12):
        k = txt(page, "#m-known"); d = txt(page, "#m-delivered"); a = txt(page, "#m-ack")
        full(page, f"momento_{i:02d}_nos{k}_data{d}_ack{a}.png")
        time.sleep(4)

    # espera node-rows
    for _ in range(30):
        try:
            if page.locator(".node-row").count() > 0: break
        except Exception: pass
        time.sleep(2)
    time.sleep(4)

    # FASE B: secoes
    full(page, "secao_00_overview.png")
    el(page, "header.topbar", "secao_01_topbar.png")
    el(page, ".graph-card", "secao_02_topologia_painel.png")
    el(page, "#cy", "secao_03_grafo.png")
    el(page, ".legend", "secao_04_legenda.png")
    el(page, ".metrics-grid", "secao_05_resumo.png")
    el(page, "#nodes-list", "secao_06_nos_lista.png")
    el(page, "#events-list", "secao_07_eventos.png")

    # itens: cada metrica
    nm = page.locator(".metric").count()
    for i in range(nm):
        lab = ""
        try: lab = page.locator(".metric").nth(i).locator(".metric-label").inner_text(timeout=1000)
        except Exception: pass
        safe = "".join(c for c in lab.lower().replace(" ", "_") if c.isalnum() or c == "_")[:16] or f"m{i}"
        el(page, ".metric", f"item_metrica_{i:02d}_{safe}.png", idx=i)

    # itens: cada node-row + hover tooltip + click highlight
    nrows = page.locator(".node-row").count()
    for i in range(nrows):
        mac = page.locator(".node-row").nth(i).get_attribute("data-mac") or f"idx{i}"
        safe = mac.replace(":", "")
        el(page, ".node-row", f"item_no_{i:02d}_{safe}.png", idx=i)
        # hover -> tooltip
        try:
            page.locator(".node-row").nth(i).hover(timeout=3000); time.sleep(0.8)
            full(page, f"detalhe_no_{i:02d}_{safe}_hover.png")
        except Exception as e:
            log.append((f"hover_{i}", "ERR:" + str(e)[:40]))
        # click -> highlight no grafo
        try:
            page.locator(".node-row").nth(i).click(timeout=3000); time.sleep(1.0)
            el(page, "#cy", f"detalhe_no_{i:02d}_{safe}_grafo.png")
            full(page, f"detalhe_no_{i:02d}_{safe}_full.png")
        except Exception as e:
            log.append((f"click_{i}", "ERR:" + str(e)[:40]))

    # itens: cada evento (primeiros 15)
    nev = min(page.locator(".event-line").count(), 15)
    for i in range(nev):
        el(page, ".event-line", f"item_evento_{i:02d}.png", idx=i)

    # interacoes
    try:
        page.locator("#btn-relayout").click(timeout=3000); time.sleep(2.2)
        el(page, "#cy", "interacao_relayout.png")
    except Exception as e: log.append(("relayout", "ERR:" + str(e)[:40]))
    try:
        page.locator("#btn-fit").click(timeout=3000); time.sleep(1.5)
        el(page, "#cy", "interacao_fit.png")
    except Exception as e: log.append(("fit", "ERR:" + str(e)[:40]))

    # FASE D: mais momentos
    for i in range(6):
        time.sleep(6)
        d = txt(page, "#m-delivered"); a = txt(page, "#m-ack")
        full(page, f"momentoB_{i:02d}_data{d}_ack{a}.png")

    ctx.close(); br.close()

ok = [n for n, s in log if not (isinstance(s, str) and s.startswith("ERR"))]
err = [(n, s) for n, s in log if isinstance(s, str) and s.startswith("ERR")]
print("OK", len(ok), "ERR", len(err))
for n, s in err: print("ERRO", n, s)
