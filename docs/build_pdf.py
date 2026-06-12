#!/usr/bin/env python3
"""
build_pdf.py - concatena toda a documentacao do AODV-EN num PDF unico.

Pipeline:
  1. Le todos os .md em ordem top-down predefinida (lista DOCS abaixo).
  2. Renderiza cada um para HTML com a lib `markdown`.
  3. Junta tudo num HTML unico com capa, sumario e CSS para impressao.
  4. Chama Chrome (ou Brave) headless para imprimir como PDF.

Uso:
  pip3 install --user markdown
  python3 docs/build_pdf.py

Saidas:
  docs/aodv-en-completo.pdf   (final, commitado)
  docs/aodv-en-completo.html  (intermediario, gitignored)

Ordem de leitura definida em DOCS.
"""

import re
import shutil
import subprocess
import sys
from pathlib import Path

import markdown

ROOT = Path("/Users/huaksonlima/Documents/tcc/aodv-en")
OUT_HTML = ROOT / "docs" / "aodv-en-completo.html"
OUT_PDF = ROOT / "docs" / "aodv-en-completo.pdf"

# Ordem de leitura (top-down).
# Cada entrada: (caminho relativo, titulo na capa/sumario)
DOCS = [
    ("README.md", "Visao Geral do Projeto"),
    ("docs/tcc-trabalho-completo.md", "Relatorio Completo do Trabalho (AODV-EN vs Flooding)"),
    ("TCC.md", "Especificacao do TCC (cenarios, metricas, algoritmos)"),
    ("docs/aodv-base-invariantes.md", "Invariantes do AODV"),
    ("docs/aodv-en-spec-v1.md", "Especificacao v1 (normativa)"),
    ("docs/aodv-en-funcionamento.md", "Funcionamento Completo do Protocolo"),
    ("docs/aodv-en-estruturas-dados.md", "Estruturas de Dados"),
    ("docs/aodv-en-mapa-do-codigo.md", "Mapa do Codigo"),
    ("docs/features/precursores.md", "Feature: Precursores"),
    ("docs/features/enfilaremento-dos-dados.md", "Feature: Fila de DATA pendente"),
    ("docs/features/articulation-point-planejado.md", "Feature planejada: Articulation Point (v2)"),
    ("docs/plano-desenvolvimento-completo.md", "Plano de Desenvolvimento Completo"),
    ("docs/runbook-bancada.md", "Runbook de Bancada"),
    ("docs/tests/README.md", "Casos de Teste - Indice"),
    ("docs/tests/tc-001-descoberta-e-entrega-direta.md", "TC-001 - Descoberta e Entrega Direta"),
    ("docs/tests/tc-002-primeiro-multi-hop.md", "TC-002 - Primeiro Multi-hop"),
    ("docs/tests/tc-003-reconvergencia-apos-falha.md", "TC-003 - Reconvergencia apos Falha"),
    ("docs/tests/tc-004-soak-estabilidade-e-reconvergencia.md", "TC-004 - Soak de Estabilidade"),
    ("docs/tests/tc-005-cadeia-4-nos.md", "TC-005 - Cadeia de 4 Nos"),
    ("docs/tests/guia-leitura-graficos-monitor.md", "Guia de leitura dos graficos do monitor"),
    ("firmware/README.md", "Firmware - README"),
    ("sim/README.md", "Simulacao - README"),
    ("docs/aodv-en-spec-v0.md", "Apendice: Especificacao v0 (OBSOLETA)"),
]

