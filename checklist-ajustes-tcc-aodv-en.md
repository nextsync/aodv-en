# Checklist de Ajustes — TCC AODV-EN

## 🔴 Críticos (erros técnicos — corrigir antes de tudo)

- [ ] **1. "Topologia estrela (A\*)" — Introdução.** A* é algoritmo de busca, não topologia. Remover "(A*)".

- [ ] **2. Limite de peers invertido — Quadro 4 e seção 3.3.3.** O texto diz "20 com criptografia / 10 sem". A documentação Espressif indica o oposto: limite total de 20 peers, com subconjunto menor para peers *criptografados*. Corrigir e citar a fonte oficial.

- [ ] **3. Métrica híbrida inconsistente e incompleta — seções 3.6.3 e 5.6.4.** Duas fórmulas diferentes (`1/RSSI_médio` vs. `Penalidade(RSSI)`); a primeira é matematicamente quebrada (RSSI é negativo); a função de penalidade nunca é definida; valores de α e β não aparecem. Unificar, definir a função e declarar os pesos no Quadro 10.

- [ ] **4. Topologia forçada não explicada — Cenários C1–C4.** Com nós a 10 m e alcance de ~55 m, N1 alcança N5 diretamente. Declarar o mecanismo: whitelist de MACs, redução de potência TX ou atenuação física.

- [ ] **5. Medição de latência sem sincronização de relógios — Quadro 11.** `L = t_recepção − t_envio` exige base de tempo comum entre ESP32s. Definir o método: RTT/2, nó sniffer com timestamp único ou sincronização por GPIO.

## 🟠 Lacunas metodológicas (a banca vai perguntar)

- [ ] **6. LRU nunca será exercitada.** Testbed de 10 nós não enche tabela de 20 peers. Reduzir artificialmente o limite no firmware em experimento dedicado, ou declarar como limitação explícita.

- [ ] **7. C4 sem métrica própria.** Adicionar **tempo de reconvergência** (falha de N3 → primeira entrega via rota alternativa).

- [ ] **8. Hipóteses mencionadas mas nunca formuladas.** Formalizar H1 (NRL menor), H2 (PDR equivalente), H3 (energia menor) e especificar os testes estatísticos (ex.: Shapiro-Wilk + t pareado ou Mann-Whitney, α = 0,05).

- [ ] **9. Parâmetros divergem da RFC 3561 sem justificativa.** HELLO_INTERVAL 2.000 ms e ACTIVE_ROUTE_TIMEOUT 10.000 ms vs. 1.000/3.000 da RFC (Quadro 6). Justificar (redução de overhead).

- [ ] **10. Inconsistência de número de nós.** Quadro 10 diz "5–10 nós", mas cenário máximo usa 7; testbed tem 10 módulos. Esclarecer papel dos nós restantes.

- [ ] **11. Obtenção do RSSI não explicada.** Indicar a fonte no firmware (callback de recepção / `rx_ctrl`).

- [ ] **12. Segurança fora de escopo, sem declaração.** BRAM-NOW é mesh *segura*; AODV-EN não trata criptografia. Declarar explicitamente como limitação/trabalho futuro.

## 🟡 ABNT e formatação

- [ ] **13. Resumo vazio e numerado.** Resumo é elemento pré-textual sem numeração (NBR 14724), 150–500 palavras + palavras-chave. Renumerar tudo: Introdução = seção 1.

- [ ] **14. Legendas de figuras/quadros.** Identificação vai na parte **superior**, fonte na inferior (NBR 14724:2011). Hoje estão embaixo.

- [ ] **15. "Figura 7 – Fórmula do custo" não é figura.** Equações são numeradas à direita: `Custo(rota) = ... (1)`, referenciadas como "Equação 1".

- [ ] **16. Erro de digitação:** "4..4.2" (ponto duplo).

- [ ] **17. Crase indevida:** "às Redes Mesh Sem Fio emergem" → "as Redes Mesh... emergem" (sujeito não leva preposição).

- [ ] **18. Quadros vs. tabelas.** Quadros 3, 6 e 10 contêm dados numéricos — tecnicamente seriam *tabelas* (convenção IBGE). Verificar manual da instituição.

- [ ] **19. Tempos verbais inconsistentes.** Metodologia no futuro ("será implementado") vs. Capítulo 5 no passado ("foi concebida"). Harmonizar na versão final (passado ou presente).

## 🔵 Pendências estruturais

- [ ] **20. Capítulo 6 (Resultados) a escrever.** Estrutura sugerida: ambiente de execução → um subcapítulo por cenário (tabela média ± DP + IC 95%, gráficos com barras de erro, descrição + interpretação) → comparação com literatura (BRAM-NOW, Becker et al., com ressalvas) → verificação das hipóteses → limitações.

- [ ] **21. Conclusão (Capítulo 7) ausente.** Retomar objetivos específicos a–e, confirmando o atendimento de cada um.

---

**Ordem de ataque sugerida:** itens 1–5 primeiro (credibilidade técnica), depois 6–12 (blindagem para a defesa), 13–19 são revisão de forma rápida, e 20–21 dependem dos dados experimentais.
