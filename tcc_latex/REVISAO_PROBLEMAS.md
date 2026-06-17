# TCC AODV-EN — Lista de Problemas e Correções (revisão profunda)

> Gerado da revisão multiagente (11 dimensões) + auditoria adversarial + verificação manual no código/arquivos.
> Branch `feat/reports` · fonte `tcc_latex/`. **Nota global atual: 66%** — bom, mas ainda não pronto para banca.

**Como usar:** cada item tem checkbox `[ ]`, severidade, categoria, local exato e correção concreta. 
Marque `[x]` ao concluir. Os números dos resultados (PDR/latência/energia, nº de seeds/nós) **vão mudar** — 
itens que dependem disso estão na Seção **D** e só fecham após a nova campanha.

## Legenda
- 🔴 **CRÍTICO** — compromete aprovação/credibilidade na banca
- 🟠 **ALTO** — fraqueza séria de conteúdo/método
- 🟡 **MÉDIO** — melhoria importante de qualidade
- ⚪ **BAIXO** — polimento

## Panorama

| Severidade | Qtd |
|---|---|
| 🔴 CRÍTICO | 12 |
| 🟠 ALTO | 27 |
| 🟡 MÉDIO | 40 |
| ⚪ BAIXO | 35 |
| **Total (capítulos + transversais)** | **114** |
| 🔎 Furos da auditoria adversarial (Seção A) | 8 |

| # | Alvo | Nota |
|---|---|---|
| 1 | Capítulo 1 — Introdução e Objetivos | 62 |
| 2 | Capítulo 2 — Referencial Teórico | 72 |
| 3 | Capítulo 3 — Metodologia | 72 |
| 4 | Capítulo 4 — Projeto e Implementação do AODV-EN | 78 |
| 5 | Capítulo 5 — Resultados e Discussão | 74 |
| 6 | Capítulo 6 — Conclusão | 79 |
| 7 | Resumo e Abstract | 85 |
| 8 | Transversal — Alinhamento Objetivos × Resultados × Conclusão | 58 |
| 9 | Transversal — Formatação ABNT / LaTeX (referências cruzadas, floats) | 58 |
| 10 | Transversal — Referências Bibliográficas (referencias.bib + citações) | 62 |
| 11 | Transversal — Redação Acadêmica PT-BR (registro, voz, tempo verbal) | 72 |

---

## 1. Capítulo 1 — Introdução e Objetivos  ·  nota 62

_A introdução tem boa densidade técnica, encadeamento lógico coerente (ubiquidade -> IoT/WSN -> ESP32 -> mesh -> roteamento -> ESP-NOW -> lacuna -> proposta) e fundamentação com citações pertinentes. O funil argumentativo até a lacuna de pesquisa está bem construído e a justificativa da escolha do AODV é convincente. Entretanto, a qualidade de escrita é prejudicada por problemas estruturais e de redação relevantes: (1) toda a introdução é UM ÚNICO parágrafo de ~120 linhas, o que viola a norma de parágrafo-tópico e dificulta gravemente a leitura; (2) a hierarquia de seções está errada — OBJETIVO GERAL e OBJETIVOS ESPECÍFICOS aparecem como \section irmãs de OBJETIVOS, e deveriam ser \subsection; (3) AUSÊNCIA da seção obrigatória de problema de pesquisa/pergunta explícita e da seção ESTRUTURA DO TRABALHO/organização dos capítulos; (4) frases fragmentadas (períodos sem verbo principal), redundâncias e uma frase órfã ("Dessa forma, o ESP-NOW possui restrições específicas...") que quebra a coesão. O problema de pesquisa está implícito mas nunca enunciado como pergunta. Os objetivos específicos usam verbos mensuráveis e alinhados, sendo o ponto mais forte do capítulo._

**Pontos fortes:**
- Encadeamento lógico do funil argumentativo bem construído: parte do macro (Computação Ubíqua/IoT) e estreita progressivamente até a lacuna específica (roteamento multi-hop sobre ESP-NOW), terminando na proposta
- Fundamentação consistente com citações pertinentes e atuais (weiser1991, becker2025, urazayev2023, cujilema2023, sestak2022, perkins2003), ancorando cada afirmação técnica
- As três limitações do ESP-NOW (multi-hop, limite de peers, broadcast) são apresentadas de forma estruturada e enumerada, sustentando bem a motivação
- Justificativa da escolha do AODV como protocolo base é clara e convincente (roteamento reativo, ampla validação acadêmica, extensibilidade)
- Objetivos específicos com verbos de ação mensuráveis (Analisar, Projetar, Implementar, Avaliar, Comparar) e bem alinhados ao objetivo geral, em sequência metodológica coerente
- Objetivo geral conciso e abrangente, capturando o tripé propor/implementar/avaliar

**Problemas:**

### [x] C1-001 · 🔴 CRÍTICO · `escrita`
- **Local:** Linhas 6-125, todo o corpo da INTRODUÇÃO
- **Problema:** A introdução inteira é um único parágrafo monolítico de cerca de 120 linhas, cobrindo desde Mark Weiser até as adaptações do AODV-EN. Isso viola a norma acadêmica de um tópico por parágrafo e torna a leitura exaustiva, escondendo a progressão argumentativa.
- **Correção:** Quebrar em 6-8 parágrafos temáticos com transições explícitas: (1) Computação Ubíqua/IoT/WSN; (2) ESP32 e desafios de topologia centralizada; (3) redes mesh e roteamento multi-hop; (4) classificação de protocolos ad-hoc; (5) ESP-NOW e suas vantagens; (6) limitações estruturais do ESP-NOW e a lacuna; (7) a proposta AODV-EN e suas adaptações.

### [x] C1-002 · 🔴 CRÍTICO · `abnt-formatacao`
- **Local:** Linhas 127-129, \section{OBJETIVOS} / \section{OBJETIVO GERAL} / \section{OBJETIVOS ESPECÍFICOS}
- **Problema:** Hierarquia de seções incorreta: OBJETIVO GERAL e OBJETIVOS ESPECÍFICOS estão marcados como \section, no mesmo nível de OBJETIVOS, quando logicamente são subdivisões dele. Além disso, \section{OBJETIVOS} fica vazia (sem texto introdutório), gerando título órfão. Isso compromete a numeração ABNT e o sumário.
- **Correção:** Transformar OBJETIVO GERAL e OBJETIVOS ESPECÍFICOS em \subsection sob \section{OBJETIVOS}, e adicionar uma frase introdutória curta sob OBJETIVOS antes da primeira subseção (ex.: 'Esta seção apresenta o objetivo geral e os objetivos específicos que orientam o desenvolvimento deste trabalho.').

### [x] C1-003 · 🔴 CRÍTICO · `completude`
- **Local:** Fim do capítulo (após linha 167)
- **Problema:** Ausência da seção de ESTRUTURA/ORGANIZAÇÃO DO TRABALHO, que descreve o conteúdo de cada capítulo subsequente. É item estrutural esperado em TCC e exigido pela maioria das bancas/normas institucionais.
- **Correção:** Adicionar uma seção final 'ESTRUTURA DO TRABALHO' (ou parágrafo de organização) resumindo, em um parágrafo, o conteúdo dos Capítulos 2 a 6 (referencial teórico, metodologia/desenvolvimento, resultados, conclusões).

### [ ] C1-004 · 🟠 ALTO · `completude`
- **Local:** Introdução, transição para a proposta (linhas 96-107)
- **Problema:** O problema de pesquisa nunca é enunciado de forma explícita como pergunta ou afirmação-problema. Ele permanece implícito ('Diante dessa lacuna...'), sem uma sentença que diga claramente qual é a questão central que o trabalho responde.
- **Correção:** Inserir, antes da apresentação da proposta, uma formulação explícita do problema, idealmente como pergunta de pesquisa (ex.: 'Diante disso, coloca-se a questão: é viável adaptar o protocolo AODV para operar sobre ESP-NOW, contornando suas restrições de hardware, mantendo desempenho competitivo frente a abordagens por flooding?').

### [ ] C1-005 · 🟠 ALTO · `coesao`
- **Local:** Linhas 107-108
- **Problema:** Frase órfã e desconexa: 'Dessa forma, o ESP-NOW possui restrições específicas de hardware e comunicação.' O conector 'Dessa forma' não tem relação causal com a frase anterior (que apresenta a RFC 3561), e a afirmação repete o que já foi exposto nas três limitações anteriores, quebrando a coesão.
- **Correção:** Remover a frase ou reescrevê-la integrada à justificativa, sem o conector indevido. Ex.: ligar diretamente a apresentação do AODV-EN à justificativa da escolha do AODV, sem essa sentença intermediária redundante.

### [ ] C1-006 · 🟠 ALTO · `escrita`
- **Local:** Linhas 24-26
- **Problema:** Período sem verbo principal (fragmento de frase): 'Características que o tornam ideal para prototipagem e implantação de soluções IoT em escala.' Inicia com pronome relativo sem oração principal, constituindo erro gramatical.
- **Correção:** Unir à frase anterior com vírgula: '...custo acessível, características que o tornam ideal para prototipagem e implantação de soluções IoT em escala.'

### [ ] C1-007 · 🟠 ALTO · `escrita`
- **Local:** Linhas 60-64
- **Problema:** Período sem oração principal: 'Desenvolvido pela Espressif Systems, operando de forma connectionless sobre a camada de enlace do padrão IEEE 802.11, permitindo a troca de mensagens curtas...' — sequência de orações reduzidas (particípio/gerúndio) sem verbo finito que sustente a frase.
- **Correção:** Reescrever com verbo principal, ex.: 'Desenvolvido pela Espressif Systems, o ESP-NOW opera de forma connectionless sobre a camada de enlace do padrão IEEE 802.11 e permite a troca de mensagens curtas (até 250 bytes de payload) sem necessidade de associação prévia a um ponto de acesso Wi-Fi.'

### [ ] C1-008 · 🟡 MÉDIO · `coesao`
- **Local:** Linhas 26-31 (transição) e 47-49
- **Problema:** Transições abruptas entre macroblocos temáticos. Ex.: 'A comunicação em redes IoT frequentemente enfrenta desafios...' surge logo após o ESP32 sem conector; e 'Os protocolos de roteamento para redes ad-hoc podem ser classificados...' entra sem ligação com o parágrafo de mesh. O problema é agravado por ser tudo um só parágrafo.
- **Correção:** Ao quebrar em parágrafos, iniciar cada um com frase-tópico de transição que conecte ao bloco anterior (ex.: 'Embora versátil, o ESP32 enfrenta...', 'Para viabilizar o roteamento nessas redes mesh, ...').

### [ ] C1-009 · 🟡 MÉDIO · `tecnico`
- **Local:** Linhas 114-117, adaptação (i) e linha 115 'cache de 8 entradas'
- **Problema:** Na introdução afirma-se 'cache de 8 entradas simultâneas (configurável)', mas no parágrafo das limitações (linha 84) menciona-se o limite de 20 peers do ESP-NOW. A introdução não explica por que o cache (8) é menor que o limite de hardware (20), o que pode confundir o leitor sobre a relação entre os dois valores.
- **Correção:** Esclarecer brevemente que o cache LRU é uma escolha de projeto (não o limite físico), ou remover o número específico da introdução, deixando o detalhamento para a metodologia. Garantir consistência terminológica entre 'peers', 'cache' e 'tabela de pares'.

### [ ] C1-010 · 🟡 MÉDIO · `argumentacao`
- **Local:** Introdução inteira vs. título do trabalho (AODV-EN vs flooding)
- **Problema:** O trabalho compara AODV-EN com baseline de flooding (conforme escopo/metodologia), mas a introdução nunca introduz a abordagem por flooding/broadcast como alternativa concorrente nem motiva a comparação. O leitor chega ao problema sem saber que flooding é o termo de comparação central.
- **Correção:** Acrescentar, na motivação ou no problema de pesquisa, uma menção à abordagem por flooding/broadcast como solução ingênua comum em ESP-NOW, justificando por que ela serve de baseline e por que um protocolo reativo poderia superá-la (latência/energia/overhead).

### [ ] C1-011 · 🟡 MÉDIO · `escrita`
- **Local:** Linha 21 e linha 60 (repetição) e linha 11/15 ('décadas')
- **Problema:** Repetições e ecos: 'desenvolvido pela Espressif Systems' aparece duas vezes (l.22 e l.60); 'Espressif' aparece ainda em 'ecossistema Espressif' (l.90). 'paradigma' e 'visão' próximos. Há também repetição de 'baixo custo'/'baixo consumo'.
- **Correção:** Eliminar a segunda atribuição redundante 'desenvolvido pela Espressif Systems' (o leitor já sabe), e variar vocabulário para reduzir ecos lexicais.

### [ ] C1-012 · ⚪ BAIXO · `abnt-formatacao`
- **Local:** Linha 4 e linha 127
- **Problema:** Inconsistência de caixa nos títulos: o capítulo e as seções OBJETIVOS estão em CAIXA ALTA, mas as seções do Capítulo 2 (ex.: 'O Microcontrolador ESP32') usam caixa de título normal. Convém padronizar o estilo de títulos em todo o trabalho.
- **Correção:** Definir um padrão único (ABNT costuma usar caixa alta para seções primárias e caixa baixa/título para subseções) e aplicá-lo consistentemente entre capítulos.

### [ ] C1-013 · ⚪ BAIXO · `escrita`
- **Local:** Linha 68 ('2,8 ms'), linha 71 ('15%'), linha 99 ('75 ms', '9,25%')
- **Problema:** Mistura de estilos de citação de números entre o português (vírgula decimal, ok) mas sem padronização do uso de 'ms', '%' colados/separados e sem espaço protegido (~) antes de unidades, podendo gerar quebras de linha indevidas em LaTeX.
- **Correção:** Padronizar e usar espaço protegido em LaTeX antes de unidades e símbolos (ex.: '2,8~ms', 'PDR superior a 99\%', '55~metros'), conforme boas práticas tipográficas/ABNT.

### [ ] C1-014 · ⚪ BAIXO · `escrita`
- **Local:** Linha 4, \label{introduuxe7uxe3o} e linha 136 \label{objetivos-especuxedficos}
- **Problema:** Labels com caracteres de escape Unicode automáticos (introduuxe7uxe3o, especuxedficos) gerados por conversão (provavelmente pandoc). São funcionais mas ilegíveis e propensos a erro em referências cruzadas.
- **Correção:** Renomear os labels para formas limpas e ASCII (ex.: \label{cap:introducao}, \label{sec:objetivos-especificos}) para manutenção e referência cruzada mais robustas.

---

## 2. Capítulo 2 — Referencial Teórico  ·  nota 72

_Capítulo bem organizado e de leitura fluida, com PT-BR formal correto e progressão lógica do macro (IoT/WSN) para o específico (AODV). A redação técnica é, na maior parte, precisa e os quadros são úteis. Porém, há três fragilidades sérias para um referencial teórico: (1) o flooding — algoritmo de referência de TODO o TCC (presente no resumo, resultados e conclusão) — nunca é definido nem mencionado aqui, e o conceito de "broadcast storm" (ni1999, já na .bib) fica ausente; (2) as métricas de avaliação do trabalho (PDR, NRL, latência, energia) não são definidas conceitualmente, apesar de PDR aparecer de passagem; a literatura de métricas (couto2003, draves2004, rfc6551, karlwillig2005, heinzelman2000) está toda relegada ao Cap. 4. (3) Referências cruzadas frágeis: as tabelas são citadas no texto com números fixos ("Quadro 1/3/6"), sem \label/\ref, e 3 dos 6 quadros nunca são referenciados na prosa. Há ainda atribuições por "Fonte:" sem \cite inline e Weiser nomeado sem citação. Corrigidos esses pontos, o capítulo sobe facilmente para a faixa de 85+._

**Pontos fortes:**
- Estrutura de seções coerente e bem ordenada (IoT/WSN -> ESP32 -> ESP-NOW -> mesh/roteamento -> AODV), com parágrafo introdutório que anuncia explicitamente a sequência do capítulo.
- Registro acadêmico PT-BR formal e correto, com terminologia técnica em itálico (\emph) e siglas expandidas na primeira ocorrência (IoT, WSN, SoC, WMN, AODV, RREQ etc.).
- A seção 2.4.3 (limitações multi-hop do ESP-NOW) é particularmente forte: encadeia as três limitações estruturais de forma argumentativa e cria a motivação direta para o AODV-EN.
- Boa explicação do mecanismo central do AODV (descoberta de rota, rota reversa/direta, números de sequência para loop-freedom), apoiada na RFC 3561.
- Subseção 2.4.3 sobre soluções mesh existentes (ESP-WIFI-MESH, PainlessMesh, BRAM-NOW) posiciona o trabalho frente à literatura e justifica a lacuna a ser preenchida.

**Problemas:**

### [ ] C2-015 · 🔴 CRÍTICO · `completude`
- **Local:** Capítulo 2 inteiro (esp. seção 2.4 Classificação dos protocolos de roteamento)
- **Problema:** O algoritmo de flooding — baseline de comparação de todo o TCC (citado no resumo.tex, abstract.tex, capitulo_5.tex e capitulo_6.tex) — nunca é definido nem mencionado no referencial teórico. O leitor chega ao Cap. 5 sem fundamentação conceitual do método contra o qual o AODV-EN é avaliado. O conceito correlato de 'broadcast storm problem' (entrada ni1999 já presente na .bib, mas citada apenas no Cap. 4) também está ausente.
- **Correção:** Adicionar uma subseção (ex.: 2.4.4 'Disseminação por flooding') definindo flooding/inundação como estratégia de difusão por broadcast, suas variantes (puro vs. controlado), o problema da tempestade de broadcast (citar \cite{ni1999}) e por que serve de baseline de referência. Mover/ecoar a definição canônica que hoje aparece solta no Cap. 4 (linha 120).

