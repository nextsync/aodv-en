#!/usr/bin/env python3
"""
build_tcc_pdf.py - gera um PDF dedicado do relatorio completo do TCC
(docs/tcc-trabalho-completo.md), standalone, com capa e CSS de impressao.

Pipeline: markdown -> HTML (capa + sumario auto) -> Chrome headless -> PDF.

Uso:
  $IDFPY docs/build_tcc_pdf.py     # IDFPY tem a lib markdown
Saidas:
  docs/tcc-trabalho-completo.pdf   (final, commitado)
  docs/tcc-trabalho-completo.html  (intermediario, gitignored)
"""

import shutil
import subprocess
import sys
from pathlib import Path

import markdown

ROOT = Path("/Users/huaksonlima/Documents/tcc/aodv-en")
SRC = ROOT / "docs" / "tcc-trabalho-completo.md"
OUT_HTML = ROOT / "docs" / "tcc-trabalho-completo.html"
OUT_PDF = ROOT / "docs" / "tcc-trabalho-completo.pdf"

CSS = """
@page { size: A4; margin: 22mm 18mm;
  @bottom-center { content: counter(page); font-family: -apple-system, sans-serif; font-size: 9pt; color: #888; } }
* { box-sizing: border-box; }
html, body { margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", "Helvetica Neue", Arial, sans-serif;
  font-size: 11pt; line-height: 1.55; color: #1a202c; background: #fff; }
.cover { page-break-after: always; padding-top: 28vh; text-align: center; }
.cover h1 { font-size: 30pt; font-weight: 800; margin: 0 0 10pt; color: #1a365d; letter-spacing: -0.02em; }
.cover .subtitle { font-size: 14pt; color: #4a5568; margin-bottom: 20pt; }
.cover .meta { font-size: 11pt; color: #718096; margin-top: 30pt; }
.cover .badge { display: inline-block; padding: 4pt 12pt; border-radius: 12pt; background: #edf2f7; color: #2d3748; font-size: 10pt; margin: 4pt; }
h1, h2, h3, h4 { color: #1a365d; margin-top: 16pt; margin-bottom: 7pt; line-height: 1.25; page-break-after: avoid; }
h1 { font-size: 20pt; border-bottom: 2px solid #cbd5e0; padding-bottom: 4pt; page-break-before: always; }
h2 { font-size: 15pt; border-bottom: 1px solid #e2e8f0; padding-bottom: 3pt; }
h3 { font-size: 12.5pt; } h4 { font-size: 11.5pt; color: #2d3748; }
p { margin: 5pt 0 8pt; text-align: justify; orphans: 3; widows: 3; }
ul, ol { margin: 5pt 0 8pt; padding-left: 22pt; } li { margin: 2pt 0; }
a { color: #2563eb; text-decoration: none; } strong { color: #1a202c; font-weight: 700; }
code { font-family: "SF Mono", Menlo, Consolas, monospace; font-size: 9.5pt; background: #f7fafc; padding: 1pt 4pt; border-radius: 3pt; border: 1px solid #e2e8f0; }
pre { background: #0f172a; color: #e2e8f0; padding: 9pt 12pt; border-radius: 5pt; overflow-x: auto; font-size: 8.3pt; line-height: 1.4; page-break-inside: avoid; margin: 8pt 0; }
pre code { background: transparent; color: inherit; border: none; padding: 0; }
table { border-collapse: collapse; width: 100%; margin: 8pt 0; font-size: 9.3pt; page-break-inside: avoid; }
th, td { border: 1px solid #cbd5e0; padding: 4pt 8pt; text-align: left; vertical-align: top; }
th { background: #edf2f7; font-weight: 700; } tr:nth-child(even) td { background: #f7fafc; }
blockquote { border-left: 4px solid #4a5568; padding: 4pt 12pt; margin: 8pt 0; background: #f7fafc; color: #2d3748; font-style: italic; }
hr { border: none; border-top: 1px solid #cbd5e0; margin: 16pt 0; }
"""

COVER = """
<section class="cover">
  <h1>AODV-EN vs Flooding controlado sobre ESP-NOW</h1>
  <div class="subtitle">Relatorio completo do trabalho — projeto, implementacao,<br>instrumentacao, medicao e comparacao</div>
  <div class="meta">
    Trabalho de Conclusao de Curso<br>
    Bacharelado em Engenharia de Software — IFG Campus Inhumas<br><br>
    <span class="badge">AODV-EN</span><span class="badge">Flooding</span>
    <span class="badge">ESP-NOW v2</span><span class="badge">ESP32</span><span class="badge">RFC 3561</span>
  </div>
</section>
"""

HTML_TEMPLATE = """<!DOCTYPE html><html lang="pt-BR"><head><meta charset="UTF-8" />
<title>AODV-EN vs Flooding — Relatorio Completo do TCC</title><style>{css}</style></head>
<body>{cover}{body}</body></html>"""

CHROME_CANDIDATES = [
    "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
    "/Applications/Brave Browser.app/Contents/MacOS/Brave Browser",
    "/Applications/Chromium.app/Contents/MacOS/Chromium",
]


def find_chrome():
    for c in CHROME_CANDIDATES:
        if Path(c).exists():
            return c
    for name in ("google-chrome", "chromium", "chrome"):
        if shutil.which(name):
            return shutil.which(name)
    return None


def main():
    md = markdown.Markdown(extensions=["fenced_code", "tables", "toc", "sane_lists", "attr_list"])
    body = md.convert(SRC.read_text(encoding="utf-8"))
    html = HTML_TEMPLATE.format(css=CSS, cover=COVER, body=body)
    OUT_HTML.write_text(html, encoding="utf-8")
    print(f"HTML: {OUT_HTML} ({OUT_HTML.stat().st_size/1024:.1f} KB)")

    chrome = find_chrome()
    if chrome is None:
        print("Chrome/Brave/Chromium nao encontrado; PDF nao gerado.", file=sys.stderr)
        return
    cmd = [chrome, "--headless", "--disable-gpu", "--no-sandbox", "--no-pdf-header-footer",
           f"--print-to-pdf={OUT_PDF}", f"file://{OUT_HTML.resolve()}"]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    if r.returncode != 0:
        print(f"ERRO Chrome {r.returncode}: {r.stderr}", file=sys.stderr)
        return
    print(f"PDF:  {OUT_PDF} ({OUT_PDF.stat().st_size/1024/1024:.2f} MB)")


if __name__ == "__main__":
    main()