# CSS focado em impressao/PDF
CSS = """
@page {
    size: A4;
    margin: 22mm 18mm;
    @bottom-center {
        content: counter(page);
        font-family: -apple-system, sans-serif;
        font-size: 9pt;
        color: #888;
    }
}

* {
    box-sizing: border-box;
}

html, body {
    margin: 0;
    padding: 0;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", "Helvetica Neue", Arial, sans-serif;
    font-size: 11pt;
    line-height: 1.55;
    color: #1a202c;
    background: white;
}

body {
    max-width: 100%;
}

/* === Capa === */
.cover {
    page-break-after: always;
    padding-top: 30vh;
    text-align: center;
}

.cover h1 {
    font-size: 36pt;
    font-weight: 800;
    margin: 0 0 12pt 0;
    color: #1a365d;
    letter-spacing: -0.02em;
}

.cover .subtitle {
    font-size: 16pt;
    color: #4a5568;
    margin-bottom: 24pt;
}

.cover .meta {
    font-size: 11pt;
    color: #718096;
    margin-top: 36pt;
}

.cover .badge {
    display: inline-block;
    padding: 4pt 12pt;
    border-radius: 12pt;
    background: #edf2f7;
    color: #2d3748;
    font-size: 10pt;
    margin: 4pt;
}

/* === TOC === */
.toc {
    page-break-after: always;
    padding-top: 18pt;
}

.toc h2 {
    border-bottom: 2px solid #1a365d;
    padding-bottom: 6pt;
    margin-bottom: 18pt;
    color: #1a365d;
    font-size: 18pt;
}

.toc ol {
    list-style: none;
    padding-left: 0;
    counter-reset: section;
}

.toc li {
    margin: 8pt 0;
    counter-increment: section;
    font-size: 11.5pt;
}

.toc li::before {
    content: counter(section, decimal-leading-zero) ". ";
    color: #4a5568;
    font-weight: 600;
    margin-right: 6pt;
}

.toc a {
    color: #2d3748;
    text-decoration: none;
}

.toc .doc-source {
    font-size: 9pt;
    color: #a0aec0;
    margin-left: 8pt;
}

/* === Cada doc === */
.doc-section {
    page-break-before: always;
    padding-top: 12pt;
}

.doc-header {
    border-bottom: 1px solid #cbd5e0;
    padding-bottom: 8pt;
    margin-bottom: 18pt;
}

.doc-header .doc-num {
    display: inline-block;
    font-size: 10pt;
    color: #4a5568;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.04em;
}

.doc-header .doc-source {
    font-family: "SF Mono", Menlo, Consolas, monospace;
    font-size: 9pt;
    color: #a0aec0;
}

/* === Markdown rendering === */
h1, h2, h3, h4, h5, h6 {
    color: #1a365d;
    margin-top: 18pt;
    margin-bottom: 8pt;
    line-height: 1.25;
    page-break-after: avoid;
}

h1 { font-size: 22pt; border-bottom: 2px solid #cbd5e0; padding-bottom: 4pt; }
h2 { font-size: 16pt; border-bottom: 1px solid #e2e8f0; padding-bottom: 3pt; }
h3 { font-size: 13pt; }
h4 { font-size: 11.5pt; color: #2d3748; }
h5, h6 { font-size: 11pt; color: #4a5568; }

p {
    margin: 6pt 0 9pt 0;
    text-align: justify;
    orphans: 3;
    widows: 3;
}

ul, ol {
    margin: 6pt 0 9pt 0;
    padding-left: 22pt;
}

li {
    margin: 2pt 0;
}

a {
    color: #2563eb;
    text-decoration: none;
}

strong {
    color: #1a202c;
    font-weight: 700;
}

em {
    color: #2d3748;
}

/* === Code === */
code {
    font-family: "SF Mono", Menlo, Consolas, "Courier New", monospace;
    font-size: 9.5pt;
    background: #f7fafc;
    padding: 1pt 4pt;
    border-radius: 3pt;
    color: #2d3748;
    border: 1px solid #e2e8f0;
}

pre {
    background: #0f172a;
    color: #e2e8f0;
    padding: 9pt 12pt;
    border-radius: 5pt;
    overflow-x: auto;
    font-size: 8.5pt;
    line-height: 1.45;
    page-break-inside: avoid;
    margin: 9pt 0;
}

pre code {
    background: transparent;
    color: inherit;
    border: none;
    padding: 0;
    font-size: inherit;
}

/* === Tables === */
table {
    border-collapse: collapse;
    width: 100%;
    margin: 9pt 0;
    font-size: 9.5pt;
    page-break-inside: avoid;
}

th, td {
    border: 1px solid #cbd5e0;
    padding: 4pt 8pt;
    text-align: left;
    vertical-align: top;
}

th {
    background: #edf2f7;
    font-weight: 700;
    color: #1a202c;
}

tr:nth-child(even) td {
    background: #f7fafc;
}

/* === Blockquote === */
blockquote {
    border-left: 4px solid #4a5568;
    padding: 4pt 12pt;
    margin: 9pt 0;
    background: #f7fafc;
    color: #2d3748;
    font-style: italic;
}

blockquote p {
    margin: 4pt 0;
}

/* === Horizontal rule === */
hr {
    border: none;
    border-top: 1px solid #cbd5e0;
    margin: 18pt 0;
}

/* === Image === */
img {
    max-width: 100%;
    page-break-inside: avoid;
}

/* === Inline code in tables menor === */
table code {
    font-size: 8.5pt;
}
"""

HTML_TEMPLATE = """<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8" />
    <title>AODV-EN - Documentacao Completa</title>
    <style>{css}</style>
</head>
<body>
{cover}
{toc}
{sections}
</body>
</html>
"""