### [ ] C2-016 · 🔴 CRÍTICO · `completude`
- **Local:** Capítulo 2 — ausência de seção sobre métricas; PDR aparece sem definição na subseção 2.3.2 (linha 239)
- **Problema:** As métricas que sustentam toda a avaliação experimental (PDR, NRL/Normalized Routing Load, latência fim-a-fim, consumo energético) não são definidas conceitualmente no referencial. PDR é usado de passagem (linha 239) como se já fosse conhecido. A literatura de métricas de roteamento (couto2003 ETX, draves2004, rfc6551, karlwillig2005, heinzelman2000) está toda relegada ao Cap. 4, deixando o Cap. 5 sem ancoragem teórica para os indicadores reportados.
- **Correção:** Inserir uma seção/subseção 'Métricas de avaliação de protocolos de roteamento' definindo formalmente PDR, NRL, latência e modelo de energia, com as fontes já disponíveis (\cite{couto2003,draves2004,rfc6551,karlwillig2005,heinzelman2000}). Definir PDR antes de usá-lo na linha 239.

### [ ] C2-017 · 🟠 ALTO · `abnt-formatacao`
- **Local:** Linhas 38, 129, 481 (referências no texto) e tabelas longtable nas linhas 41-72, 104-123, 133-164, 201-227, 337-365, 485-505
- **Problema:** As tabelas são referenciadas na prosa por número fixo ('O Quadro 1', 'O Quadro 3', 'O Quadro 6') em vez de \ref{}, e nenhuma das 6 tabelas possui \label. Se a numeração mudar (inserção/remoção de quadro ou pelo contador automático do \LTcaptype), o texto fica inconsistente. Pior: a numeração escrita já não bate — há 6 quadros, mas o texto pula para 'Quadro 3' e 'Quadro 6', sugerindo numeração manual frágil.
- **Correção:** Adicionar \label{tab:...} a cada longtable e substituir os números fixos por '\ref{tab:...}' (ou 'o Quadro \ref{...}'). Garantir que o contador 'quadro' seja consistente em todo o documento.

### [ ] C2-018 · 🟠 ALTO · `coesao`
- **Local:** Linhas 38, 129, 481 vs. os 6 quadros do capítulo
- **Problema:** Três dos seis quadros nunca são referenciados na prosa: Quadro 2 (Especificações do ESP32, linha 106), Quadro 4 (Características do ESP-NOW, linha 205) e Quadro 5 (Comparativo de protocolos, linha 343). Em texto acadêmico ABNT, todo elemento flutuante deve ser chamado e comentado no corpo do texto; tabelas 'órfãs' são apontadas em banca.
- **Correção:** Inserir frase de chamada para cada quadro não referenciado (ex.: 'A Tabela X sintetiza as especificações...', 'O Quadro Y compara as três categorias quanto a latência, overhead e memória'), idealmente com uma leitura interpretativa, não apenas 'ver Quadro X'.

### [ ] C2-019 · 🟠 ALTO · `referencia`
- **Local:** Linhas 75, 125, 166, 229, 367, 507 ('Fonte: ...')
- **Problema:** Várias afirmações factuais e numéricas são atribuídas apenas em linhas 'Fonte:' fora do fluxo do texto (ex.: 'Fonte: Adaptado de Priyadarshi et al. (2025) e Chai e Zeng (2021)'), e algumas fontes aparecem escritas por extenso ('Becker et al. (2025)') em vez de \cite/\citeonline. Isso mistura estilos de citação e enfraquece a rastreabilidade ABNT (autor-data deve ser gerado pela ferramenta, não digitado).
- **Correção:** Padronizar todas as atribuições via \citeonline/\cite (inclusive nas linhas 'Fonte:', usando \citeonline{...}). Evitar nomes de autor digitados manualmente como 'Becker et al. (2025)' na linha 229 — usar \citeonline{becker2025}.

### [ ] C2-020 · 🟡 MÉDIO · `referencia`
- **Local:** Linhas 23-24 (seção 2.1)
- **Problema:** Mark Weiser e o conceito de Computação Ubíqua (1991) são nomeados no texto sem citação, embora a entrada \cite{weiser1991} exista na .bib e seja usada no Cap. 1. Afirmação histórica relevante fica sem âncora bibliográfica neste capítulo.
- **Correção:** Acrescentar \cite{weiser1991} após 'antecipada por Mark Weiser em 1991 sob o conceito de Computação Ubíqua'.

### [ ] C2-021 · 🟡 MÉDIO · `completude`
- **Local:** Seção 2.4.1 (linhas 281-305) — conceito de multi-hop
- **Problema:** Embora 'multi-hop' seja explicado no contexto de redes mesh (linha 289), o termo já é usado antes — no título da subseção 2.3.3 (linha 252) e no texto da seção 2.3 — sem definição prévia. O conceito central de comunicação em múltiplos saltos aparece pela primeira vez como limitação do ESP-NOW antes de ser conceituado.
- **Correção:** Antecipar uma definição breve de comunicação multi-hop/multi-salto na seção 2.1 ou no início de 2.3, ou reordenar para que a conceituação (2.4.1) preceda o uso do termo nas limitações do ESP-NOW (2.3.3).

### [ ] C2-022 · 🟡 MÉDIO · `coesao`
- **Local:** Transições entre seções 2.1->2.2, 2.2->2.3, 2.3->2.4, 2.4->2.5
- **Problema:** As seções principais começam de forma abrupta ('O ESP32 é...', 'O ESP-NOW é...', 'As Redes Mesh...', 'O AODV é...'), sem frases-ponte que liguem o fim de uma seção ao início da seguinte. A coesão inter-seções depende só do parágrafo introdutório do capítulo, não de transições locais.
- **Correção:** Adicionar 1 frase de transição ao final/início de cada seção (ex.: ao fim de 2.2 sobre consumo do ESP32, ligar à necessidade de um protocolo de comunicação leve, introduzindo o ESP-NOW em 2.3).

### [ ] C2-023 · 🟡 MÉDIO · `tecnico`
- **Local:** Linha 216 (Quadro 'Padrão base: IEEE 802.11b') vs. linha 180 (texto: 'camada de enlace do padrão IEEE 802.11')
- **Problema:** Inconsistência entre o texto (que diz 'IEEE 802.11' genérico) e o quadro (que afirma 'IEEE 802.11b'). Em referencial teórico, divergências entre prosa e tabela sobre o padrão-base do protocolo central minam a credibilidade técnica.
- **Correção:** Unificar a designação do padrão (definir se é 802.11 genérico ou 802.11b especificamente) entre texto e Quadro, com citação da fonte que sustenta a escolha.

### [ ] C2-024 · ⚪ BAIXO · `escrita`
- **Local:** Seção 2.4.2, linhas 322-330 (parágrafo dos protocolos reativos)
- **Problema:** O parágrafo que descreve protocolos reativos é notavelmente mais longo e detalhado que os de proativos e híbridos, criando desequilíbrio. Embora o AODV seja reativo (e o foco do trabalho), aqui o objetivo é classificar as três categorias com paralelismo.
- **Correção:** Equilibrar os três parágrafos da subseção 2.4.2 (a justificativa de foco no reativo pode ser feita na transição para a seção 2.5, não inflando o item de classificação).

### [ ] C2-025 · ⚪ BAIXO · `escrita`
- **Local:** Linha 31-32 (seção 2.1)
- **Problema:** Frase longa com lista 'temperatura, umidade, pressão, movimento ou luminosidade' sem vírgula antes do 'e transmitir', dificultando a leitura ('...ou luminosidade e transmitir os dados...').
- **Correção:** Inserir vírgula/reorganizar: '...ou luminosidade, e transmitir os dados coletados...' ou fechar a enumeração entre parênteses.

### [ ] C2-026 · ⚪ BAIXO · `tecnico`
- **Local:** Linha 169 (seção 2.2)
- **Problema:** Preço em reais ('R$ 25,00 e R$ 40,00') sem data de referência/cotação. Valores monetários datam rapidamente e, sem ano-base, comprometem a atualidade da afirmação ao longo do tempo.
- **Correção:** Acrescentar ano de referência da cotação (ex.: 'em valores de 2025') e/ou citar fonte de preço, ou converter para faixa em dólar com a mesma data-base.

### [ ] C2-027 · ⚪ BAIXO · `argumentacao`
- **Local:** Seção 2.2 (ESP32) — falta de fechamento crítico
- **Problema:** A seção descreve o ESP32 de forma majoritariamente expositiva (specs e modos de energia) e termina na questão de custo (linha 168-172), sem amarrar explicitamente por que essas características (dual-core, modos de sleep, baixo custo) habilitam especificamente uma rede de sensores roteável — a ponte argumentativa para o restante do TCC fica implícita.
- **Correção:** Acrescentar frase de fechamento conectando as capacidades do ESP32 (processamento para rodar lógica de roteamento + modos de baixo consumo) à viabilidade de implementar um protocolo como o AODV-EN no dispositivo.

---

## 3. Capítulo 3 — Metodologia  ·  nota 72

_Capítulo metodologicamente bem estruturado e com escrita academica em geral clara e fluente: a caracterizacao da pesquisa, a fundamentacao da DSR (Hevner, Simon, Peffers, March e Smith) e o mapeamento fases-etapas estao solidos e bem citados. As maiores fraquezas nao estao nos numeros (que mudarao), mas na FORMA: (1) inconsistencia de tempo verbal grave — as fases sao narradas no passado (trabalho ja feito), mas as secoes de Metricas, Flooding e Coleta/Analise estao no futuro ("sera implementado", "serao coletados"), como se nada tivesse sido executado, embora o Capitulo 5 ja reporte resultados; (2) contradicao interna entre o Quadro 9 (30 repeticoes) e o texto/Cap. 5 (6 repeticoes reais), e a promessa de IC95 nunca cumprida no Cap. 5; (3) todas as referencias cruzadas sao numeros fixos digitados a mao ("Quadro 7", "Figura 1", "Secao 3.4", "Capitulo 5, Secao 5.4") em vez de \ref/\label, o que e fragil e quebra na recompilacao; (4) formulas das metricas escritas como texto puro dentro das celulas, sem modo matematico LaTeX. Corrigidos esses pontos de forma e coerencia temporal, o capitulo chega facilmente a faixa 85+._

**Pontos fortes:**
- Fundamentacao teorica da DSR robusta e bem ancorada (Hevner 2004, Simon 1996, Peffers 2007, March e Smith 1995), com o artefato corretamente classificado como metodo + instanciacao
- Mapeamento explicito das cinco fases do trabalho as seis etapas do processo DSR de Peffers, reforcado por quadro e por figura de fluxo (boa rastreabilidade metodologica)
- Justificativa do Flooding como baseline e tecnicamente bem argumentada em quatro eixos (simplicidade/limite inferior, PDR maximo, overhead extremo, ausencia de estado), deixando claro o papel de cada propriedade na comparacao
- Definicao das quatro metricas (PDR, latencia, NRL, energia) com semantica clara e justificativa de selecao ligada a literatura de redes ad-hoc/WSN
- Especificacao do Flooding detalhada e operacional (deteccao de duplicatas por buffer circular, controle de TTL, broadcast de enlace), preservando a definicao classica de uma retransmissao por no
- Texto coeso e em registro academico formal, com bom encadeamento de conectivos entre paragrafos e nota de rodape justificando os desvios em relacao a RFC 3561

**Problemas:**

### [ ] C3-028 · 🔴 CRÍTICO · `coerencia`
- **Local:** Secao 3.4.2, Quadro 9 (Parametros dos experimentos), linha 439 — 'Repetições por cenário & 30 & 30'
- **Problema:** O quadro declara 30 repeticoes por cenario e a Secao 3.5 (linha 507-508) promete media, desvio padrao e IC95, mas o restante do proprio capitulo (linhas 318-319, 358-360) e o Capitulo 5 (linhas 23, 263) afirmam que apenas 6 repeticoes foram efetivamente executadas, em um unico cenario reduzido, sem IC95 reportado. Ha contradicao direta entre o parametro planejado e o que foi feito, e uma promessa estatistica (IC95) nao cumprida.
- **Correção:** Reconciliar o planejado com o executado: ou (a) manter 30 como meta de planejamento, mas adicionar nota explicita no quadro/texto deixando claro que a campanha real teve 6 repeticoes por limitacao de hardware, com remissao a Secao/Capitulo onde isso e tratado; ou (b) ajustar o valor. Em qualquer caso, alinhar a promessa de IC95 da Secao 3.5 com o que o Cap. 5 realmente entrega (se nao houver IC95 calculado, reformular para 'media e desvio padrao', justificando a ausencia de IC por n pequeno).

### [x] C3-029 · 🟠 ALTO · `coerencia`
- **Local:** Secoes 3.5 (Metricas), 3.6 (Flooding) e 3.7 (Coleta e Analise) — verbos no futuro: 'sera realizada' (l.458), 'sera implementado' (l.514), 'serao coletados' (l.564), 'serao calculados' (l.507), 'permitira' (l.555/627)
- **Problema:** Inconsistencia de tempo verbal dentro do mesmo capitulo. As Fases 1-5 (3.3.1-3.3.5) estao no passado ('foi realizado', 'correspondeu', 'deu continuidade', 'materializando'), descrevendo trabalho concluido, mas tres secoes inteiras estao no futuro, como se o experimento ainda nao tivesse ocorrido — quando o Capitulo 5 ja apresenta os resultados. Isso confunde o leitor sobre o que foi planejado versus realizado e soa como capitulo escrito antes da execucao e nao revisado.
- **Correção:** Padronizar o tempo verbal de todo o capitulo. Como o trabalho ja foi executado, converter para passado/presente de descricao metodologica: 'A avaliacao foi realizada por meio de...', 'foi implementado um algoritmo de referencia...', 'os dados foram coletados...'. Manter coerencia com o tom retrospectivo das Fases.

### [x] C3-030 · 🟠 ALTO · `abnt-formatacao`
- **Local:** Todo o capitulo — 'Quadro 7' (l.78), 'Quadro 8' (l.156), 'Figura 1' (l.208), 'Figura 2' (l.621), 'Secao 3.4' (l.318), 'Capitulo 5, Secao 5.4' (l.361)
- **Problema:** Todas as referencias a quadros, figuras, secoes e capitulos usam numeros fixos digitados manualmente, sem \label/\ref. Os quadros usam \caption mas nenhum tem \label, e nenhum numero e gerado automaticamente. Qualquer insercao/remocao de um quadro ou figura quebra silenciosamente toda a numeracao e as remissoes, alem de tornar a citacao 'Capitulo 5, Secao 5.4' fragil a reorganizacao.
- **Correção:** Adicionar \label{quad:dsr-etapas}, \label{fig:fluxo-metodologico} etc. a cada elemento flutuante e a cada \section/\subsection, e substituir os numeros literais por \ref{}/\autoref{}. Conferir que a numeracao gerada de fato corresponde (o 'Quadro 7' sugere que ha 6 quadros em capitulos anteriores — confirmar continuidade).

### [ ] C3-031 · 🟡 MÉDIO · `abnt-formatacao`
- **Local:** Secao 3.5, Quadro 10 (Metricas), coluna Formula, linhas 489-501
- **Problema:** As formulas das quatro metricas estao escritas como texto puro dentro das celulas ('PDR = (Pacotes recebidos / Pacotes enviados) × 100%', 'L = t_recepcao - t_envio', 'E = Σ(N_tx × E_tx + N_rx × E_rx + t_idle × P_idle)'). Variaveis nao ficam em italico, o subscrito aparece como 't\_envio' (underscore literal), e o sinal de multiplicacao mistura '×' com nada — formatacao impropria para texto academico.
- **Correção:** Tipografar em modo matematico LaTeX, idealmente como equacoes numeradas referenciaveis: $PDR = \frac{P_{rec}}{P_{env}} \times 100\%$, $L = t_{rec} - t_{env}$, $NRL = \frac{P_{ctrl}}{P_{dados}}$, $E = \sum (N_{tx}E_{tx} + N_{rx}E_{rx} + t_{idle}P_{idle})$. Definir cada simbolo logo apos a equacao.

### [ ] C3-032 · 🟡 MÉDIO · `completude`
- **Local:** Secao 3.4.1 (Cenarios), Quadro 8 vs execucao real — C1 'Linear, 5 nos' (l.384) mas Quadro 9 'Numero de nos 5-10' (l.434) e texto 'C1-3n, tres nos' (l.358)
- **Problema:** Ha tres definicoes diferentes de 'numero de nos' coexistindo sem ponte explicita: os cenarios C1-C4 do Quadro 8 (5/7/6/5 nos), o parametro generico '5-10 conforme cenario' do Quadro 9, e o cenario realmente executado 'C1-3n' com 3 nos. O leitor nao consegue mapear o que e planejamento, o que e o intervalo de variacao e o que foi de fato medido. O '5-10' tambem nao bate com os valores dos cenarios (que vao de 5 a 7).
- **Correção:** Tornar explicita a distincao entre cenarios PLANEJADOS (C1-C4) e cenario EXECUTADO em hardware (C1-3n), e corrigir o intervalo '5-10' para refletir os valores reais dos cenarios definidos (p.ex. '5-7' ou justificar o teto de 10). Idealmente uma frase de transicao no inicio da Secao 3.4 explicando o estatuto de cada quadro.

### [ ] C3-033 · 🟡 MÉDIO · `argumentacao`
- **Local:** Secao 3.3.3 (Fase 3, l.306-309) e Secao 3.4.1 (l.357-361)
- **Problema:** A limitacao central do trabalho — campanha em hardware restrita a 3 nos/6 repeticoes e demais cenarios 'avaliados por simulacao' — e mencionada de passagem, mas o capitulo de metodologia nao descreve a simulacao em si: que simulador/modelo foi usado, como o modelo foi calibrado/validado contra os dados de hardware, quais pressupostos. Para um capitulo de metodologia, a ausencia de uma subsecao sobre o metodo de simulacao compromete a reprodutibilidade da maior parte dos resultados.
- **Correção:** Acrescentar uma subsecao (ou paragrafo substancial) descrevendo o ambiente de simulacao: ferramenta/abordagem, modelo de propagacao/energia adotado, parametros, e como a simulacao foi ancorada nos 6 seeds de hardware (calibracao/validacao). Sem isso, a frase 'avaliados por simulacao' fica como caixa-preta metodologica.

