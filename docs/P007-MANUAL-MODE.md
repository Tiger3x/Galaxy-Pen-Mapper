# P007 — Modo manual e teste da ponta (0.3.2)

Este documento substitui o desenho anterior baseado em COL04. Em 0.3.0, interceptação e correção de ponta/borracha foram confirmadas por capturas e pelo usuário no Paint. A revisão 0.3.1 trata a pressão; veja a situação de instalação em [resultado real](P007-LIVE-RESULT.md).

## Campo de teste

A área branca do Tray observa exclusivamente eventos de caneta entregues pelo Windows àquela área, sem injetar entrada. Funciona mesmo com driver ausente ou correção desligada.

1. Aproxime a PW500 sem encostar: veja ferramenta e ausência de contato.
2. Faça traços leves e depois moderados; não use força excessiva.
3. Veja contato, pressão Windows (0–1024), traço azul de caneta ou laranja de borracha/invertida.
4. Salve CSV, de preferência na pasta de capturas existente, com nomes como `ponta-original.csv`, `ponta-corrigida.csv` e `ponta-pressao.csv`.
5. O programa avisa antes de descartar dados não salvos. O limite é 50.000 eventos por teste; ao atingir o limite, salva-se e limpa-se para recomeçar.

O CSV registra tempo relativo de recebimento, tipo de mensagem, ID, posição local, flags, contato, presença de pressão, pressão e inclinação. Não captura o desktop todo, não identifica fisicamente a caneta e não substitui o Raw HID detalhado. Eventos de mouse/toque não contam. Mensagens podem ser agrupadas pelo Windows: não é gravação de todos os pacotes do hardware.

## Correção

- Desligado por padrão; relatórios encaminhados sem mudanças.
- Ativação exige versão compatível, capacidade de correção, exatamente um dispositivo pronto e um relatório COL01 real desde a última retomada.
- Ao ativar/mudar curva, levante a ponta: a mudança aguarda estado de levantamento conhecido.
- `0x28 -> 0x20`: remove inversão em hover; `0x2C -> 0x21`: converte borracha/invertida em ponta.
- Somente report ID `0x02` com 15 bytes e pressão válida (0–4095). Outros IDs/estados/tamanhos e campos não envolvidos permanecem iguais.
- Inversão e ganho de pressão são opção independente, aplicados apenas ao contato PW500 conhecido. Na 0.3.2: ganho 100–3.200%, padrão 800%. Começar a comparação acima de 800% em 1.200%, reduzindo se o traço atingir o máximo cedo demais.
- Pressão inicial é um mínimo artificial de 1–25%, padrão 5%. A fórmula HID é `min(4095, ceil(4095 * mínimo / 100) + floor((4095 - entrada) * ganho / 1000))`, com ganho em milésimos. Não se aplica ao hover. Com entrada 4095 e mínimo 5%, saída 205; aproximadamente 51 na escala Windows, sujeito à conversão real.
- O painel informa mínimo/máximo bruto dos contatos e total saturado. Esses contadores reiniciam ao reconectar, mudar configuração ou transitar energia; não a cada renovação de autorização. Não são dados sincronizados com cada linha do CSV.
- Pressão saturada não contém resolução suficiente para reconstruir força real. Os botões não são remapeados.

O painel esquerdo exibe o último instantâneo de entrada e saída do driver, na escala HID 0–4095; o campo direito mostra o resultado Windows 0–1024. Não são amostras sincronizadas nem prova automática de funcionamento no Paint.

## Desligamento e manutenção

Fechar/liberar o identificador de controle desliga o modo no kernel. Suspensão, remoção e múltiplos dispositivos também desarmam. A autorização de operação expira em 2,5 segundos sem renovação; a expiração é conferida no processamento das leituras e consultas, não por um temporizador independente. Ao reconectar/retomar, a interface não reativa automaticamente.

**Liberar driver para manutenção** fecha a conexão e suspende reconexão. O campo de teste continua disponível. Para executar o atualizador, salve os dados e feche o Tray completamente; nenhum processo é encerrado à força.

O ícone da bandeja permite abrir, ativar/desativar quando disponível e sair. Alterar opções não reinstala o driver nem reinicia Windows. A sonda Probe não pode ser ativada nem pelo menu nem por outro cliente IOCTL.

## Validação pendente

Próximo teste dirigido: ponta e pressão ligadas, ganho 450%, mínimo 5%; levantar a ponta e fazer traços leves/moderados, sem força excessiva. Salvar CSV e observar intervalo bruto no painel. Contato positivo sem variação valida o mínimo, não pressão dinâmica. Confirmar também off/on, fechamento, suspensão/retomada e estabilidade. Não repetir capturas históricas ou os testes de ponta aprovados sem necessidade.
