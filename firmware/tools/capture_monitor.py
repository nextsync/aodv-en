import os, time, sys
from playwright.sync_api import sync_playwright

OUT = "/Users/huaksonlima/Documents/tcc/aodv-en/monitor_prints"
URL = "http://127.0.0.1:8080"
os.makedirs(OUT, exist_ok=True)
for f in os.listdir(OUT):
    if f.endswith(".png"):
        os.remove(os.path.join(OUT, f))

log = []
def shot_full(page, name):
    p = os.path.join(OUT, name)
    page.screenshot(path=p, full_page=True)
    log.append((name, os.path.getsize(p)))

def shot_el(page, sel, name):
    try:
        loc = page.locator(sel).first
        loc.scroll_into_view_if_needed(timeout=4000)
        p = os.path.join(OUT, name)
        loc.screenshot(path=p)
        log.append((name, os.path.getsize(p)))
    except Exception as e:
        log.append((name, "ERRO:" + str(e)[:50]))

with sync_playwright() as pw:
    br = pw.chromium.launch(headless=True)
    ctx = br.new_context(viewport={"width": 1500, "height": 1000}, device_scale_factor=2)
    page = ctx.new_page()
    # espera o servidor responder
    for _ in range(40):
        try:
            page.goto(URL, timeout=4000)
            break
        except Exception:
            time.sleep(2)
    # ---- FASE A: momentos (serie temporal desde a conexao) ----
    momentos = 12
    known = "0"
    for i in range(momentos):
        try:
            known = page.locator("#m-known").inner_text(timeout=2000)
        except Exception:
            known = "?"
        data = "0"
        try:
            data = page.locator("#m-data").inner_text(timeout=1000)
        except Exception:
            pass
        shot_full(page, f"momento_{i:02d}_known{known}_data{data}.png")
        time.sleep(4)
    # espera malha estabilizar (ate 3 nos) p/ as secoes
    for _ in range(30):
        try:
            if page.locator("#m-known").inner_text(timeout=1500).strip() not in ("0", "?", ""):
                break
        except Exception:
            pass
        time.sleep(2)
    time.sleep(4)  # layout do grafo

    # ---- FASE B: secoes / itens ----
    shot_full(page, "secao_00_overview.png")
    shot_el(page, "header.topbar", "secao_01_topbar.png")
    shot_el(page, ".graph-panel", "secao_02_topologia_painel.png")
    shot_el(page, "#net", "secao_03_grafo.png")
    shot_el(page, "#metrics", "secao_04_resumo.png")
    shot_el(page, "#nodes", "secao_05_nos_lista.png")
    shot_el(page, ".events-panel", "secao_06_eventos.png")

    # cada item: cada metrica do resumo
    metrics = page.locator("#metrics .metric")
    for i in range(metrics.count()):
        shot_el(page, f"#metrics .metric >> nth={i}", f"item_resumo_{i:02d}.png")

    # cada item: cada node-card
    cards = page.locator(".node-card")
    ncards = cards.count()
    for i in range(ncards):
        mac = cards.nth(i).get_attribute("data-mac") or f"idx{i}"
        safe = mac.replace(":", "")
        shot_el(page, f".node-card >> nth={i}", f"item_no_{i:02d}_{safe}.png")

    # cada item: cada evento (primeiros 12)
    evs = page.locator("#events .event") if page.locator("#events .event").count() else page.locator("#events > *")
    nev = min(evs.count(), 12)
    for i in range(nev):
        shot_el(page, f"#events > * >> nth={i}", f"item_evento_{i:02d}.png")

    # ---- FASE C: interacoes / detalhe ----
    for i in range(ncards):
        try:
            cards.nth(i).click(timeout=3000)
            time.sleep(1.2)
            mac = cards.nth(i).get_attribute("data-mac") or f"idx{i}"
            safe = mac.replace(":", "")
            shot_el(page, "#net", f"detalhe_no_{i:02d}_{safe}_grafo.png")
            shot_full(page, f"detalhe_no_{i:02d}_{safe}_full.png")
        except Exception as e:
            log.append((f"detalhe_no_{i}", "ERRO:" + str(e)[:50]))
    # botoes
    try:
        page.locator("#btn-relayout").click(timeout=3000); time.sleep(2)
        shot_el(page, "#net", "interacao_relayout.png")
    except Exception as e:
        log.append(("relayout", "ERRO:" + str(e)[:50]))
    try:
        page.locator("#btn-fit").click(timeout=3000); time.sleep(1.5)
        shot_el(page, "#net", "interacao_fit.png")
    except Exception as e:
        log.append(("fit", "ERRO:" + str(e)[:50]))

    # ---- FASE D: mais momentos (malha rodando) ----
    for i in range(6):
        time.sleep(6)
        d = "0"
        try: d = page.locator("#m-data").inner_text(timeout=1500)
        except Exception: pass
        shot_full(page, f"momentoB_{i:02d}_data{d}.png")

    ctx.close(); br.close()

print("TOTAL", len([1 for n, _ in log]))
for n, s in log:
    print(n, s)