### [ ] C3-034 · 🟡 MÉDIO · `tecnico`
- **Local:** Secao 3.6.2, item b (l.547-549) — 'Garantia de entrega: ... o Flooding garante que o pacote alcancara o destino ... servindo como referencia de PDR maximo alcancavel'
- **Problema:** Afirmacao tecnicamente forte demais sem ressalva. Flooding so garante entrega na ausencia de colisoes/perdas; em ESP-NOW sobre broadcast, o flooding tipicamente sofre o problema da tempestade de broadcast (broadcast storm), com colisoes que DERRUBAM o PDR em redes densas — ou seja, pode nao ser o 'PDR maximo alcancavel' na pratica. O proprio trabalho depende de comparar PDR, entao tratar o baseline como teto teorico de entrega e arriscado.
- **Correção:** Qualificar: 'em redes conectadas e na ausencia de perdas por colisao, o Flooding tende a maximizar a probabilidade de entrega, servindo como referencia superior de cobertura'. E reconhecer o efeito broadcast storm como limitacao do baseline (isso fortalece, em vez de enfraquecer, a comparacao a favor do AODV-EN).

### [ ] C3-035 · ⚪ BAIXO · `coerencia`
- **Local:** Secao 3.6.1, item b (l.527-529) 'buffer circular com os ultimos N identificadores (N=100)' vs Secao 3.4.2, Quadro 9 'Cache de peers 8 entradas'
- **Problema:** Dois tamanhos de buffer distintos (100 para deteccao de duplicatas do Flooding; 8 para cache de peers do AODV-EN) aparecem proximos sem deixar claro que sao estruturas de propositos diferentes. Risco de o leitor inferir inconsistencia.
- **Correção:** Nenhuma mudanca de valor necessaria; apenas garantir que cada numero esteja inequivocamente associado a sua estrutura/algoritmo (p.ex. 'buffer de deteccao de duplicatas do Flooding (N=100)' vs 'cache LRU de peers do AODV-EN (8 entradas)').

### [ ] C3-036 · ⚪ BAIXO · `escrita`
- **Local:** Secao 3.7, ultima frase (l.637-640) — 'sera avaliado se o AODV-EN, apresenta ganhos em eficiencia'
- **Problema:** Virgula indevida separando sujeito ('o AODV-EN') do verbo ('apresenta'). Erro de pontuacao.
- **Correção:** Remover a virgula: 'sera avaliado se o AODV-EN apresenta ganhos em eficiencia...'. (E ajustar o verbo ao tempo padronizado conforme item de tempo verbal.)

### [ ] C3-037 · ⚪ BAIXO · `escrita`
- **Local:** Secao 3.1 (l.35) 'obtidos atraves de medicoes' e demais ocorrencias de 'atraves de' (l.333)
- **Problema:** Uso de 'atraves de' com sentido de 'por meio de' e considerado impreciso/coloquial em registro academico formal ABNT (estritamente significa 'de um lado a outro').
- **Correção:** Substituir 'atraves de' por 'por meio de' / 'mediante' nas duas ocorrencias.

### [ ] C3-038 · ⚪ BAIXO · `abnt-formatacao`
- **Local:** Fonte dos quadros — 'Fonte: Elaborado pelos autores' (l.206, 400, 449, 505)
- **Problema:** Atribuicao no plural ('autores'). Verificar se o TCC tem mais de um autor; em TCC de autor unico deve ser 'pelo autor'. Inconsistencia de autoria entre capa e atribuicoes prejudica a formalidade.
- **Correção:** Confirmar o numero de autores e uniformizar para 'pelo autor' ou 'pelos autores' em todo o documento.

### [ ] C3-039 · ⚪ BAIXO · `coesao`
- **Local:** Secao 3.6 inicio (l.514) 'sera implementado um algoritmo de referencia baseado em Flooding' — repeticao
- **Problema:** A expressao 'algoritmo de referencia baseado em Flooding' aparece de forma quase identica multiplas vezes em sequencia proxima (l.343-344, 514-515, 523, 623, 633), tornando a leitura repetitiva.
- **Correção:** Apos a primeira definicao, alternar com formas reduzidas ('o baseline Flooding', 'o Flooding', 'o algoritmo de referencia') para reduzir redundancia lexical.

---

## 4. Capítulo 4 — Projeto e Implementação do AODV-EN  ·  nota 78

_Capítulo tecnicamente sólido e bem escrito, com registro acadêmico formal adequado e fidelidade notável entre texto e código: verifiquei na implementação (firmware/components/aodv_en) que a métrica híbrida (Custo = α·HopCount + β·Penalidade(RSSI), α=8/β=1, RSSI_best=-55/worst=-90 dBm), a EWMA fator 3/4 (aodv_en_neighbors.c: (avg*3+novo)/4), a política LRU com peers pinned (aodv_en_peers.c) e o cache de 8 entradas (AODV_EN_PEER_CACHE_SIZE=8) estão de fato implementados e ativos por padrão (Kconfig rssi_weight default=1; campaign_compare.c roda os perfis hop_only/hybrid_default/hybrid_rssi_bias). Não há promessas de design falsas — as afirmações do texto correspondem ao artefato. Os principais problemas são de forma, não de substância: (1) referências cruzadas de figuras e quadros estão hardcoded ("Figura 3", "Quadro 12") em vez de \ref/\label, criando risco real de numeração errada e inconsistência com as equações que usam \ref; (2) duplicação substancial — as equações de custo e penalidade aparecem duas vezes idênticas (eq:custo/eq:custo_impl, eq:penalidade/eq:penalidade_impl) e as subseções de flooding, métrica híbrida e gerenciamento de peers reescrevem o mesmo conteúdo na Seção 4.2 (Decisões) e na 4.7 (Adaptações); (3) a Seção 4.2.4 (Estimativa de consumo energético) está deslocada — introduz modelo de energia sem fechar a equação para o ESP32 e sem conectar ao restante do capítulo. Ajustando essas questões estruturais o capítulo sobe facilmente para a faixa 88-92._

**Pontos fortes:**
- Fidelidade texto-código exemplar: todas as decisões de projeto descritas (métrica híbrida, LRU, pinned peers, EWMA 3/4, broadcast flooding com cache de duplicatas e TTL) foram confirmadas como implementadas e ativas no firmware; nenhuma promessa de design é vaporware
- Justificativa metodológica forte e honesta no descarte do flooding por unicast sequencial (Seção 4.2.2 e 4.7.2): explica o viés de retransmissão MAC CTS-ACK do unicast ESP-NOW e a necessidade de simetria com o algoritmo de referência, ancorada em ni1999 e RFC 3561
- Boa progressão didática: arquitetura em camadas -> decisões de projeto -> estrutura modular -> estruturas de dados -> mensagens -> funcionamento -> adaptações, conforme anunciado na introdução do capítulo
- Embasamento bibliográfico adequado para a métrica composta (couto2003/ETX, draves2004/WCETT, rfc6551 para EWMA), situando a escolha de projeto na literatura em vez de apresentá-la como ad hoc
- Registro acadêmico formal PT-BR consistente, uso correto de itálico para termos estrangeiros (\emph) e quadros bem estruturados com fonte declarada

**Problemas:**

### [x] C4-040 · 🟠 ALTO · `abnt-formatacao`
- **Local:** Todo o capítulo — linhas 24, 225, 275, 388, 484, 578, 632 ("Figura 3", "Figura 4", "Quadro 12", "Quadro 13", "Figura 5", "Figura 6", "Quadro 14")
- **Problema:** Todas as referências cruzadas a figuras e quadros usam números fixos digitados manualmente, enquanto as equações usam \ref corretamente (linhas 147, 156, 169). Além de inconsistente, isso quase garante numeração errada na versão final: as figuras nem têm \label, e qualquer inserção de figura/quadro em capítulos anteriores desalinha todas as chamadas. O "Figura 3/4/5/6" e "Quadro 12/13/14" sugerem numeração global manual que não acompanha o LaTeX.
- **Correção:** Adicionar \label{fig:...} em cada \caption de figura e quadro e trocar todas as menções por \ref (ex.: "A Figura~\ref{fig:arquitetura} apresenta..."). Usar \autoref ou o padrão já adotado nas equações para uniformizar. Nunca digitar o número.

### [ ] C4-041 · 🟠 ALTO · `coesao`
- **Local:** Seção 4.2 (Decisões de Projeto, linhas 76-183) vs Seção 4.7 (Adaptações, linhas 619-771)
- **Problema:** Há duplicação substancial de conteúdo entre as duas seções: flooding controlado (4.2.2 e 4.7.2), métrica híbrida (4.2.3 e 4.7.4) e gerenciamento de peers/LRU (4.2.1 e 4.7.3) são explicados duas vezes, em parte com as mesmas frases (ex.: o argumento do unicast/CTS-ACK aparece quase idêntico em 113-124 e 699-710). Isso infla o capítulo, cansa o leitor e dilui a fronteira entre "decisão de projeto" e "adaptação".
- **Correção:** Definir um papel distinto para cada seção: 4.2 fundamenta e justifica a decisão (o "porquê", com a literatura); 4.7 descreve sucintamente o resultado da adaptação (o "o quê") e remete a 4.2 via \ref em vez de reexplicar. Eliminar a reescrita do argumento do unicast em 4.7.2, substituindo por "conforme discutido na Seção~\ref{...}".

### [ ] C4-042 · 🟠 ALTO · `coesao`
- **Local:** Equações: linhas 149-151/743-745 (custo) e 158-164/753-759 (penalidade)
- **Problema:** As equações de custo (eq:custo e eq:custo_impl) e de penalidade (eq:penalidade e eq:penalidade_impl) são reproduzidas literalmente, idênticas, em duas seções. Duas equações numeradas iguais no mesmo capítulo são um defeito editorial: o leitor questiona qual é a "oficial" e por que diferem (não diferem).
- **Correção:** Apresentar cada equação uma única vez (preferencialmente na Seção 4.2.3, onde é fundamentada) e, na Seção 4.7.4, referenciá-la por \ref em vez de reescrevê-la. Remover eq:custo_impl e eq:penalidade_impl.

### [ ] C4-043 · 🟡 MÉDIO · `coerencia`
- **Local:** Seção 4.2.4 — Estimativa de consumo energético (linhas 185-207)
- **Problema:** A subseção destoa do capítulo: o capítulo trata de PROJETO/IMPLEMENTAÇÃO do AODV-EN, mas esta seção introduz um modelo analítico de energia (heinzelman2000, E_tx = P_tx·t_tx) que não é uma decisão de projeto do protocolo nem é amarrado à implementação. A equação fica genérica (não há valores de V, I_tx, t_tx para o ESP32) e a seção não conclui como o consumo é estimado no artefato. Parece material de metodologia de avaliação inserido no lugar errado.
- **Correção:** Decidir o papel: se a estimativa de energia é parte do método de avaliação, mover para o capítulo de metodologia/resultados. Se permanecer aqui, fechar o raciocínio com os parâmetros concretos adotados para o ESP32 (corrente de TX/RX, tensão, tempo por quadro) e explicitar como o firmware/simulação contabiliza energia, conectando à implementação.

### [ ] C4-044 · 🟡 MÉDIO · `completude`
- **Local:** Seção 4.2.3 / 4.7.4 — definição da penalidade quando não há vizinho ativo
- **Problema:** O texto afirma que a métrica considera a qualidade do enlace, mas não menciona o comportamento de borda implementado: quando o próximo salto não é um vizinho ativo, o código usa RSSI_worst (-90 dBm), ou seja, penalidade máxima 100 (aodv_en_node.c, aodv_en_node_route_metric_pick_rssi retorna WORST quando neighbor==NULL ou state!=ACTIVE). Esse default penaliza rotas sem RSSI conhecido e afeta a seleção — é uma decisão de projeto relevante e omitida.
- **Correção:** Acrescentar uma frase explicando que, na ausência de RSSI confiável para o próximo salto (vizinho não ativo), aplica-se penalidade máxima como política conservadora, e justificar a escolha. Isso aumenta a fidelidade e a completude da descrição da métrica.

### [ ] C4-045 · 🟡 MÉDIO · `escrita`
- **Local:** Quadro 13 (linha 422) — mensagem ACK classificada como "Controle/aplicação"
- **Problema:** O tipo da mensagem ACK é descrito de forma ambígua ("Controle/aplicação") sem que o texto subsequente (linhas 445-450) esclareça a distinção. Como o próprio capítulo enfatiza que a confirmação é fim-a-fim em nível de aplicação (linha 710), classificá-la também como "Controle" gera inconsistência conceitual com o resto do texto.
- **Correção:** Padronizar a classificação: se a confirmação é fim-a-fim em nível de aplicação (como afirma 4.7.2), rotular o ACK como "Aplicação" e remover a barra "Controle/". Garantir que a coluna Tipo do quadro seja coerente com a descrição textual.

### [ ] C4-046 · 🟡 MÉDIO · `tecnico`
- **Local:** Seção 4.6.1 / Figura 5 (linhas 479-481) — RREP a partir de nó intermediário com rota válida
- **Problema:** O texto afirma que o RREP é gerado "quando a mensagem RREQ alcança o destino, ou um nó que possua rota válida para ele" (intermediate route reply). É preciso confirmar que esse caminho está implementado; se o firmware só gera RREP no destino final (gratuitous/intermediate reply não implementado), a afirmação seria uma promessa de design não cumprida. A Figura 5 só ilustra o caso do destino, o que pode mascarar a divergência.
- **Correção:** Verificar no código (aodv_en_node.c, tratamento de RREQ) se há geração de RREP por nó intermediário com rota válida e número de sequência suficiente. Se não houver, remover a cláusula "ou um nó que possua rota válida" para não afirmar funcionalidade ausente; se houver, considerar ilustrá-la ou citá-la explicitamente.

### [ ] C4-047 · ⚪ BAIXO · `escrita`
- **Local:** Linha 137-138 — sigla RSSI e linha 169 EWMA
- **Problema:** A sigla RSSI é expandida ("Received Signal Strength Indicator") apenas na Seção 4.2.3 (linha 137), mas o termo "RSSI" já havia sido implicitamente tratado antes; já EWMA é expandida na linha 169. Convém garantir que cada sigla seja definida na primeira ocorrência no corpo do capítulo e usada de forma consistente depois, conforme norma ABNT.
- **Correção:** Revisar a primeira ocorrência de cada sigla no capítulo (RSSI, EWMA, LRU, TTL, MAC, ETX, WCETT) e expandi-la apenas na primeira menção, usando a sigla isolada nas seguintes. Padronizar.

### [ ] C4-048 · ⚪ BAIXO · `escrita`
- **Local:** Linhas 86 e 367 — repetição da definição de AODV_EN_PEER_CACHE_SIZE=8
- **Problema:** A informação "cache de peers com 8 entradas, configurável, parâmetro AODV_EN_PEER_CACHE_SIZE" aparece quase idêntica em 4.2.1 (linha 86-92) e em 4.4.4 (linha 366-373), reforçando o problema geral de redundância entre seções de decisão e de estrutura.
- **Correção:** Definir o parâmetro e seu valor uma única vez (na decisão de projeto) e, na Seção 4.4.4, descrever apenas o mecanismo (separação vizinhança lógica vs peers físicos, LRU, pinned) sem repetir o valor/nome do macro, ou remeter via \ref.

### [ ] C4-049 · ⚪ BAIXO · `escrita`
- **Local:** Linha 153 e 747 — apresentação dos pesos α e β
- **Problema:** Os valores padrão dos pesos (α=8, β=1) são apresentados duas vezes; além da redundância já apontada, na linha 747 os nomes dos macros (AODV_EN_ROUTE_METRIC_HOP_WEIGHT/RSSI_WEIGHT) só aparecem na segunda ocorrência, criando assimetria informacional entre as duas exposições da mesma equação.
- **Correção:** Consolidar a apresentação dos pesos e dos nomes de parâmetros configuráveis em um único ponto (na fundamentação da métrica), evitando que metade da informação esteja em uma seção e metade em outra.

---

## 5. Capítulo 5 — Resultados e Discussão  ·  nota 74

_Capítulo bem estruturado e, no geral, bem escrito: a sequência (ambiente/validação → resultados de hardware → discussão por métrica → escalabilidade por simulação → conformidade RFC → escopo/limitações) é lógica e cobre as quatro métricas prometidas na metodologia. O grande mérito é a honestidade metodológica: a seção de limitações é exemplar (amostra reduzida, topologia de um salto, quantização da latência, energia estimada e não medida, campanhas coletadas em momentos distintos) e há transparência rara ao admitir que a vantagem de PDR do flooding em campanha preliminar vinha da retransmissão do unicast ESP-NOW, não de mérito do algoritmo — isso protege a credibilidade. A discussão interpreta (não apenas descreve): atribui causas (descoberta reativa, ausência de ACK de enlace, custo estrutural de re-disseminação) e fecha o raciocínio ligando o cruzamento de transmissões/entrega à hipótese central. Os problemas mais graves são de FORMA e RASTREABILIDADE, não de conteúdo: várias referências cruzadas estão erradas (o capítulo se autodenomina "Capítulo 5" mas aponta para conteúdo que está em outros capítulos; "Seção 4.4" e "Seção 6.4" não batem com a numeração real), figuras/quadros/tabelas são numerados manualmente em vez de \ref/\label (com dois esquemas conflitantes — "Quadro 15" e "Tabela 1" no mesmo capítulo), e há um neologismo informal ("flodada/flodadas") inadequado ao registro acadêmico. A ligação resultados→objetivos é parcial: o objetivo específico (e) (comparação com a literatura correlata) não é retomado, e a discussão não problematiza o tamanho amostral ao afirmar equivalência estatística sem teste de hipótese declarado._

