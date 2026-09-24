# P009 — Botões laterais da PW500 na tela

Próxima etapa proposta após a aprovação do usuário para publicar a versão 0.3.2. Nenhum mapeamento de botão é implementado nesta versão.

## Objetivo

Identificar os acionamentos dos botões e, se houver sinal suficiente, oferecer mapeamento opcional no aplicativo sem regredir ponta, pressão ou o modo desligado.

## Evidências e cuidados

- Na mesa Huion, botão 1 estava configurado como tecla E e botão 2 como clique direito. Essa configuração pertence ao caminho da mesa, não prova comandos equivalentes na tela Samsung.
- Na tela, já foram observadas interrupções e mudanças de contato durante os botões. O usuário confirmou que a interrupção da ponta faz parte do comportamento habitual; não tratá-la automaticamente como defeito.
- Os testes anteriores não estabeleceram uma assinatura inequívoca para cada botão. Duração de pausa ou ordem dos cliques não é identificador confiável.
- As capturas anteriores são o ponto de partida. Não pedir uma nova bateria de testes sem apontar a informação específica que falta.

## Sequência

1. Revisar os intervalos conhecidos de cada botão nas capturas existentes, incluindo flags, pressão, presença do dispositivo e diferenças entre hover e contato.
2. Separar o que é sinal de botão do que pode ser levantamento, saída de alcance ou falta de relatório. Excluir intervalos ambíguos.
3. Determinar se há dois estados distinguíveis, apenas um evento comum, ou informação insuficiente. Registrar a conclusão antes de escolher a arquitetura do mapeamento.
4. Se a identificação for viável, projetar comandos configuráveis, desligados por padrão. Confirmar com o usuário quais ações deseja, sem assumir E/clique direito como preferência definitiva para a tela.
5. Implementar primeiro testes offline com acionamento, soltura, repetição e perda de sinal. Evitar tecla/botão preso ao desativar, fechar, desconectar ou suspender.
6. Fazer somente o teste físico direcionado necessário, mantendo a versão funcional disponível para retorno.

## Aceite

- Ponta e pressão continuam funcionando como na referência 0.3.2.
- Botões não disparam por um simples levantamento da ponta ou perda de alcance.
- Cada ação tem soltura correspondente e não permanece presa.
- Modo desligado não injeta comandos.
- Se os dois botões não forem distinguíveis, não apresentar dois mapeamentos como implementados.

Não alterar o driver Samsung, enviar comandos de firmware desconhecidos ou interpretar pausas como cliques por tentativa.
