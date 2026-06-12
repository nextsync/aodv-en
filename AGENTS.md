# Session Bootstrap - aodv-en

Este projeto usa memoria compartilhada em:
- `/home/dioguin/Documentos/base_conhecimento`

Leitura obrigatoria no inicio da sessao (ordem):
- `/home/dioguin/Documentos/base_conhecimento/wiki/domains/networking/indexes/index.md`
- `/home/dioguin/Documentos/base_conhecimento/wiki/domains/networking/logs/log.md`
- `/home/dioguin/Documentos/base_conhecimento/wiki/domains/networking/entities/aodv-en-implementation.md`
- `/home/dioguin/Documentos/base_conhecimento/wiki/domains/networking/sources/aodv-en-project-docs.md`
- `/home/dioguin/Documentos/base_conhecimento/wiki/domains/networking/synthesis/aodv-en-adaptation-synthesis.md`
- `/home/dioguin/Documentos/base_conhecimento/wiki/domains/networking/comparisons/rfc-3561-vs-aodv-en.md`

Contrato de continuidade:
- continuar a partir do ultimo estado marcado como pendente/em andamento;
- evitar reabrir decisoes ja implementadas e registradas no log;
- se wiki e codigo divergirem, prevalece o codigo e a wiki deve ser atualizada.

Ao concluir implementacao:
- atualizar paginas impactadas em `wiki/domains/networking/`;
- atualizar indice se a navegacao mudar;
- registrar evento em `/home/dioguin/Documentos/base_conhecimento/wiki/domains/networking/logs/log.md`.

Contrato global:
- `/home/dioguin/Documentos/base_conhecimento/schema/assistant-bootstrap-global.md`