**Pontos fortes:**
- Seção de limitações honesta e completa (cinco limitações concretas: amostra, topologia 1-salto, quantização de latência, energia estimada, coletas em momentos distintos) — fortalece a credibilidade e antecipa a banca.
- Transparência metodológica rara: admite explicitamente que a vantagem de PDR do flooding em campanha preliminar decorria da retransmissão do unicast ESP-NOW e não de mérito do algoritmo, justificando a adoção de transporte simétrico por broadcast.
- Discussão interpreta os números em vez de só descrevê-los: cada métrica recebe explicação causal (descoberta reativa, ausência de ACK de enlace, custo estrutural de re-disseminação) e a razão RX/TX≈1,98 é ligada à topologia (dois vizinhos por broadcast).
- Separação clara entre escopo implementado e escopo projetado (LRU e métrica híbrida saltos+RSSI declaradas como trabalho futuro), evitando que o leitor atribua ao protótipo capacidades não avaliadas.
- Seção de conformidade RFC 3561 enumera ajustes pontuais e concretos (dedup de DATA, contagem de ACK, condição de RERR, delete period, proteção contra rebaixamento de rota), ancorando a implementação na especificação.

**Problemas:**

### [x] C5-050 · 🔴 CRÍTICO · `referencia`
- **Local:** Seção 5.5 (Escopo e Limitações), linha 258: "embora projetadas e descritas no Capítulo 5"
- **Problema:** O capítulo é o próprio Capítulo 5 (RESULTADOS E DISCUSSÃO) e remete a si mesmo, mas as adaptações LRU e métrica híbrida (saltos+RSSI) estão descritas no Capítulo 4 (PROJETO E IMPLEMENTAÇÃO), confirmado em capitulo_4.tex (subseções 'Gerenciamento de peers com política LRU' e 'Métrica híbrida'). A referência envia o leitor ao lugar errado e revela que o autor perdeu a noção da posição do capítulo.
- **Correção:** Trocar 'descritas no Capítulo 5' por 'descritas no Capítulo 4' e, idealmente, usar \ref ao label do capítulo/seção de projeto em vez de número fixo.

### [x] C5-051 · 🔴 CRÍTICO · `referencia`
- **Local:** Seção 5.1, linha 22 ("planejamento experimental (Seção 4.4)") e linha 26 ("avaliados por simulação (Seção 6.4)")
- **Problema:** Ambas as referências cruzadas estão incorretas para a numeração real. O planejamento experimental está em capitulo_3.tex (Capítulo 3 — METODOLOGIA, seção 'Planejamento Experimental'), não na 'Seção 4.4'. A análise por simulação é a Seção 5.4 deste próprio capítulo (não 'Seção 6.4'). Referências erradas a seções comprometem a navegação e a credibilidade do trabalho perante a banca.
- **Correção:** Substituir por \ref a labels reais: o planejamento aponta para o Capítulo 3 (METODOLOGIA / Planejamento Experimental) e a simulação aponta para a Seção 5.4 (\ref{anuxe1lise-de-escalabilidade-por-simulauxe7uxe3o}). Eliminar todos os números de seção digitados à mão.

### [x] C5-052 · 🟠 ALTO · `abnt-formatacao`
- **Local:** Todo o capítulo: 'Quadro 15' (l.57/71), 'Tabela 1' (l.107/112), 'Figura 8/9/10/11' (l.128, 138, 173, 206)
- **Problema:** Numeração de quadros, tabelas e figuras digitada manualmente, sem \label/\ref. Há ainda incoerência de esquema: convivem 'Quadro 15' e 'Tabela 1' (numerações de partida totalmente distintas) e a ABNT pede tratamento consistente de quadros vs. tabelas. Qualquer reordenação quebra a numeração; e 'Quadro 15' sugere continuidade de outros capítulos enquanto 'Tabela 1' reinicia, o que confunde o leitor.
- **Correção:** Atribuir \caption + \label a cada flutuante e referenciá-los no texto com \ref (ex.: 'O Quadro~\ref{quad:ledger} apresenta...'). Padronizar a decisão Quadro vs. Tabela e deixar o LaTeX numerar automaticamente.

### [x] C5-053 · 🟠 ALTO · `escrita`
- **Local:** Seção 5.3 (Latência), l.170 'confirmações (ACK) flodadas de volta'; Seção 5.4 (Energia), l.194 'cada disseminação é re-flodada por todos os nós'
- **Problema:** 'flodadas' e 're-flodada' são neologismos/aportuguesamentos informais de 'flood', inadequados ao registro acadêmico formal PT-BR. Comprometem o tom científico do capítulo.
- **Correção:** Substituir por construções formais: 'confirmações (ACK) disseminadas de volta por flooding' e 'cada disseminação é retransmitida por todos os nós (re-inundação)' ou 'redisseminada por flooding'. Manter 'flooding' em itálico como termo técnico, sem conjugá-lo em português.

### [ ] C5-054 · 🟠 ALTO · `argumentacao`
- **Local:** Seção 5.3 (PDR), l.150-152: "entrega estatisticamente equivalente... diferença inferior a um desvio padrão"
- **Problema:** Afirma-se 'equivalência estatística' sem teste de hipótese declarado; 'diferença inferior a um desvio padrão' não é critério de equivalência estatística (não controla tamanho amostral nem variância da diferença). Com a metodologia prevendo 30 repetições e tendo apenas 6, a afirmação de 'equivalência' é metodologicamente frágil e a banca pode contestar.
- **Correção:** Ou (a) declarar o teste aplicado (ex.: teste t / TOST de equivalência, com p-valor/intervalo) e os pressupostos, ou (b) suavizar a redação para 'as médias situam-se dentro da dispersão observada, sem diferença prática perceptível neste tamanho de amostra', remetendo o teste formal à campanha ampliada nas limitações.

### [ ] C5-055 · 🟡 MÉDIO · `completude`
- **Local:** Capítulo inteiro — ligação resultados→objetivos
- **Problema:** O capítulo não retoma explicitamente os objetivos específicos. Em particular, o objetivo (e) (Cap.1, l.164: 'Comparar os resultados obtidos com trabalhos correlatos da literatura') não é abordado em nenhum momento da discussão. O leitor não consegue mapear quais objetivos foram atingidos pelos resultados.
- **Correção:** Adicionar, ao final da discussão ou no início da seção de escopo, um parágrafo curto amarrando cada resultado ao objetivo específico correspondente (PDR/latência/NRL/energia ↔ objetivo d; conformidade ↔ objetivo c) e ou cumprir o objetivo (e) com um cotejo mínimo à literatura, ou declarar explicitamente que esse cotejo é feito no Capítulo de Conclusão/Trabalhos Correlatos.

### [ ] C5-056 · 🟡 MÉDIO · `coerencia`
- **Local:** Seção 5.5, l.252-260 vs. objetivos específicos (Cap.1, l.150-153)
- **Problema:** O objetivo específico (b) inclui explicitamente 'gerenciamento de peers com política LRU... e métrica híbrida de roteamento' como parte da proposta. O capítulo de resultados declara essas duas adaptações como não implementadas / trabalho futuro. Essa lacuna entre projetado e avaliado é declarada, mas não é problematizada quanto ao cumprimento do objetivo — fica a impressão de objetivo parcialmente não atendido sem que o texto reconheça e enquadre isso.
- **Correção:** Enquadrar explicitamente: esclarecer que o objetivo (b) foi atendido no nível de PROJETO (Cap.4) e que a AVALIAÇÃO dessas adaptações ficou fora do escopo experimental, distinguindo 'projetar' de 'avaliar' para não dar a impressão de objetivo descumprido.

### [ ] C5-057 · 🟡 MÉDIO · `tecnico`
- **Local:** Seção 5.2, l.57-61 e Tabela 1 (l.114) — campanhas comparadas
- **Problema:** O texto afirma que flooding e AODV-EN usam parâmetros idênticos e transporte 'simétrico por broadcast', mas a limitação (v) (l.271-273) revela que as campanhas foram coletadas 'em momentos distintos, sujeitas a variações nas condições de canal'. A comparação direta de médias na Tabela 1 (inclusive a coluna Δ) é apresentada como se fosse pareada/simultânea, o que não condiz com a coleta assíncrona. A força comparativa fica enfraquecida e isso só aparece no fim do capítulo.
- **Correção:** Antecipar a ressalva: ao introduzir a Tabela 1 (l.107), inserir uma frase reconhecendo que as campanhas não foram simultâneas e que a comparação assume condições de canal comparáveis, remetendo à limitação (v). Considerar reportar a coluna Δ como 'diferença observada' e não como efeito controlado.

### [ ] C5-058 · 🟡 MÉDIO · `argumentacao`
- **Local:** Seção 5.4 (Análise de Escalabilidade), l.204-214
- **Problema:** A conclusão de escalabilidade (cruzamento em ~9-11 nós; flooding deixa de alcançar destinos além de 5 saltos com TTL=5) é central para a tese, mas baseia-se inteiramente em simulação cuja fidelidade/validação não é discutida aqui — apenas se mencionou na Seção 5.1 que houve 'simulações determinísticas'. Não se declara o modelo de canal/perdas da simulação (parece ideal, já que 'ambos entregam a totalidade dos pacotes'), o que limita a generalização para hardware real.
- **Correção:** Acrescentar uma frase qualificando a simulação (modelo de propagação/perda assumido, ausência de colisões/ruído reais) e indicando que o cruzamento é uma estimativa de tendência, não um ponto operacional medido. Conectar à limitação de que o multi-hop real não foi medido em hardware.

### [ ] C5-059 · ⚪ BAIXO · `referencia`
- **Local:** Seção 5.6 (Conformidade RFC), l.239-241: 'conforme a Seção 6.11 da RFC'
- **Problema:** Referência à seção da RFC 3561 digitada no corpo do texto sem citação formal da fonte (\citeonline/\cite) na sentença, diferente do padrão de citação do restante do TCC. Verificar também se a numeração (6.11) corresponde de fato ao processamento de RERR na RFC 3561.
- **Correção:** Citar a RFC formalmente (ex.: 'conforme a Seção 6.11 da RFC 3561 \citeonline{rfc3561}') e conferir a numeração da seção citada contra o documento original.

### [ ] C5-060 · ⚪ BAIXO · `coesao`
- **Local:** Seção 5.4 (Energia/canal), l.190-199
- **Problema:** A seção mistura duas métricas distintas (consumo energético e ocupação de canal) sob um único parágrafo denso, com vários números intercalados (21%, 12,66 J, 10,45 J, 1.066 vs 564, 1,89, 1,98), dificultando a leitura. A Figura 9 (canal) é referenciada na seção 5.2, longe da discussão de canal aqui.
- **Correção:** Separar em duas ideias-tópico (energia; ocupação de canal) ou ao menos quebrar em dois parágrafos, e referenciar a Figura de canal no ponto da discussão onde os números RX/TX são interpretados, aproximando figura e texto.

### [ ] C5-061 · ⚪ BAIXO · `escrita`
- **Local:** Seção 5.1, l.19-21: 'versão reduzida do cenário C1 (denominada C1-3n)'
- **Problema:** O cenário 'C1' é citado como se o leitor já o conhecesse, mas a definição está no Capítulo 3 e não é referenciada aqui por \ref. O sufixo '-3n' (três nós) também não é explicado na primeira ocorrência.
- **Correção:** Referenciar o cenário C1 com \ref à tabela de cenários do Cap.3 e explicitar na primeira menção que 'C1-3n' designa a variante de três nós do cenário C1.

### [ ] C5-062 · ⚪ BAIXO · `escrita`
- **Local:** Seção 5.3 (Latência), l.172-173: 'Os valores são quantizados pelo laço de 100 ms da aplicação.'
- **Problema:** Informação metodológica relevante (resolução de medida) aparece apenas de passagem na discussão e repetida na limitação (iii); a primeira menção não explica a consequência (todos os valores de latência são múltiplos de ~100 ms? por que então 60,0 ms e 61,9 ms?), gerando aparente contradição com os valores reportados.
- **Correção:** Esclarecer a relação entre o laço de 100 ms e os valores reportados (por exemplo, como 60,0 ms ou 83,6 ms se conciliam com quantização de 100 ms) para não soar contraditório; ou ajustar a descrição da granularidade real da medida.

---

## 6. Capítulo 6 — Conclusão  ·  nota 79

_Boa conclusao; ver pontos fortes e problemas._

**Pontos fortes:**
- Registro PT-BR formal correto.
- Sintese final (32-35) condensa a tese reativo vs flooding.
- Limitacoes honestas e trabalhos futuros concretos (45-59).
- Honestidade metodologica sobre o vies do unicast (37-43).

**Problemas:**

### [ ] C6-063 · 🟠 ALTO · `completude`
- **Local:** objetivo e, cap_1 163-166
- **Problema:** O objetivo de comparar resultados com trabalhos correlatos da literatura nao e retomado; a conclusao so compara contra o baseline interno de flooding. O objetivo d tambem fica incompleto: a linha 19 declara cumprimento sem ressalvar o cenario unico e a NRL so aparece perifrasticamente (linha 27).
- **Correção:** No paragrafo de objetivos (17-20), posicionar a contribuicao frente aos correlatos do Cap. 3 e ressalvar o atendimento parcial do objetivo d.

### [ ] C6-064 · 🟡 MÉDIO · `argumentacao`
- **Local:** linhas 18-20 e 32-35
- **Problema:** Os objetivos especificos sao resolvidos por remissao a capitulos sem sintetizar o achado de cada um (tres num so periodo); e o fechamento nao retoma a pergunta de pesquisa da introducao, prejudicando a circularidade.
- **Correção:** Expandir cada objetivo com seu achado-chave antes da remissao e inserir no fechamento frase que responda a questao de pesquisa.

### [ ] C6-065 · ⚪ BAIXO · `escrita`
- **Local:** linhas 6,14,22-32,18-31
- **Problema:** Polimento: o trabalho como sujeito 3x na abertura (6,14,17); paragrafo de resultados (22-32) unico de ~14 linhas; remissoes fixas e sigla C1-3n (22) sem reapresentacao.
- **Correção:** Voz impessoal no 2o paragrafo; quebrar resultados em dois; referencia cruzada e aposto em C1-3n.

---

## 7. Resumo e Abstract  ·  nota 85

_O resumo e o abstract sao de alta qualidade de escrita: bem estruturados, com registro academico formal, e cobrem todos os elementos canonicos exigidos pela ABNT NBR 6028 (contexto, objetivo/problema, metodo, resultados e conclusao) em um unico paragrafo continuo. O tamanho (243 e 211 palavras de corpo) esta confortavelmente dentro da faixa de 150-500 palavras. O abstract e uma traducao fiel e idiomatica do resumo, sem omissoes de conteudo. O tempo verbal predominante (presente para descrever o trabalho, preterito para os experimentos) e adequado. Os principais pontos a melhorar sao: (1) ausencia do verbo conjugado na 3a pessoa do singular ou na voz passiva recomendada pela NBR 6028 — o resumo abre na voz ativa com sujeito implicito ("Este trabalho propoe"), o que e aceitavel mas a norma prefere construcao impessoal; (2) o abstract usa "We conclude", introduzindo 1a pessoa do plural que destoa do registro impessoal do resumo PT ("Conclui-se"), gerando leve infidelidade de tom; (3) excesso de subordinacao e densidade de siglas/numeros em poucas frases, que poderiam ser quebradas para melhor legibilidade. Nenhum problema critico de credibilidade ou completude foi encontrado._

**Pontos fortes:**
- Cobre integralmente os cinco elementos da ABNT NBR 6028: contexto (limitacao do ESP-NOW em multi-hop), objetivo (propor/implementar/avaliar o AODV-EN), metodo (Design Science Research, implementacao em C, comparacao com flooding, metricas PDR/latencia/NRL/energia), resultados (equivalencia em PDR, vantagem em latencia e energia, cruzamento de eficiencia) e conclusao (roteamento reativo vantajoso com a escala)
- Tamanho dentro da faixa ABNT (corpo de ~243 palavras no resumo e ~211 no abstract; paragrafo unico, conforme a norma)
- Abstract e traducao fiel e idiomatica, sem omissao de conteudo nem 'portugues traduzido' artificial; terminologia tecnica correta em ingles (controlled flooding, link-layer broadcast, sequence numbers, end-to-end acknowledgment)
- Palavras-chave presentes em ambos, consistentes entre si (seis termos, mesma ordem e correspondencia PT/EN), separadas por ponto e virgula conforme a ABNT
- Uso correto e consistente de \emph{} para estrangeirismos no resumo PT (flooding, broadcast, Design Science Research, Packet Delivery Ratio etc.), com expansao das siglas na primeira ocorrencia

**Problemas:**

### [ ] RA-066 · 🟡 MÉDIO · `coerencia`
- **Local:** abstract.tex, linha 23: 'We conclude that AODV-EN's reactive routing...'
- **Problema:** O abstract introduz a 1a pessoa do plural ('We conclude'), enquanto o resumo PT usa a construcao impessoal 'Conclui-se que' (linha 25). Alem de quebrar a fidelidade de tom entre as duas versoes, o uso de 1a pessoa contraria o registro impessoal recomendado pela ABNT NBR 6028, que pede o verbo na voz ativa de 3a pessoa ou voz passiva sintetica.
- **Correção:** Substituir por construcao impessoal equivalente, p. ex. 'It is concluded that AODV-EN's reactive routing becomes advantageous...'. Garantir simetria de pessoa/voz com o resumo PT ao longo de todo o texto.