COVER_HTML = """
<section class="cover">
    <h1>AODV-EN</h1>
    <div class="subtitle">Documentacao Completa do Protocolo de Roteamento</div>
    <div class="meta">
        Trabalho de Conclusao de Curso<br>
        Bacharelado em Engenharia de Software - IFG Campus Inhumas<br>
        <br>
        <span class="badge">v1</span>
        <span class="badge">RFC 3561</span>
        <span class="badge">ESP-NOW v2</span>
        <span class="badge">ESP32</span>
        <br>
        <br>
        <em>Adaptacao do AODV para redes mesh multi-hop sobre ESP-NOW</em>
    </div>
</section>
"""


def render_md(text):
    """Converte markdown para HTML com extensoes uteis."""
    md = markdown.Markdown(
        extensions=[
            "fenced_code",
            "tables",
            "toc",
            "sane_lists",
            "attr_list",
            "footnotes",
            "abbr",
        ],
        extension_configs={
            "toc": {"toc_depth": "2-4", "permalink": False},
        },
    )
    return md.convert(text)


def fix_internal_links(html, current_path):
    """
    Reescreve links relativos para anchors internos do PDF.
    Ex: [foo](bar.md) -> #doc-bar
    """
    # Remove links internos quebrados; transforma .md → âncora simples
    def repl(m):
        href = m.group(2)
        if href.startswith(("http://", "https://", "mailto:", "#")):
            return m.group(0)
        # qualquer .md vira anchor
        if ".md" in href:
            # extrai apenas o basename do md
            base = href.split("#")[0]
            base = base.rsplit("/", 1)[-1].replace(".md", "")
            return f'{m.group(1)}="#doc-{base}"'
        return m.group(0)

    return re.sub(r'(href)="([^"]+)"', repl, html)


def build_toc(docs):
    """Gera o sumário."""
    items = []
    for idx, (path, title) in enumerate(docs, 1):
        anchor = Path(path).stem
        items.append(
            f'<li><a href="#doc-{anchor}">{title}</a>'
            f' <span class="doc-source">{path}</span></li>'
        )
    return f"""
<section class="toc">
    <h2>Sumario</h2>
    <ol>{"".join(items)}</ol>
</section>
"""


def build_section(idx, path, title, total):
    """Gera uma seção (= um doc) com header e conteúdo."""
    full_path = ROOT / path
    if not full_path.exists():
        body = f"<p><em>Arquivo nao encontrado: {path}</em></p>"
    else:
        text = full_path.read_text(encoding="utf-8")
        body = render_md(text)
        body = fix_internal_links(body, path)

    anchor = Path(path).stem
    header = f"""
    <header class="doc-header">
        <span class="doc-num">Documento {idx:02d} de {total:02d}</span><br>
        <span class="doc-source">{path}</span>
    </header>
    <h1 style="margin-top:0">{title}</h1>
    """
    return f'<section class="doc-section" id="doc-{anchor}">{header}{body}</section>'


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


def render_pdf(html_path, pdf_path):
    """Imprime HTML para PDF via Chrome headless."""
    chrome = find_chrome()
    if chrome is None:
        print(
            "AVISO: Chrome/Brave/Chromium nao encontrado; PDF nao foi gerado.\n"
            f"O HTML esta em {html_path} e pode ser convertido manualmente.",
            file=sys.stderr,
        )
        return False

    print(f"Usando: {chrome}")
    cmd = [
        chrome,
        "--headless",
        "--disable-gpu",
        "--no-sandbox",
        "--no-pdf-header-footer",
        f"--print-to-pdf={pdf_path}",
        f"file://{html_path.resolve()}",
    ]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    if result.returncode != 0:
        print(f"ERRO: Chrome retornou {result.returncode}", file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        return False
    return True


def main():
    sections_html = []
    total = len(DOCS)
    for idx, (path, title) in enumerate(DOCS, 1):
        sections_html.append(build_section(idx, path, title, total))

    html = HTML_TEMPLATE.format(
        css=CSS,
        cover=COVER_HTML,
        toc=build_toc(DOCS),
        sections="\n".join(sections_html),
    )

    OUT_HTML.parent.mkdir(parents=True, exist_ok=True)
    OUT_HTML.write_text(html, encoding="utf-8")
    print(f"HTML gerado: {OUT_HTML}  ({OUT_HTML.stat().st_size / 1024:.1f} KB)")

    if render_pdf(OUT_HTML, OUT_PDF):
        size_mb = OUT_PDF.stat().st_size / 1024 / 1024
        print(f"PDF gerado:  {OUT_PDF}  ({size_mb:.2f} MB)")


if __name__ == "__main__":
    main()