### [ ] RA-067 · 🟡 MÉDIO · `abnt-formatacao`
- **Local:** resumo.tex, linha 8-9: 'Este trabalho propoe, implementa e avalia o AODV-EN'
- **Problema:** A ABNT NBR 6028 recomenda que o resumo seja redigido com o verbo na voz ativa em 3a pessoa ou na voz passiva, evitando sujeito-agente como 'Este trabalho'. A abertura com 'Este trabalho propoe' e amplamente tolerada, mas para rigor normativo a frase de objetivo poderia ser impessoalizada.
- **Correção:** Opcionalmente reformular para voz passiva sintetica, p. ex. 'Propoe-se, implementa-se e avalia-se o AODV-EN, uma adaptacao...'. Caso se mantenha 'Este trabalho', verificar com o orientador a preferencia da banca/IFG.

### [ ] RA-068 · ⚪ BAIXO · `escrita`
- **Local:** resumo.tex, linhas 18-22: frase iniciada em 'No cenario avaliado em hardware (tres ESP32, seis repeticoes)...'
- **Problema:** Periodo muito longo e denso, com multiplos resultados numericos, parenteses e oracoes encaixadas em uma unica frase, dificultando a leitura. Concentra PDR, latencia, energia e NRL sem pausa sintatica adequada.
- **Correção:** Dividir em duas frases: uma para confiabilidade (PDR equivalente) e outra para as vantagens do AODV-EN (latencia, energia) com o custo (NRL). Ex.: '...os dois algoritmos apresentaram confiabilidade equivalente (PDR de 98,93% ... e 98,77% ...). O AODV-EN obteve vantagem em latencia ... e em consumo energetico estimado ..., ao custo de uma carga de controle normalizada de 0,77.'

### [ ] RA-069 · ⚪ BAIXO · `escrita`
- **Local:** resumo.tex, linhas 21-22: 'latencia (60,0 ms estaveis contra 62 a 100 ms)'
- **Problema:** A expressao '60,0 ms estaveis contra 62 a 100 ms' e eliptica e um pouco coloquial; nao deixa explicito que se compara latencia do AODV-EN com a do flooding, e o adjetivo 'estaveis' (variabilidade) mistura-se ao valor (tendencia central) sem clareza.
- **Correção:** Tornar a comparacao explicita e separar tendencia de variabilidade, p. ex. 'menor e mais previsivel latencia (60,0 ms, ante 62 a 100 ms do flooding)'. Espelhar o ajuste no abstract ('stable 60.0 ms versus 62 to 100 ms', linha 20).

### [ ] RA-070 · ⚪ BAIXO · `coesao`
- **Local:** resumo.tex, linha 18 e abstract.tex linha 17: 'No cenario avaliado em hardware' / 'In the hardware scenario'
- **Problema:** Transicao abrupta entre a descricao do metodo e a apresentacao dos resultados; falta um conectivo que sinalize a passagem para a secao de achados, prejudicando a fluidez do paragrafo unico.
- **Correção:** Inserir conectivo de resultado, p. ex. 'Os experimentos em hardware (tres ESP32, seis repeticoes) mostraram que...' / 'Hardware experiments (three ESP32 nodes, six repetitions) showed that...'.

### [ ] RA-071 · ⚪ BAIXO · `tecnico`
- **Local:** resumo.tex, linha 22 e abstract.tex linha 21: 'carga de controle normalizada de 0,77' / 'normalized routing load of 0.77'
- **Problema:** A sigla NRL e definida antes (linha 18) como Normalized Routing Load, mas na apresentacao do resultado usa-se a forma por extenso ('carga de controle normalizada' / 'normalized routing load') em vez da sigla ja introduzida, criando leve inconsistencia terminologica. Alem disso, o valor 0,77 e dado sem unidade/interpretacao, o que pode ser opaco ao leitor do resumo.
- **Correção:** Usar a sigla ja definida: 'ao custo de um NRL de 0,77' / 'at the cost of an NRL of 0.77'. Considerar uma breve qualificacao interpretativa (ex.: 'NRL de 0,77, indicando sobrecarga de controle moderada') se couber no limite de palavras.

### [ ] RA-072 · ⚪ BAIXO · `escrita`
- **Local:** abstract.tex, linha 19: 'with AODV-EN providing lower and more predictable latency'
- **Problema:** Pequena divergencia de enfase entre versoes: o resumo PT diz '60,0 ms estaveis' (foco em estabilidade) e o abstract diz 'lower and more predictable latency' (acrescenta 'lower'/'more predictable'). A traducao esta boa, mas a assimetria de adjetivos entre PT e EN deveria ser harmonizada para fidelidade plena.
- **Correção:** Alinhar os dois textos: ou incluir 'menor e mais previsivel' no resumo PT, ou ajustar o abstract para refletir exatamente a formulacao do PT. Recomenda-se a forma mais informativa ('menor e mais previsivel' / 'lower and more predictable') em ambos.

---

## 8. Transversal — Alinhamento Objetivos × Resultados × Conclusão  ·  nota 58

_A cadeia objetivo → resultado → conclusão é parcialmente coerente, mas apresenta um desalinhamento crítico e vários médios. Os objetivos (c) implementar e (d) avaliar desempenho estão bem endereçados nos resultados e retomados na conclusão. Porém o objetivo (e) — comparar com trabalhos correlatos da literatura e posicionar a contribuição — NÃO é entregue no Cap. 5: a comparação se limita ao baseline interno de flooding, sem confrontar os trabalhos citados na introdução (becker2025, cujilema2023/BRAM-NOW, urazayev2023). A conclusão tampouco cumpre esse objetivo, mencionando o posicionamento apenas como desdobramento futuro. Os objetivos (a) e (b) são retomados na conclusão por referência, mas a conclusão os atribui a capítulos inconsistentes com o que o próprio Cap. 5 afirma (Cap. 4 vs. Cap. 5), e há referências cruzadas quebradas no Cap. 5 (Seção 4.4 e Seção 6.4 inexistentes/erradas). O problema da introdução (lacuna de roteamento padronizado sobre ESP-NOW) é respondido em viabilidade, mas a parte do problema relativa à comparabilidade com a literatura — justamente a fraqueza apontada no BRAM-NOW — fica sem fechamento. Correções pontuais elevam o trabalho a um patamar consistente._

**Pontos fortes:**
- Os objetivos (c) implementar protótipo e (d) avaliar desempenho com PDR, latência, NRL e energia estão integralmente endereçados no Cap. 5 (Quadro 15, Tabela 1, Figuras 8-11) e retomados explicitamente na conclusão — as quatro métricas prometidas no objetivo (d) aparecem todas, com a mesma nomenclatura.
- A conclusão (Cap. 6, l. 14-20) faz um esforço explícito de retomar o objetivo geral e enumerar os específicos, sinalizando ao leitor a intenção de fechar o ciclo objetivo→entrega.
- A seção 'Escopo Implementado e Limitações' (Cap. 5, l. 249-260) e o parágrafo correspondente da conclusão são honestos quanto ao que do objetivo (b) foi de fato exercitado nos resultados (núcleo reativo + hop count) versus o que ficou como trabalho futuro (LRU e métrica híbrida), o que preserva a credibilidade.
- A 'Análise de Escalabilidade por Simulação' (Cap. 5, l. 201-228) conecta o resultado à 'hipótese central do trabalho' e é fielmente retomada na conclusão (crossover de 9-11 nós, limite de TTL), dando coesão à narrativa do problema multi-hop.

**Problemas:**

### [ ] AL-073 · 🔴 CRÍTICO · `completude`
- **Local:** Cap. 5 inteiro (Resultados) vs. Cap. 1, objetivo específico (e), l. 163-166
- **Problema:** O objetivo (e) promete 'Comparar os resultados obtidos com trabalhos correlatos da literatura, posicionando a contribuição do AODV-EN em relação às soluções existentes para redes mesh sobre ESP-NOW e outras tecnologias'. O Cap. 5 NÃO entrega essa comparação: todas as comparações são contra o baseline interno de flooding. Não há nenhum confronto com os trabalhos da literatura citados na introdução (becker2025: latência 2,8 ms, PDR >99%; cujilema2023/BRAM-NOW: 75 ms, 9,25% de perda; urazayev2023). A busca por 'correlat|literatura|becker|cujilema|BRAM' no Cap. 5 não retorna nenhuma ocorrência substantiva. Objetivo prometido e não entregue — é exatamente a lacuna que a própria introdução (l. 100-103) usa para justificar o trabalho (o BRAM-NOW 'não se fundamenta em protocolos padronizados, o que dificulta sua comparação').
- **Correção:** Acrescentar ao Cap. 5 uma seção 'Comparação com Trabalhos Correlatos' (antes de Conformidade com a RFC) que confronte explicitamente as métricas do AODV-EN com os números reportados por becker2025, cujilema2023 e urazayev2023, discutindo diferenças de cenário/condições, e posicione a contribuição (roteamento padronizado AODV vs. BRAM-NOW ad-hoc). Caso a comparação numérica direta não seja metodologicamente justa, declarar isso explicitamente e fazer ao menos a comparação qualitativa de abordagem/escopo. Sem isso, o objetivo (e) deve ser removido/reformulado no Cap. 1 — mas removê-lo enfraquece a justificativa.

### [ ] AL-074 · 🟠 ALTO · `completude`
- **Local:** Cap. 6 (Conclusão), l. 54-59, e ausência de fechamento do objetivo (e)
- **Problema:** A conclusão não retoma o objetivo (e). O posicionamento frente à literatura aparece apenas como promessa futura ('Tais extensões consolidariam o AODV-EN como uma alternativa padronizada...', l. 58-59) e como nota metodológica sobre o flooding canônico (l. 42-43). Como o problema da introdução é a falta de solução padronizada E comparável com a literatura, a conclusão deixa metade do problema sem resposta: demonstra viabilidade (objetivo geral), mas não fecha a comparabilidade prometida.
- **Correção:** Adicionar à conclusão um parágrafo que responda diretamente à comparação com correlatos (espelhando a nova seção do Cap. 5), afirmando onde o AODV-EN se situa em relação a BRAM-NOW e às tecnologias avaliadas em becker2025/urazayev2023. Conectar de volta à frase 'dificulta sua comparação com soluções estabelecidas' da introdução, mostrando que o trabalho supera essa limitação.

### [ ] AL-075 · 🟠 ALTO · `coerencia`
- **Local:** Cap. 5, l. 258 ('Capítulo 5') vs. Cap. 6, l. 18-19 ('Capítulo 4')
- **Problema:** Contradição direta entre os dois capítulos sobre onde as adaptações (LRU, métrica híbrida) foram projetadas/descritas. O Cap. 5 (l. 257-259) diz: 'embora projetadas e descritas no Capítulo 5'. O Cap. 6 (l. 18-19) diz: 'projetadas e implementadas as adaptações necessárias (Capítulo 4)'. A estrutura real do documento (verificada nos includes) é: Cap. 4 = 'PROJETO E IMPLEMENTAÇÃO DO ALGORITMO'. Logo o Cap. 5 está errado (autorreferência: o Cap. 5 é o próprio capítulo de Resultados) e o Cap. 6 está certo. Um avaliador que cruzar as duas frases percebe a inconsistência, o que mina a credibilidade da retomada de objetivos.
- **Correção:** Corrigir o Cap. 5, l. 258: trocar 'descritas no Capítulo 5' por 'projetadas e descritas no Capítulo 4'. Conferir todas as autorreferências do Cap. 5.

### [ ] AL-076 · 🟠 ALTO · `referencia`
- **Local:** Cap. 5, l. 22 ('Seção 4.4') e l. 26 ('Seção 6.4')
- **Problema:** Duas referências cruzadas erradas/quebradas, ambas em pontos que sustentam o alinhamento com a metodologia. (1) L. 22: 'o planejamento experimental (Seção 4.4) preveja cenários de até cinco saltos e trinta repetições' — o Planejamento Experimental é uma seção do Cap. 3 (METODOLOGIA), não do Cap. 4 (Projeto/Implementação). (2) L. 26: 'os cenários de maior diâmetro e escala foram avaliados por simulação (Seção 6.4)' — a análise de escalabilidade por simulação está DENTRO do próprio Cap. 5 (seção 'Análise de Escalabilidade por Simulação', l. 201); o Cap. 6 é a Conclusão e não tem seção 6.4. As referências estão hardcoded como texto literal (não \ref), então não quebram na compilação, mas apontam para o lugar errado.
- **Correção:** Substituir os números literais por \ref{} com labels reais: l. 22 deve apontar para a seção de Planejamento Experimental do Cap. 3 (label cenuxe1rios-experimentais/planejamento-experimental); l. 26 deve apontar para a seção de escalabilidade do próprio Cap. 5 (label anuxe1lise-de-escalabilidade-por-simulauxe7uxe3o). Preferir \ref/\autoref a números fixos para evitar reincidência.

### [ ] AL-077 · 🟡 MÉDIO · `coerencia`
- **Local:** Cap. 1, objetivo (a), l. 144-148 vs. Cap. 6, l. 17-18 ('Capítulo 2')
- **Problema:** A conclusão atribui o cumprimento do objetivo (a) — 'analisar as limitações técnicas do ESP-NOW, identificando os desafios... limite de peers, ausência de broadcast nativo e gerenciamento de rotas' — ao Capítulo 2 (Referencial Teórico). Porém o Cap. 2 apresenta o limite de 20 peers como fato pontual em tabela (l. 225), não como uma 'análise das limitações com identificação de desafios'. A análise crítica dessas três limitações está, na verdade, concentrada na Introdução (Cap. 1, l. 75-103). Há descasamento entre onde o objetivo (a) é dito cumprido e onde de fato é desenvolvido; um objetivo de 'analisar/identificar' fica frágil se o conteúdo é majoritariamente descritivo no referencial.
- **Correção:** Garantir que exista no corpo (Cap. 2 ou seção própria) uma análise explícita das três limitações como desafios de projeto, e ajustar a referência da conclusão para o(s) capítulo(s) que realmente contêm essa análise. Alternativamente, reformular o objetivo (a) para 'caracterizar' se o tratamento for descritivo.

### [ ] AL-078 · 🟡 MÉDIO · `coerencia`
- **Local:** Cap. 6, l. 17-20 (retomada dos objetivos b, c, d)
- **Problema:** A retomada dos objetivos na conclusão é genérica e parcial: cita os objetivos (a), (b), (c)/(d) em bloco, mas não há um fechamento item-a-item que torne inequívoco qual objetivo foi atendido e em que medida. O objetivo (b) inclui explicitamente 'gerenciamento de peers com política LRU, flooding controlado e métrica híbrida'; a conclusão admite que LRU e métrica híbrida não foram avaliados (apenas projetados/implementados). Logo o objetivo (b) é cumprido só no eixo 'projetar', não no de validar — isso fica diluído e o leitor pode inferir cumprimento total.
- **Correção:** Reescrever a retomada como mapeamento explícito objetivo→evidência (ex.: lista ou parágrafo por objetivo), declarando para (b) que o projeto foi concluído mas a avaliação comparativa de LRU/métrica híbrida é trabalho futuro, distinguindo 'projetado/implementado' de 'avaliado'. Isso alinha a conclusão ao escopo realmente exercitado no Cap. 5.

### [ ] AL-079 · 🟡 MÉDIO · `coerencia`
- **Local:** Cap. 1 (Objetivos) vs. Cap. 5/6 — ausência de pergunta de pesquisa/hipótese explícita
- **Problema:** A introdução não enuncia uma pergunta de pesquisa nem uma hipótese formal (busca por 'problema|pergunta|hipótese' no Cap. 1 não retorna enunciado explícito). No entanto, o Cap. 5 (l. 223) e o Cap. 6 afirmam 'confirmar a hipótese central do trabalho'. Há referência a uma hipótese que nunca foi formalmente declarada na introdução, criando um elo solto na cadeia problema→hipótese→resultado→conclusão.
- **Correção:** Inserir na introdução (ou no início da metodologia) o enunciado explícito da pergunta de pesquisa e da hipótese central ('o roteamento reativo paga overhead de controle aproximadamente constante para evitar o custo de disseminação que cresce com a rede'), para que o 'confirmar a hipótese' dos Caps. 5 e 6 tenha antecedente textual.

### [ ] AL-080 · ⚪ BAIXO · `coesao`
- **Local:** Cap. 5, l. 119 (Tabela 1) e Cap. 6, l. 24-25
- **Problema:** Pequena inconsistência de valores entre capítulos no relato da latência do flooding: o Cap. 5 (Tabela 1, l. 120) reporta faixa derivada do ledger 61,9–100,0 ms; a conclusão (l. 25) escreve '62 a 100 ms'. O arredondamento de 61,9 para 62 é defensável, mas diverge do número exato apresentado no corpo, e em retomada de resultado convém manter o mesmo valor.
- **Correção:** Uniformizar o valor reportado entre Cap. 5 e Cap. 6 (usar 61,9 ms ou declarar o arredondamento de forma consistente). Conferir todos os números repetidos na conclusão contra a Tabela 1.

---

## 9. Transversal — Formatação ABNT / LaTeX (referências cruzadas, floats)  ·  nota 58

_O texto tem boa redação acadêmica e estrutura de capítulos coerente, mas a camada de formatação ABNT/LaTeX está seriamente comprometida por um problema sistêmico: TODAS as referências cruzadas a figuras, quadros, tabelas, seções e capítulos são números "chumbados" (hardcoded) no texto, em vez de \ref{}/\label{}. Isso já produz inconsistências reais e verificáveis — as figuras do Capítulo 5 estão todas deslocadas em uma unidade (texto diz "Figura 8–11", o contador do LaTeX gerará 7–10), várias referências de seção apontam para locais inexistentes ("Seção 6.4", "Capítulo 5" autorreferenciando o próprio capítulo) e diversos floats (Quadros 2, 4, 5, 9, 10, 11) nunca são citados no corpo do texto, violando a regra ABNT de que todo elemento flutuante seja chamado. Some-se a isto: dois \section indevidos no Capítulo 1 (objetivos deveriam ser \subsection), citações em texto corrido fora do padrão abntex2cite (GIL 2017, "Becker et al. (2025)", "Chai e Zeng (2021)" digitados manualmente), "Fonte:" de quadros em negrito em alguns casos e normal em outros, e títulos de seção em caixa alta inconsistente. Nenhum desses pontos afeta os números experimentais; são puramente de forma e referenciação, mas vários são de severidade alta/crítica porque um avaliador percebe imediatamente "Figura 8" sem que exista "Figura 7", e numeração quebrada de floats compromete a credibilidade. A correção é mecânica (trocar números fixos por \ref a labels colocados nos floats) mas precisa ser feita de forma abrangente._

**Pontos fortes:**
- Equações (Capítulo 4) usam corretamente \label/\ref (eq:custo, eq:penalidade, eq:ewma, etc.) — única família de referências cruzadas implementada da forma certa, servindo de modelo para o resto.
- Todas as 21 chaves do referencias.bib estão definidas e todas as chaves citadas via \cite/\citeonline existem no .bib (sem citações órfãs nem entradas não usadas, exceto gil2017/arregui2025 — ver problemas), e o uso de \citeonline vs \cite distingue corretamente citação no texto vs. parentética.
- Os quatro arquivos de figura .png referenciados em includegraphics (fig_hw_metrics, fig_hw_channel, fig_latency_seeds, fig_sim_crossover) existem em figuras/; nenhuma figura quebrada.
- Floats convertidos do pandoc (longtable/booktabs com \toprule/\midrule/\bottomrule) estão tecnicamente bem montados, com larguras de coluna calculadas e \endhead/\endlastfoot para quebra entre páginas.
- O ambiente customizado 'quadro' via \DeclareFloatingEnvironment e o \LTcaptype{quadro} estão corretamente configurados no preâmbulo, distinguindo Quadro (dados textuais) de Tabela (dados numéricos) conforme a convenção ABNT/IBGE.
- Figuras imagens (Capítulo 5) incluem atributo alt= para acessibilidade, o que é um cuidado acima do esperado.

**Problemas:**

### [x] AB-081 · 🔴 CRÍTICO · `abnt-formatacao`
- **Local:** capitulo_5.tex, linhas 128, 133 (e 138/143/173/178/206/218) — todas as figuras do Capítulo 5
- **Problema:** As figuras do Capítulo 5 são chamadas no texto como 'Figura 8', 'Figura 9', 'Figura 10' e 'Figura 11', mas o contador automático do LaTeX produzirá 7, 8, 9 e 10 (as figuras anteriores são 6: cap.3 tem 2, cap.4 tem 4). Não existe nenhuma 'Figura 7' no texto — o número 7 é pulado. Resultado: cada figura do cap.5 será citada com o número errado e o leitor verá 'Figura 8' apontando para uma figura rotulada 'Figura 7'.
- **Correção:** Adicionar \label{fig:hw_metrics} (etc.) dentro de cada ambiente figure, logo após \caption, e substituir as menções 'Figura 8/9/10/11' por 'Figura~\ref{fig:hw_metrics}' etc. Idem para TODAS as figuras dos cap.3 e cap.4. Nunca escrever o número à mão.

### [x] AB-082 · 🔴 CRÍTICO · `abnt-formatacao`
- **Local:** capitulo_2.tex L38/L129/L481; capitulo_3.tex L78/L156; capitulo_4.tex L275/L388/L632; capitulo_5.tex L57 — todas as menções 'Quadro N' e 'Tabela 1'
- **Problema:** Todos os quadros e a tabela são referenciados por número fixo no texto ('Quadro 1', 'Quadro 3', 'Quadro 6', ... 'Quadro 15', 'Tabela 1'). Nenhum \label/\ref é usado para floats não-equação. A numeração chumbada parte de uma contagem manual que não corresponde ao contador real: por exemplo, salta de 'Quadro 1' direto para 'Quadro 3' (pula o Quadro 2 = Especificações ESP32) e de 'Quadro 8' para 'Quadro 12'. Qualquer inserção/remoção/reordenação de float quebra silenciosamente todas as referências seguintes.
- **Correção:** Inserir \label{quad:...} em cada longtable de quadro/tabela (logo após \caption) e trocar as menções por \ref. Deixar o LaTeX numerar. Conferir após compilar que cada 'Quadro X' citado bate com o número impresso.

### [x] AB-083 · 🔴 CRÍTICO · `abnt-formatacao`
- **Local:** capitulo_1.tex, linhas 129 e 136
- **Problema:** 'OBJETIVO GERAL' e 'OBJETIVOS ESPECÍFICOS' estão marcados como \section, no mesmo nível hierárquico de 'OBJETIVOS' (L127, também \section). Logo, no sumário aparecerão como seções 1.1, 1.2 e 1.3 irmãs, quando deveriam ser subseções de OBJETIVOS (1.1.1 e 1.1.2). A hierarquia está achatada e incorreta.
- **Correção:** Trocar os \section{OBJETIVO GERAL} e \section{OBJETIVOS ESPECÍFICOS} por \subsection. Verificar também se 'INTRODUÇÃO' deveria ter uma seção inicial sem título (contextualização) antes de 1.1 OBJETIVOS, conforme o template.

### [ ] AB-084 · 🟠 ALTO · `abnt-formatacao`
- **Local:** capitulo_2.tex (Quadros 2, 4, 5 = Especificações ESP32, Características ESP-NOW, Comparativo protocolos); capitulo_3.tex (Quadros 9, 10, 11 = Configuração cenários, Parâmetros experimentos, Métricas de avaliação)
- **Problema:** Seis quadros nunca são citados pelo número no corpo do texto (não há 'Quadro 2', 'Quadro 4', 'Quadro 5', 'Quadro 9', 'Quadro 10', 'Quadro 11' em lugar nenhum). A ABNT exige que todo elemento flutuante seja referenciado/chamado no texto antes de aparecer. Atualmente esses floats 'flutuam' sem âncora textual.
- **Correção:** Adicionar no parágrafo imediatamente anterior a cada um desses quadros uma frase de chamada do tipo 'O Quadro~\ref{quad:espec_esp32} apresenta...'. Isso resolve simultaneamente a chamada obrigatória e a numeração automática.

### [ ] AB-085 · 🟠 ALTO · `abnt-formatacao`
- **Local:** capitulo_3.tex L361 ('Capítulo 5, Seção 5.4'); capitulo_5.tex L26 ('Seção 6.4') e L240 ('Seção 6.11' — esta é da RFC, ok) e L258 ('descritas no Capítulo 5')
- **Problema:** Referências cruzadas a seções/capítulos apontam para locais errados ou inexistentes. (a) cap.3 L361 manda o leitor à 'Seção 5.4' para a simulação, mas a análise por simulação está na Seção 5.4 do Capítulo 5 (Resultados) — ok em número mas o cap.5 L26 chama a mesma análise de 'Seção 6.4', que não existe (Capítulo 6 é a Conclusão, sem seções numeradas). (b) cap.5 L258 diz 'projetadas e descritas no Capítulo 5', mas o projeto/descrição das adaptações está no Capítulo 4 — o capítulo refere-se a si mesmo por engano. Há contradição interna sobre se a análise está no cap.5 ou cap.6.
- **Correção:** Padronizar via \label nos títulos de capítulo/seção e \ref/\autoref nas menções. Corrigir 'Seção 6.4' → \ref da seção de escalabilidade do cap.5; corrigir 'descritas no Capítulo 5' → 'Capítulo 4'. Revisar todas as menções 'Seção 4.4'/'Seção 3.4' (cap.3 L318 cita 'Seção 3.4' que é a própria — ok, mas use \ref).

### [x] AB-086 · 🟠 ALTO · `referencia`
- **Local:** capitulo_3.tex, linha 26
- **Problema:** A citação de Gil é feita em texto corrido manualmente como '(GIL, 2017)', fora do mecanismo abntex2cite. A chave gil2017 EXISTE no referencias.bib mas não é usada por \cite em lugar nenhum — ou seja, a referência não entrará na lista de Referências e o formato '(GIL, 2017)' não seguirá o estilo abntex2-alf do resto do documento.
- **Correção:** Substituir '(GIL, 2017)' por '\cite{gil2017}'. Conferir que a entrada apareça na lista final. Fazer varredura por outras citações digitadas à mão.

### [ ] AB-087 · 🟡 MÉDIO · `referencia`
- **Local:** capitulo_2.tex L75, L229, L367 (linhas 'Fonte:' dos quadros)
- **Problema:** Nas linhas de fonte dos quadros, autores são digitados manualmente em formato autor-data ('Priyadarshi et al.~(2025)', 'Chai e Zeng (2021)', 'Becker et al.~(2025)') misturados com \citeonline. Isso gera inconsistência tipográfica (sobrenome manual vs. formatado pelo abntex2cite) e risco de divergência com a entrada real do .bib. 'et al.' manual também deveria ser itálico ou seguir a norma do estilo.
- **Correção:** Usar \citeonline{priyadarshi2025}, \citeonline{chaizeng2021}, \citeonline{becker2025} também nas linhas de Fonte, deixando o estilo formatar o et al./e. Padronizar todas as fontes de quadro com o mesmo mecanismo.

### [ ] AB-088 · 🟡 MÉDIO · `abnt-formatacao`
- **Local:** capitulo_4.tex L308, L427, L667 vs capitulo_2.tex L75/L125/L166/L229 e capitulo_3.tex L206/L400/L449/L505
- **Problema:** Inconsistência de formatação da linha 'Fonte:' dos quadros. Em vários quadros do Capítulo 4 a fonte está em negrito ('\textbf{Fonte: Elaborado pelos autores.}'), enquanto nos Capítulos 2, 3 e 5 a mesma linha está em texto normal ('Fonte: ...'). A ABNT pede fonte em fonte menor (ex.: \footnotesize/\small) e padronizada, normalmente sem negrito.
- **Correção:** Padronizar todas as linhas de Fonte: remover \textbf, aplicar tamanho reduzido uniforme (ex.: definir um comando \fonte{...}) e mantê-las imediatamente abaixo do quadro/figura sem linha em branco extra.

### [ ] AB-089 · 🟡 MÉDIO · `abnt-formatacao`
- **Local:** capitulo_5.tex, linhas 110-124 (Tabela 1) e quadros longtable em geral
- **Problema:** A 'Tabela 1' usa \LTcaptype{table} num longtable, o que é correto, mas o restante dos dados numéricos de resultados (Quadro 15 'Ledger', L63-103) é declarado como \LTcaptype{quadro} embora contenha exclusivamente dados numéricos por seed. Pela convenção ABNT/IBGE, conteúdo numérico aberto nas laterais é Tabela, não Quadro. Há mistura de critério: o ledger numérico é Quadro, mas a consolidação numérica é Tabela.
- **Correção:** Reclassificar o 'Ledger de execuções' como Tabela (\LTcaptype{table}) por conter apenas medições numéricas, ou justificar explicitamente o critério adotado. Garantir coerência: dados numéricos → Tabela; dados textuais/esquemáticos → Quadro.

### [ ] AB-090 · 🟡 MÉDIO · `abnt-formatacao`
- **Local:** Template_Pacheco_TCC.tex L64; figuras em geral usam [H]
- **Problema:** O preâmbulo redefine \cleardoublepage como \clearpage e o ambiente quadro é declarado com placement=H; as figuras tikz/imagem usam \begin{figure}[H] (posicionamento forçado via float/H). O uso indiscriminado de [H] em todas as figuras pode gerar grandes espaços em branco e quebra ruim de página, e contraria a recomendação de deixar o LaTeX posicionar floats; além disso as figuras de imagem do cap.5 (L131, 141, 176, 216) usam \begin{figure} SEM especificador de posição, criando inconsistência com as demais que usam [H].
- **Correção:** Padronizar o especificador de posição das figuras (escolher [htbp] ou [H] de forma consistente) e revisar se o uso de [H] é realmente necessário caso a caso para evitar páginas com excesso de espaço.

### [ ] AB-091 · ⚪ BAIXO · `abnt-formatacao`
- **Local:** capitulo_1.tex L4 'INTRODUÇÃO', cap.2 L4 'REFERENCIAL TEÓRICO', cap.4 L4 'PROJETO E IMPLEMENTAÇÃO...' (todos caixa alta) vs. seções como cap.2 L16 'Internet das Coisas e Redes de Sensores Sem Fio' (caixa baixa) e subseções 'Tabela de Rotas' (Title Case) vs 'Características técnicas' (sentence case)
- **Problema:** Inconsistência de capitalização de títulos. Capítulos em CAIXA ALTA (ok para ABNT nível 1), mas as seções/subseções misturam Title Case ('Tabela de Rotas', 'Cache de RREQ', 'Métrica Híbrida com Hop Count e RSSI') com sentence case ('Características técnicas', 'Desempenho e alcance', 'Conceitos fundamentais de redes mesh'). O padrão deve ser único em todo o documento.
- **Correção:** Definir um padrão (recomendado: apenas primeira palavra maiúscula nos níveis 2+, conforme muitos manuais ABNT) e aplicar a todos os \section/\subsection. Ex.: 'Tabela de rotas', 'Cache de RREQ', 'Métrica híbrida com hop count e RSSI'.

### [ ] AB-092 · ⚪ BAIXO · `tecnico`
- **Local:** capitulo_4.tex L685 (\label{...-2}) e L731 (\label{metrica-hibrida-com-hop-count-e-rssi-2})
- **Problema:** Existem labels de subseção com sufixo '-2' (gerados pelo pandoc por colisão de slug, ex. flooding-controlado...-2 e metrica-hibrida...-2) porque há subseções com títulos quase idênticos no mesmo capítulo (Decisões de Projeto vs. Adaptações do AODV repetem 'Flooding controlado sobre broadcast' e 'Métrica híbrida'). Conteúdo redundante e labels poluídos.
- **Correção:** Renomear os labels para nomes semânticos (\label{sec:flooding-adaptacao}) e avaliar se as duas subseções quase homônimas (4.2.2/4.7.2 sobre flooding; 4.2.3/4.7.4 sobre métrica) não deveriam ser fundidas para evitar repetição de conteúdo.

### [ ] AB-093 · ⚪ BAIXO · `abnt-formatacao`
- **Local:** capitulo_3.tex L451-454 (nota de rodapé manual com \textsuperscript{a})
- **Problema:** A nota explicativa dos valores ajustados da RFC é implementada manualmente com \textsuperscript{a} no quadro e um parágrafo \footnotesize solto abaixo, em vez de \footnote real. Não há vínculo automático nem numeração de nota gerenciada pelo LaTeX; o 'a' é fixo.
- **Correção:** Usar \footnote{} ou, dentro de longtable, o mecanismo de nota de tabela apropriado (ex.: pacote threeparttablex), para que a marca e o texto da nota fiquem vinculados e formatados conforme a norma.

### [ ] AB-094 · ⚪ BAIXO · `referencia`
- **Local:** referencias.bib vs uso — chave arregui2025
- **Problema:** A chave arregui2025 é citada apenas 1 vez (cap.1 L41) e gil2017 não é citada por \cite (ver problema separado). Vale auditar se todas as 21 entradas do .bib são efetivamente usadas após corrigir o caso GIL, para não deixar entradas órfãs nem, ao contrário, citações sem entrada.
- **Correção:** Após corrigir '(GIL, 2017)' → \cite{gil2017}, rodar verificação (ex.: comparar chaves do .bib com as usadas) e remover do .bib qualquer entrada que não seja citada, garantindo lista de Referências enxuta e completa.

---

## 10. Transversal — Referências Bibliográficas (referencias.bib + citações)  ·  nota 62

_O conjunto de referências é coerente com o tema e a cobertura conceitual é boa (DSR, AODV/RFC 3561, ESP-NOW, WSN, métricas de roteamento, modelo energético). A integração texto-bibliografia é majoritariamente correta: das 21 entradas do .bib, 20 são citadas e todas as 20 chaves citadas existem no .bib (nenhuma citação quebrada). Porém há problemas concretos de formatação ABNT que comprometem a nota: (1) seis das dez entradas @article — justamente as fontes centrais sobre ESP-NOW/WSN/mesh de 2021-2025 (becker2025, urazayev2023, cujilema2023, chaizeng2021, priyadarshi2025, arregui2025) — estão sem volume, número e páginas, campos obrigatórios pela NBR 6023 para artigos de periódico; (2) uso de "and others" no campo author de várias entradas, gerando "et al." na lista de referências, o que a ABNT NÃO admite na seção Referências (só nas citações no texto — todos os autores devem ser listados); (3) entrada gil2017 (@book) está no .bib mas nunca é citada — como o estilo é author-date (abntex2-alf), ela simplesmente não aparece, ficando como entrada órfã/morta; (4) citação manual "Becker et al.~(2025)" digitada à mão numa linha de Fonte (cap.2, l.229) em vez de \citeonline, quebrando a consistência; (5) menção a "Mark Weiser em 1991" no cap.2 (l.23) sem anexar \cite{weiser1991}. Além disso, os capítulos 5 (Resultados) e 6 (Conclusão) não têm nenhuma citação — ausência de ancoragem na literatura ao discutir resultados é fraqueza acadêmica. O número de 21 referências é o piso aceitável para um TCC, mas é enxuto; a base sobre AODV/ad hoc routing está subdimensionada (praticamente só a RFC 3561 e ni1999)._

**Pontos fortes:**
- Integridade referencial intacta: nenhuma citação no texto aponta para chave inexistente, e 20 das 21 entradas são efetivamente citadas (verificado por grep cruzado de \cite/\citeonline/\citeauthor/\citeyear contra as chaves do .bib).
- Cobertura conceitual adequada e bem distribuída por eixo: metodologia DSR (hevner2004, simon1996, peffers2007, marchsmith1995), protocolo base (perkins2003/RFC 3561, ni1999), ESP-NOW (becker2025, urazayev2023, cujilema2023, sestak2022, espressif2024), métricas de roteamento (couto2003/ETX, draves2004/WCETT, rfc6551) e modelo energético (heinzelman2000/LEACH, karlwillig2005).
- Uso correto e idiomático da distinção ABNT entre citação no texto e entre parênteses: \citeonline{} para autor como sujeito da frase (ex.: cap.4 l.138 '\citeonline{couto2003} demonstraram') e \cite{} para citação entre parênteses ao fim da sentença.
- Mistura equilibrada de fontes seminais/clássicas (weiser1991, heinzelman2000, couto2003) e fontes recentes (várias de 2023-2025), demonstrando atualização e fundamentação histórica.
- Tipos de entrada majoritariamente corretos: @inproceedings para anais de conferência (ni1999, heinzelman2000, couto2003, draves2004), @mastersthesis para a dissertação (sestak2022) e @misc para RFCs e datasheet.

**Problemas:**

### [ ] BIB-095 · 🟠 ALTO · `referencia`
- **Local:** referencias.bib, entradas becker2025 (l.9-14), urazayev2023 (l.16-21), cujilema2023 (l.23-28), chaizeng2021 (l.30-35), priyadarshi2025 (l.37-42), arregui2025 (l.51-56)
- **Problema:** As seis entradas @article mais centrais ao tema (todas as fontes sobre ESP-NOW, WSN e mesh) estão sem os campos volume, number e pages. A NBR 6023 exige, para artigo de periódico, a localização completa (volume, número/fascículo e páginas inicial-final). Sem isso a referência fica irrastreável e a lista de Referências reprovaria numa banca rigorosa. Contraste: as entradas clássicas (weiser1991, hevner2004, peffers2007, marchsmith1995) têm volume/number/pages completos.
- **Correção:** Completar volume, number e pages de cada artigo a partir do registro do periódico (ex.: Sensors, IEEE Access, IEEE Latin America Transactions, IEEE Communications Surveys & Tutorials, Internet of Things). Adicionar também DOI quando disponível, prática recomendada pela ABNT para documentos eletrônicos.

### [ ] BIB-096 · 🟠 ALTO · `abnt-formatacao`
- **Local:** referencias.bib: becker2025 (l.10), urazayev2023 (l.17), cujilema2023 (l.24), priyadarshi2025 (l.38), arregui2025 (l.52), rfc6551 (l.165)
- **Problema:** Uso de 'and others' no campo author. No estilo author-date abntex2-alf isso renderiza 'et al.' na própria lista de Referências. A NBR 6023 só admite 'et al.' nas citações no texto (mais de três autores); na seção Referências todos os autores devem ser nomeados (ou, na convenção tolerada, até três). Listar 'Becker, M. et al.' como entrada bibliográfica é não conformidade ABNT.
- **Correção:** Substituir 'and others' pela lista completa de autores de cada artigo. Se a banca/orientador adotar a convenção de truncar, manter pelo menos os três primeiros antes de 'et al.', mas o ideal ABNT é listar todos.

### [ ] BIB-097 · 🟡 MÉDIO · `referencia`
- **Local:** referencias.bib: gil2017 (l.113-120)
- **Problema:** A entrada gil2017 (Gil, Como Elaborar Projetos de Pesquisa) está definida no .bib mas nunca é citada em nenhum capítulo (confirmado por grep: zero ocorrências de \cite{gil2017} fora do .bib). Como o estilo é author-date, a entrada não aparece na lista compilada (o .bbl tem 20 itens, não 21) — vira referência morta. Pior: sendo a fonte metodológica clássica em PT-BR sobre tipologia de pesquisa, sua ausência de citação é uma lacuna, pois o cap.3 caracteriza a pesquisa (natureza, abordagem) sem ancorá-la em Gil.
- **Correção:** Citar gil2017 no capítulo 3 ao classificar a pesquisa quanto à natureza/abordagem/objetivos/procedimentos (uso canônico de Gil), ou remover a entrada do .bib se não for usada. Não deixar entrada órfã.

### [ ] BIB-098 · 🟡 MÉDIO · `abnt-formatacao`
- **Local:** documentos/capitulo_2.tex, l.229
- **Problema:** A linha 'Fonte: Becker et al.~(2025); \citeonline{espressif2024}.' mistura citação digitada manualmente ('Becker et al.~(2025)') com comando \citeonline. Isso quebra a consistência (em todo o resto do texto becker2025 é citado via comando), não fica sincronizado com o .bib e pode divergir da forma gerada pelo estilo (ex.: caixa do nome, ponto-e-vírgula, ano).
- **Correção:** Trocar por '\citeonline{becker2025}; \citeonline{espressif2024}.' (ou \cite com múltiplas chaves) para que a formatação seja gerada pelo estilo ABNT de forma consistente.

### [ ] BIB-099 · 🟡 MÉDIO · `referencia`
- **Local:** documentos/capitulo_2.tex, l.23-24
- **Problema:** Menção 'Essa visão, antecipada por Mark Weiser em 1991 sob o conceito de Computação Ubíqua' atribui afirmação a autor e ano específicos sem anexar \cite{weiser1991}. A chave weiser1991 só é citada uma vez (cap.1, l.10); esta segunda menção nominal-datada no cap.2 deveria igualmente ancorar a citação formal.
- **Correção:** Acrescentar \cite{weiser1991} (ou reescrever com \citeonline{weiser1991} como sujeito) ao fim do trecho da l.24, garantindo que toda atribuição autoral nomeada tenha citação formal correspondente.

### [ ] BIB-100 · 🟡 MÉDIO · `completude`
- **Local:** documentos/capitulo_5.tex e capitulo_6.tex (Resultados e Conclusão) — zero citações
- **Problema:** Os capítulos 5 (Resultados/Análise) e 6 (Conclusão) não contêm nenhuma citação. Ao discutir e interpretar resultados experimentais (PDR, latência, energia) é esperado confrontá-los com a literatura — por exemplo retomar becker2025/urazayev2023 (valores de latência/alcance ESP-NOW), heinzelman2000 (modelo energético) e ni1999 (broadcast storm, para justificar ganho do AODV sobre flooding). A ausência total de ancoragem enfraquece a discussão e a validade externa.
- **Correção:** Inserir, no cap.5, comparações dos resultados com os valores/tendências reportados nas fontes (especialmente becker2025, urazayev2023, heinzelman2000, ni1999) e, no cap.6, retomar as referências que sustentam as contribuições. Não é necessário citar números fixos, mas sim posicionar os achados frente à literatura.

### [ ] BIB-101 · ⚪ BAIXO · `referencia`
- **Local:** referencias.bib (base como um todo); subconjunto AODV/ad hoc routing
- **Problema:** Com 21 entradas, a base está no piso do aceitável para um TCC, mas o eixo do protocolo estudado (AODV / roteamento ad hoc reativo) é sustentado quase só pela RFC 3561 (perkins2003) e por ni1999. Faltam fontes secundárias revisadas por pares sobre AODV (surveys de protocolos reativos MANET, comparativos AODV/DSR/DSDV, trabalhos de avaliação de desempenho do AODV) que dariam profundidade ao referencial do protocolo base e ao estado da arte de comparações com flooding.
- **Correção:** Acrescentar 3-5 referências revisadas por pares sobre AODV/roteamento reativo MANET e sobre comparações de overhead reativo vs. flooding, fortalecendo o capítulo de fundamentação e a seção de trabalhos relacionados.

### [ ] BIB-102 · ⚪ BAIXO · `abnt-formatacao`
- **Local:** referencias.bib: espressif2024 (l.122-128), perkins2003 (l.1-7), rfc6551 (l.164-170)
- **Problema:** Entradas @misc (RFCs e datasheet) colocam a URL e a data de acesso dentro do campo note como texto livre ('Disponível em: ...'). Não há campo de data de acesso explícito ('Acesso em: DD mês AAAA'), exigido pela NBR 6023 para documentos online, e o datasheet espressif2024 usa howpublished='Documentação técnica' (genérico) sem versão do documento.
- **Correção:** Padronizar os @misc com 'Disponível em: <URL>. Acesso em: <data>.' no note, incluir a data de acesso e, no datasheet, a versão/revisão do documento. Considerar usar campos url/urldate se o fluxo de compilação suportar.

---

## 11. Transversal — Redação Acadêmica PT-BR (registro, voz, tempo verbal)  ·  nota 72

_A redação tem boa base técnica e registro formal predominante, mas há padrões recorrentes que comprometem a qualidade acadêmica esperada de um TCC. Os três problemas estruturais mais sérios e sistêmicos são: (1) referências cruzadas com numeração manual hardcoded ("Figura 3", "Quadro 12", "Capítulo 2/4/5") em vez de \ref/\autoref — frágil e propenso a ficar incorreto, problema da ABNT/LaTeX; (2) inconsistência de tempo verbal entre capítulos — o cap. 4 descreve o artefato ora no passado ("foi concebida", "foi organizada"), ora no presente ("utiliza", "separa"), e o cap. 1 usa presente/futuro ("propõe", "incluem") enquanto o cap. 6 usa passado ("propôs", "implementou"); como o trabalho já foi concluído, falta uma política verbal única e justificada; (3) "Fonte: Elaborado pelos autores" e parágrafos isolados em \cite revelam autoria em plural, ao passo que o registro impessoal domina o resto — há ambiguidade sobre a voz adotada. Além disso, há frases muito longas (períodos de 4-6 linhas com encadeamento de subordinadas), repetição lexical pesada ("permite que" / "essa abordagem" / "Dessa forma" como muletas de início de frase), e anglicismos com tratamento itálico inconsistente. Nenhum problema é crítico isoladamente, mas o conjunto rebaixa o polimento. Recomenda-se uma passada de uniformização de tempo verbal, conversão de todas as referências cruzadas para \ref, e revisão de períodos longos._

**Pontos fortes:**
- Registro formal e técnico predominantemente correto, com terminologia consistente (AODV-EN, RREQ/RREP/RERR, flooding, peers) e definição de siglas no primeiro uso (IoT, WSN, WMN, PDR, NRL, LRU, RSSI, EWMA).
- Encadeamento argumentativo do Capítulo 1 é sólido: parte da computação ubíqua, afunila para IoT/ESP32, identifica a lacuna (limitações do ESP-NOW) e justifica a proposta de forma lógica e bem fundamentada em literatura.
- O Capítulo 6 (Conclusão) é objetivo e honesto: retoma objetivos geral e específicos, declara limitações de forma transparente e separa claramente o que foi avaliado do que ficou como trabalho futuro — postura academicamente madura.
- Boa coesão referencial intra-parágrafo na descrição dos mecanismos (cap. 4), com transições claras entre descoberta, encaminhamento, manutenção e tratamento de falhas.

**Problemas:**

### [x] PT-103 · 🟠 ALTO · `abnt-formatacao`
- **Local:** capitulo_4.tex, linhas 24, 225, 275, 388, 484, 578, 632 ("A Figura 3", "A Figura 4", "Quadro 12", "Quadro 13", "Figura 5", "Figura 6", "Quadro 14") e capitulo_6.tex linhas 18-20, 50 ("Capítulo 2", "Capítulo 4", "Capítulo 5")
- **Problema:** Numeração de figuras, quadros e capítulos está escrita manualmente (hardcoded) no texto, em vez de usar referências automáticas \ref/\autoref. As figuras do cap. 4 sequer têm \label, e os números fixos (3, 4, 12, 13, 14) quase certamente ficarão incorretos quando o documento for recompilado ou reordenado — risco real de o texto citar "Figura 3" enquanto a numeração automática gera outro número. É um defeito de formatação ABNT/LaTeX que compromete a integridade das remissões.
- **Correção:** Adicionar \label{fig:arquitetura}, \label{fig:modular} etc. a cada \caption e substituir todas as menções por \autoref{...} ou "Figura~\ref{...}". O mesmo para os \chapter dos capítulos 2, 4 e 5 referenciados na conclusão.

### [ ] PT-104 · 🟠 ALTO · `coerencia`
- **Local:** Inconsistência entre capitulo_1.tex (linhas 103-125: "propõe", "incluem"), capitulo_4.tex (tempo misto, ver abaixo) e capitulo_6.tex (linha 6: "propôs, implementou e avaliou")
- **Problema:** Não há política de tempo verbal unificada para descrever o trabalho. O Capítulo 1 apresenta a proposta no presente ("o presente trabalho propõe", "As principais adaptações propostas... incluem"), o que é aceitável numa introdução, mas o Capítulo 6 narra tudo no passado ("propôs, implementou e avaliou"). Como a pesquisa já está concluída, a introdução escrita majoritariamente no presente/futuro de intenção pode soar como projeto em andamento e não como trabalho realizado, gerando dissonância com a conclusão.
- **Correção:** Definir e aplicar uma convenção única: introdução pode manter presente para a proposta, mas garantir que verbos de execução já realizada (implementação, experimentos) estejam no passado de forma consistente. Revisar o cap. 1 para que descrições do que foi feito não fiquem em futuro/intenção quando já concluídas.

### [ ] PT-105 · 🟠 ALTO · `coerencia`
- **Local:** capitulo_4.tex: presente vs. passado alternados — ex.: linha 18 "foi concebida", linha 212 "foi organizada", linha 270 "foram projetadas" (passado) versus linha 86 "suporta", linha 87 "utiliza", linha 364 "separa", linha 376 "utiliza" (presente)
- **Problema:** Dentro do mesmo capítulo, o artefato é descrito ora no pretérito (relato do que foi feito: "A arquitetura foi concebida", "A implementação foi organizada", "As estruturas foram projetadas") ora no presente atemporal (descrição de funcionamento: "O ESP-NOW suporta", "o AODV-EN utiliza", "o protocolo separa"). A alternância não segue critério explícito e por vezes ocorre em parágrafos adjacentes, prejudicando a uniformidade do registro.
- **Correção:** Adotar critério claro: pretérito para decisões/ações de projeto já tomadas ("foi adotada a política LRU") e presente para o comportamento permanente do protocolo ("o protocolo retransmite cada RREQ uma única vez"). Padronizar cada subseção segundo esse critério em vez de misturar.

### [ ] PT-106 · 🟡 MÉDIO · `escrita`
- **Local:** capitulo_1.tex, linhas 21-26 e 60-64
- **Problema:** Frases nominais sem verbo principal (fragmentos) usadas como períodos completos. Linha 25-26: "Características que o tornam ideal para prototipagem e implantação de soluções IoT em escala." é um fragmento — deveria ligar-se à frase anterior. Linha 60-64: "Desenvolvido pela Espressif Systems, operando de forma connectionless sobre a camada de enlace do padrão IEEE 802.11, permitindo a troca de mensagens curtas (...)" é uma sequência de orações reduzidas sem oração principal — não há verbo finito que sustente o período.
- **Correção:** Unir o fragmento à oração anterior com vírgula/relativo: "...custo acessível, características que o tornam ideal...". No segundo caso, fornecer verbo principal: "O ESP-NOW, desenvolvido pela Espressif Systems, opera de forma \emph{connectionless} sobre a camada de enlace do IEEE 802.11 e permite a troca de mensagens curtas...".

### [ ] PT-107 · 🟡 MÉDIO · `escrita`
- **Local:** Recorrente nos três capítulos — ex.: capitulo_1.tex linha 11 "Essa visão", linha 41 "Essa abordagem"; capitulo_4.tex linha 93 "Essa abordagem", linha 181 "Essa abordagem", linha 335 "Essa estrutura", linha 372 "Essa estratégia", linha 535 "Esse mecanismo"; conectivo "Dessa forma" em capitulo_4.tex linhas 217(impl.), 352, 628, 782
- **Problema:** Uso repetitivo e quase formulaico de demonstrativos anafóricos para iniciar frases ("Essa abordagem...", "Essa estrutura...", "Esse mecanismo...") e de conectivos de fechamento ("Dessa forma", "Assim", "Portanto"). O padrão se repete dezenas de vezes e cria monotonia sintática, além de tornar a anáfora por vezes vaga (a que "abordagem" exatamente se refere quando há duas no parágrafo anterior).
- **Correção:** Variar as aberturas de frase (nominalizar o referente explicitamente: "A separação entre vizinhança lógica e tabela física..." em vez de "Essa abordagem...") e reduzir conectivos de fechamento redundantes, eliminando os que apenas reafirmam o já dito.

### [ ] PT-108 · 🟡 MÉDIO · `escrita`
- **Local:** capitulo_4.tex, linhas 701-710 (parágrafo do unicast sequencial) e capitulo_1.tex linhas 26-47
- **Problema:** Frases excessivamente longas com múltiplas subordinadas encadeadas. Ex. cap.4 l.701-710: período único de ~7 linhas com três orações coordenadas listando consequências ("inflaria... subcontaria... descaracterizaria") aninhadas em explicações entre travessões e parênteses. O cap.1 também concentra períodos de 5-6 linhas. A densidade dificulta a leitura e a localização do núcleo da oração.
- **Correção:** Quebrar períodos com mais de 3-4 linhas em duas ou três frases. No exemplo do unicast, separar a decisão ("A alternativa foi descartada.") das três justificativas em frases curtas ou em lista, em vez de um único período sobrecarregado.

### [ ] PT-109 · 🟡 MÉDIO · `escrita`
- **Local:** capitulo_1.tex, linhas 107-108: "Dessa forma, o ESP-NOW possui restrições específicas de hardware e comunicação."
- **Problema:** Conectivo "Dessa forma" mal empregado: a frase anterior introduz a proposta do AODV-EN como adaptação do AODV, e a seguinte afirma que o ESP-NOW tem restrições — não há relação de consequência/conclusão entre as duas, então o conectivo é semanticamente incoerente. A frase também repete informação já estabelecida várias vezes no capítulo (as limitações do ESP-NOW já foram detalhadas nas linhas 75-96), soando redundante e deslocada.
- **Correção:** Remover a frase ou reposicioná-la, e eliminar o conectivo "Dessa forma". Se a intenção é fechar o parágrafo justificando a escolha do AODV, ligar diretamente: "A escolha do AODV como protocolo base justifica-se..." sem a sentença intermediária redundante.

### [ ] PT-110 · 🟡 MÉDIO · `abnt-formatacao`
- **Local:** Anglicismos com itálico inconsistente — ex.: capitulo_4.tex linha 86 "peers" sem itálico ("O ESP-NOW suporta até 20 \emph{peers}" está com itálico em 86, mas "peers" aparece sem \emph em linhas 358, 361, 716, 718, 720, 723, 725, 727; "hop count" com itálico na l.661 mas sem na l.741/747; "broadcast" sem itálico em todo o cap.4 §4.5.2 enquanto está com \emph no cap.1 e §4.2.2; "flooding" sem itálico nas linhas 349, 693, 700 mas com em outros pontos
- **Problema:** O mesmo estrangeirismo aparece ora em itálico (\emph), ora em redondo, ao longo dos capítulos e às vezes na mesma seção. A norma exige tratamento tipográfico uniforme para termos em língua estrangeira. A inconsistência é visível e recorrente (peers, broadcast, flooding, hop count).
- **Correção:** Definir uma lista de termos estrangeiros e aplicar \emph{} (ou a macro padronizada do template) a TODAS as ocorrências, ou — após a primeira definição — decidir aportuguesar/manter em redondo de forma consistente. Fazer uma varredura por termo.

### [ ] PT-111 · 🟡 MÉDIO · `coerencia`
- **Local:** capitulo_4.tex linha 308, 427, 667: "\textbf{Fonte: Elaborado pelos autores.}" e linha 25 "a arquitetura proposta" (impessoal) vs. autoria plural
- **Problema:** A fonte dos quadros declara autoria em primeira pessoa do plural ("Elaborado pelos autores"), enquanto todo o corpo do texto adota rigorosamente a impessoalidade ("foi concebida", "optou-se", "avaliou-se"). Há também tensão com a natureza do TCC (verificar se é individual ou em dupla — se individual, "pelos autores" no plural está incorreto). Essa coexistência de impessoal no texto e plural autoral nas fontes denota falta de decisão sobre a voz.
- **Correção:** Se o trabalho é individual, corrigir para "Elaborado pelo autor". Manter coerência: como o texto é impessoal, a fórmula de fonte é aceitável por convenção ABNT, mas o número (singular/plural) deve refletir a autoria real e ser uniforme em todo o documento.

### [ ] PT-112 · ⚪ BAIXO · `escrita`
- **Local:** capitulo_4.tex, linha 469 ("A descoberta de rotas é iniciada quando um nó de\norigem...") e linha 525 ("Após a descoberta de uma rota válida, o\nencaminhamento...")
- **Problema:** Quebras de linha no meio de sintagmas ("um nó de / origem", "o / encaminhamento") sugerem reflow automático de conversão (provavelmente de Markdown/Pandoc) que deixou rupturas visuais no fonte. Embora o LaTeX ignore a quebra na compilação, isso é sintoma de origem por conversão automática e pode ter introduzido outros artefatos.
- **Correção:** Reformatar o fonte para fluxo de parágrafo limpo; revisar todo o arquivo em busca de outros resíduos de conversão automática (espaçamentos duplos, quebras estranhas).

### [ ] PT-113 · ⚪ BAIXO · `escrita`
- **Local:** capitulo_1.tex linha 53, 105; capitulo_4.tex linha 133 — "ad-hoc" / "Ad-hoc"
- **Problema:** Grafia de "ad-hoc" inconsistente em maiúscula/minúscula e sem itálico (latinismo). Ora "redes ad-hoc", ora dentro de nome próprio "Ad-hoc On-Demand". Termos latinos como "ad hoc" convencionalmente vão em itálico no registro formal.
- **Correção:** Padronizar grafia ("ad hoc" sem hífen é a forma latina; com hífen quando adjetivo composto, conforme a convenção adotada) e aplicar itálico de forma consistente, exceto quando parte de nome próprio de protocolo.

### [ ] PT-114 · ⚪ BAIXO · `argumentacao`
- **Local:** capitulo_6.tex, linhas 22-35 (parágrafo de resultados na conclusão)
- **Problema:** A conclusão reapresenta valores numéricos detalhados (PDR 98,93%, 60,0 ms, 21%, 1,89 vez, 9 a 11 nós). Uma conclusão deve sintetizar achados, não repetir a tabela de resultados do capítulo 5. O excesso de números na conclusão é redundante com o capítulo de resultados e dilui a mensagem de síntese.
- **Correção:** Reduzir a densidade numérica na conclusão, mantendo apenas o achado qualitativo central (confiabilidade equivalente, vantagem em latência e energia, cruzamento de eficiência com a escala) e remetendo ao capítulo 5 para os valores exatos.

---

## A. Furos da auditoria adversarial — VERIFICADOS no código/arquivos

> A auditoria refutou a nota inicial (71%) → sugeriu 66%; adotada **66%**. 
> São furos de **conteúdo/honestidade**, não só formatação. Prioridade alta.

### [x] AUD-001
- FACTUAL — nao apenas \ref quebrada — no Cap.5 (l.258): o parecer trata 'projetadas e descritas no Capitulo 5' so como autorreferencia de numero errado. O furo real e maior: o Cap.5 afirma que LRU e metrica hibrida 'permanecem como TRABALHO FUTURO DE IMPLEMENTACAO e nao influenciam os resultados', enquanto o Cap.6 (l.18 e l.50) diz que foram 'projetadas e IMPLEMENTADAS no firmware (Capitulo 4)' e o Cap.4 (secoes 4.x, l.712-771) as descreve como implementadas. Verifiquei o firmware: aodv_en_peers.c (aodv_en_peer_find_lru_index, l.29/134) implementa o LRU e aodv_en_node.c (l.122-123) + aodv_en_limits.h (pesos 8/1) implementam a metrica hop+RSSI. Logo a frase do Cap.5 e FALSA e contradiz Cap.6+codigo. O correto e 'implementadas mas NAO avaliadas experimentalmente'. Isso e furo de credibilidade/honestidade metodologica (classe mais grave que \ref), e o parecer nao o capturou.

### [ ] AUD-002
- TITULO NAO AUDITADO e desalinhado do conteudo: title_page.tex/informacoes.tex/ata usam 'PROPOSTA DE ALGORITMO DE ROTEAMENTO EM REDES MESH DE BAIXO CUSTO COM ESP-NOW'. O titulo (i) diz 'PROPOSTA' enquanto resumo/intro/conclusao afirmam 'propoe, implementa e avalia', e (ii) NAO menciona AODV nem AODV-EN, que e o objeto central do trabalho. O parecer nao tem nenhum alvo para titulo e nao notou o descompasso titulo x escopo entregue.

### [x] AUD-003
- AGRADECIMENTOS sao TEXTO-MODELO nao preenchido: agradecimentos.tex (l.14) ainda contem o boilerplate do template ('Nesta secao, voce deve expressar seus agradecimentos...') e assina '\textit{Autor}'. Defeito de altissima visibilidade para banca, completamente fora do parecer (nenhum alvo para pre-textuais).

### [ ] AUD-004
- PRE-TEXTUAIS / FICHA / ATA / TERMOS nao auditados: ficha_catalografica.tex e ata_defesa.tex sao apenas \includegraphics de .jpg (ficha_catalografica.jpg, ata_defesa.jpg) que precisam existir/estar corretos; ha termo_autorizacao e declaracao_distribuicao no fluxo (Template l.81-84). Nenhum desses elementos obrigatorios foi avaliado pelo parecer.

### [ ] AUD-005
- COERENCIA DE TERMINOLOGIA flooding nao auditada como dimensao: o texto alterna 'Flooding' (maiusculo, Cap.3) vs 'flooding' (italico/minusculo, Cap.5/6), e usa o termo ora como algoritmo-baseline ora como tecnica de disseminacao do proprio AODV-EN ('flooding controlado de RREQ'). Essa polissemia pode confundir a banca; o parecer toca em 'flodadas/re-flodada' mas nao na inconsistencia de capitalizacao/uso do termo central.

### [ ] AUD-006
- DISCREPANCIA NUMERICA latencia entre capitulos nao flagrada: o resumo/abstract e a conclusao (Cap.6 l.25) dizem '62 a 100 ms' para o flooding, mas a Tabela/discussao do Cap.5 (l.167) reporta faixa '61,9 a 100,0 ms' e media 83,6. Arredondar 61,9->62 no resumo e aceitavel, mas e um ponto de consistencia numerica resumo-vs-corpo que o parecer (que elogiou o resumo com nota 85) nao verificou.

### [ ] AUD-007
- PLAGIO/AUTO-PLAGIO vs material .md anterior nao avaliado: existe TCC.md (300KB) e tcc_topic_4_4.txt / tcc_topic_4_7.txt na raiz; o parecer nao checou sobreposicao textual entre o LaTeX e esse material previo nem a originalidade — dimensao explicitamente pedida e ausente.

### [ ] AUD-008
- CALIBRACAO levemente generosa: o parecer rotula como puramente 'mecanicos/formatacao' defeitos que sao de CONTEUDO/correção (contradicao implementado-vs-trabalho-futuro entre Cap.5 e Cap.6; objetivo (e) prometido e nao entregue; titulo desalinhado; agradecimentos placeholder). Para um TCC de Eng. de Software, esses sao furos de substancia, nao so de \ref. 71% superestima ligeiramente o estado atual; faixa mais justa ~64-67%.

**Dimensões que faltaram revisar (verificar manualmente):**

- [ ] Titulo do trabalho (title_page.tex, informacoes.tex) e seu alinhamento com o escopo/abstract — nenhum alvo no parecer.
- [ ] Elementos pre-textuais obrigatorios: agradecimentos.tex (placeholder do template, nao preenchido), ficha_catalografica.tex e ata_defesa.tex (apenas includegraphics de JPG), termo_autorizacao.tex, declaracao_distribuicao.tex — nenhum avaliado.
- [ ] Contradicao factual implementado-vs-trabalho-futuro entre Cap.5 (l.258 'trabalho futuro de implementacao') e Cap.6 (l.18/50 'implementadas no firmware'), conferida contra o firmware (LRU em aodv_en_peers.c; metrica hop+RSSI em aodv_en_node.c/aodv_en_limits.h) — o parecer so tratou o numero de capitulo errado, nao a divergencia de conteudo.
- [ ] Consistencia de terminologia/capitalizacao de 'flooding/Flooding' e duplo-sentido do termo (baseline vs tecnica de disseminacao do RREQ) ao longo do documento.
- [ ] Consistencia numerica resumo/abstract/conclusao vs corpo (ex.: latencia '62 a 100' vs '61,9 a 100,0'; PDR/NRL/energia replicados) — o resumo recebeu 85 sem cross-check com o Cap.5.
- [ ] Originalidade / auto-plagio: comparacao do LaTeX com o material previo na raiz (TCC.md ~300KB, tcc_topic_4_4.txt, tcc_topic_4_7.txt) e com as fontes citadas.
- [ ] Existencia/correcao dos assets de imagem referenciados (figuras/fig_hw_metrics.png, fig_hw_channel.png, fig_latency_seeds.png, fig_sim_crossover.png; ata_defesa.jpg; ficha_catalografica.jpg; imagem_universidade.jpg) — nao verificado se os arquivos existem no repo.
- [ ] Coerencia da contagem real de floats apos compilacao: o parecer afirma o sintoma (numeros chumbados pulam), mas nao recompilou para confirmar a numeracao final efetiva de Quadros/Figuras/Tabelas.

---

## B. Ordem de ataque sugerida (top 10 por impacto)

1. [ ] Converter TODA a referenciação cruzada para \label/\ref (figuras, quadros, tabelas, seções, capítulos), em todos os capítulos. Adicionar \label logo após cada \caption e substituir os números digitados à mão. Isso resolve de uma vez: figuras do Cap.5 numeradas erradas (8-11→1-4), autorreferência do Cap.5 ao 'Capítulo 5', 'Seção 4.4'/'Seção 6.4' quebradas e a numeração de quadros que pula valores. Recompilar e conferir cada menção contra o número impresso.
2. [ ] Cumprir o objetivo específico (e): adicionar ao Cap.5 uma seção 'Comparação com trabalhos correlatos' confrontando AODV-EN com becker2025/cujilema2023/urazayev2023 (ao menos qualitativamente, ressalvando diferenças de cenário), e retomá-la na conclusão ligando à frase da introdução sobre o BRAM-NOW 'dificultar a comparação'.
3. [ ] Reconciliar planejado×executado: corrigir o Quadro 9 (30→indicar a campanha real de 6 repetições, ou marcar 30 como meta com nota explícita) e alinhar a promessa de IC95 da Seção 3.5 ao que o Cap.5 entrega (se não há IC, reformular para 'média e desvio padrão' justificando a ausência por n pequeno).
4. [ ] Padronizar o tempo verbal: converter as Seções de Métricas/Flooding/Coleta do Cap.3 do futuro para o passado/presente de descrição metodológica (o trabalho já foi executado), e uniformizar o Cap.4 (pretérito para decisões de projeto, presente para o comportamento permanente do protocolo).
5. [ ] Reestruturar o Cap.1: quebrar a introdução monolítica em 6-8 parágrafos temáticos; rebaixar OBJETIVO GERAL e OBJETIVOS ESPECÍFICOS para \subsection; enunciar explicitamente o problema/pergunta de pesquisa e a hipótese central; adicionar a seção 'Estrutura do Trabalho'.
6. [ ] Completar o referencial (Cap.2): adicionar subseção definindo flooding/broadcast storm (\cite{ni1999}) como baseline, e uma seção de métricas definindo PDR/NRL/latência/energia com as fontes já disponíveis, antes de usá-las.
7. [ ] Eliminar a duplicação do Cap.4: apresentar cada equação e cada decisão (flooding, métrica híbrida, LRU) uma única vez na Seção 4.2 e remeter por \ref na 4.7; remover eq:custo_impl e eq:penalidade_impl e os labels '-2'. Decidir se a Seção 4.2.4 (energia) vai para a metodologia.
8. [ ] Corrigir a bibliografia: completar volume/number/pages/DOI dos 6 @article centrais; substituir 'and others' pela lista completa de autores; citar gil2017 no Cap.3 (ou removê-la); trocar '(GIL, 2017)' e 'Becker et al. (2025)' por \cite/\citeonline; ancorar weiser1991 no Cap.2.
9. [ ] Chamar todos os floats órfãos no texto e padronizar as linhas 'Fonte:' (remover negrito, tamanho uniforme, \citeonline em vez de nomes digitados, decidir 'autor' vs 'autores' conforme a autoria real); reclassificar o 'ledger' numérico do Cap.5 de Quadro para Tabela.
10. [ ] Limpar a discussão do Cap.5: substituir 'flodadas/re-flodada' por formas formais; suavizar/embasar a 'equivalência estatística'; antecipar a ressalva das campanhas assíncronas ao introduzir a Tabela 1; qualificar a simulação (modelo de canal, ausência de colisões) e distinguir 'projetado' de 'avaliado' para os objetivos (b)/(d) na conclusão.

## C. O que falta para chegar a ~100%

- [ ] Camada de formatação ABNT/LaTeX 100% automática: nenhum número de figura/quadro/tabela/seção digitado à mão; todos os floats com \label, chamados no texto e com numeração verificada após compilação. Hoje este é o maior bloqueador isolado.
- [ ] Cadeia objetivo→resultado→conclusão completa e sem furos: cada objetivo específico mapeado a uma evidência concreta, incluindo a comparação com a literatura correlata (objetivo e) efetivamente realizada, e a distinção explícita entre o que foi projetado e o que foi avaliado (LRU e métrica híbrida).
- [ ] Coerência metodológica plena: planejamento (cenários, repetições, estatística) batendo exatamente com o que foi executado e reportado; descrição completa do método de simulação (ferramenta, modelo, calibração contra hardware) para tornar reprodutível a maior parte dos resultados.
- [ ] Referencial teórico autossuficiente: flooding, broadcast storm e todas as métricas de avaliação definidas no Cap.2 antes de serem usadas; base de AODV/roteamento reativo MANET ampliada com 3-5 fontes revisadas por pares além da RFC 3561 e ni1999.
- [ ] Polimento de redação uniforme: tempo verbal único e justificado em todo o documento; fim das frases-fragmento sem verbo principal, dos neologismos informais e da repetição lexical/anáforas vagas; itálico consistente em estrangeirismos; períodos longos quebrados.
- [ ] Bibliografia em conformidade total com a NBR 6023: artigos com localização completa (volume/número/páginas/DOI), todos os autores nomeados (sem 'et al.' na lista), @misc com data de acesso, e zero entradas órfãs ou citações fora do mecanismo abntex2cite.
- [ ] Resultados confrontados com a literatura e ancorados em citações nos Caps.5 e 6 (hoje sem nenhuma citação), com afirmações estatísticas sustentadas por teste declarado e tamanho amostral adequado à promessa metodológica.

---

## D. Itens que dependem da nova campanha experimental (mais nós + mais seeds)

Fecham só após coletar os novos dados:

- [ ] Atualizar Quadro/ledger e Tabela de comparação do Cap.5 com os novos números (N nós, M seeds)
- [ ] Entregar IC95 prometido no Cap.3 §3.5 (ou justificar ausência) — agora com n adequado
- [ ] Reconciliar '30 repetições' (Quadro 9) com o número real executado
- [ ] Regenerar figuras: fig_hw_metrics, fig_hw_channel, fig_latency_seeds, fig_sim_crossover
- [ ] Atualizar números replicados no Resumo, Abstract e Conclusão para baterem com o Cap.5
- [ ] Teste estatístico declarado para sustentar 'equivalência' de PDR (substituir 'menor que 1 desvio')
- [ ] Descrever o método de simulação (ferramenta, modelo de canal/energia, calibração vs hardware)

---

_Total acionável: 114 itens de capítulo/transversal + 8 furos de auditoria + 8 dimensões a verificar + Seções B/C/D._
