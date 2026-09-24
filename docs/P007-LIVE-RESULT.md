# P007 — Evidência real e revisão 0.3.1

## Novo teste com força elevada — variação confirmada

O usuário informou ter aplicado bastante força (condição não desejável) e percebido resposta de pressão. `ponta-20260923-184127.csv` confirma 3.291 eventos em 9.281 ms, com 3.189 contatos e 174 valores distintos de pressão Windows entre 51 e 1024. Foram 1.239 contatos em 51, 1.950 acima de 51 e apenas um em 1024. Primeiro traço: 1.554 contatos, faixa 51–331; segundo: 1.635 contatos, faixa 51–1024. Todos os contatos têm pressão válida; as 102 amostras sem contato estão em zero; nenhum evento indica borracha/inversão.

Isso confirma variação da saída no Windows sob força elevada relatada pelo usuário, não uma curva física calibrada nem sensibilidade confortável com toque leve. O CSV não registra força aplicada, configuração ou pressão bruta sincronizada. O resultado anterior de entrada constante continua válido para aquela sessão; não representa incapacidade absoluta de variar. Ganho pode ampliar uma variação existente, mas não antecipar resposta enquanto a entrada permanecer em 4095. Não solicitar repetição com força excessiva. Prioridade restante: resposta com força confortável, sem declarar que aumentar ganho resolve a região de entrada saturada.

## Pressão 0.3.1 — mínimo confirmado, entrada sem variação

`ponta-20260923-183644.csv` contém 2.243 eventos em 8.531 ms, 1.542 amostras de contato com pressão válida, todas em 51/1024, e 701 amostras sem contato, todas em zero. Foram 8 down/8 up e nenhum evento de borracha/inversão. Isso confirma o mínimo artificial de 5% nesta sessão, não pressão dinâmica.

A inspeção da interface real, sem alterar opções, confirmou ponta e pressão ligadas, ganho 450%, mínimo 5%, intervalo bruto **4095–4095**, **6.176/6.176 contatos saturados**. Contadores gerais: 7.704 leituras recebidas, 7.703 concluídas, 1 pendente, 7.446 relatórios alterados, zero falhas de envio/acesso. O código coleta as estatísticas de contato antes de `GalaxyPenEngineProcess`; portanto a ausência de variação já ocorre na entrada do nosso filtro. O intervalo do painel abrange mais eventos que o CSV e não é uma correlação amostra a amostra.

Conclusão delimitada: nesta sessão não havia variação de pressão para inversão/ganho aproveitar. Não atribuir definitivamente a causa ao hardware: o filtro está acima de `PenS2Helper`, e esta medição não distingue caneta/digitizador de processamento anterior. Não prometer que outra curva recuperará força ausente, não pedir força excessiva, nem reinstalar o driver por este resultado. Os relatos/capturas anteriores com queda de pressão não são negados; não houve essa queda neste teste. A ferramenta segue funcional e o mínimo foi confirmado, mas pressão dinâmica continua não resolvida.

## Pressão 0.3.1 instalada — 23/09/2026, 21:33 UTC

A troca 0.3.0 → 0.3.1 foi concluída sem reiniciar o PC. O atualizador removeu apenas o pacote COL01 do projeto, reiniciou a coleção da caneta e instalou o novo Corrector. O Windows reutilizou `oem188.inf`; o INF agora declara 0.3.1.0. COL01/COL04 estão OK, com `GalaxyPenCol01Corrector` acima de `PenS2Helper`/`mshidkmdf`. Registro: `build-ninja/col01-pressure-update-031-20260923.json` e `.json.log`. Nenhuma política de segurança foi alterada.

O Tray 0.3.1 conectou ao canal V3 (configuração 3, estatísticas 5), desligado e aguardando leitura física: 1 recebida/pendente, 0 concluídas. Isso confirma comunicação com o novo filtro, não valida a nova pressão. A interface foi conferida pela habilidade de controle de interface, primeiro em simulação sem acesso ao driver e depois no app real.

A nova curva soma mínimo artificial ajustável de 1–25% (padrão 5%) à inversão amplificada, apenas em contato conhecido. Com entrada saturada 4095, a saída padrão é 205/4095, não 1/4095 como antes. O painel distingue intervalo bruto de contato e contagem saturada; força ausente não é reconstruída. Estatísticas reiniciam com reconexão, mudança de configuração e transições de energia.

Verificação: sete CTests, teste de segurança do instalador com PnP simulado, laboratório HID, WDK/análise estática, INF/catálogo/assinaturas e replay de 11 arquivos (85.494 relatórios; 75.073 normalizações) aprovados. O replay conta 36.534 contatos de assinatura PW500, 33.061 saturados, intervalo agregado 1–4095; misturar sessões/botões não demonstra uma curva de força útil. Backup recuperável do pacote anterior: `build-ninja/driver/packages/col01-corrector-0.3.0-backup`.

Próximo aceite: aproximar para liberar controles, ligar ponta e pressão, manter ganho 450% e mínimo 5%, levantar antes de desenhar. Traços leves/moderados apenas, salvar CSV e observar intervalo bruto. Pressão positiva constante valida somente o mínimo; variação real continua pendente.

## Validação no Paint — confirmação do usuário

Após orientação para manter ponta/borracha ligada e desmarcar inversão de pressão, o usuário confirmou: no Paint a PW500 funcionou como lápis, exatamente como esperado. Trata-se de confirmação do usuário, não de observação remota independente. Em conjunto com o CSV sem flags de borracha/inversão, o resultado valida a correção de ferramenta em um aplicativo externo. Pressão dinâmica, suspensão/retomada e atualização local do driver continuam pendentes. Não repetir os testes de ponta já aprovados sem uma regressão ou mudança relevante.

## Primeiro teste com inversão de pressão — resultado limitado

Após orientação para habilitar pressão a 450%, o usuário relatou traço fino e salvou `ponta-20260923-180727.csv`. Foram 4.027 eventos em 13.296 ms, incluindo 3.156 contatos com pressão válida, 9 down e 9 up. Nenhum evento apresentou borracha/inversão. Entretanto, TODOS os contatos tiveram pressão Windows igual a 0; não houve variação de pressão demonstrada. O campo interno desenha com espessura mínima mesmo quando a pressão é zero, portanto o traço fino não comprova sensibilidade funcional em aplicativos externos. O motor limita a saída bruta mínima a 1/4095; perda por quantização na conversão para 0–1024 é uma hipótese compatível, mas este CSV não contém os relatórios brutos para confirmá-la. Ponta/borracha continuam validadas; pressão dinâmica permanece pendente/limitada. Não pedir força excessiva nem classificar este teste como pressão plenamente corrigida.

## Ponta/borracha corrigida em hardware — 23/09/2026

O usuário ativou apenas ponta/borracha e confirmou traços azuis. `ponta-20260923-180407.csv` contém 1.359 eventos em 4.828 ms, todos com pen_flags=0 (nenhuma inversão/borracha), incluindo 480 eventos de contato, 8 mensagens down e 8 up. A pressão dos contatos permaneceu em 1024, pois a inversão de pressão estava desativada. A interface confirmou ponta ligada/pressão original, 17.568 relatórios COL01 e 2.428 alterados, sem falhas de envio/acesso registradas. Isso valida a normalização de ferramenta no caminho real até WM_POINTER, não apenas a cor do desenho. Próximo: pressão opcional e confirmação em aplicativo externo. Suspensão/retomada e atualização local continuam pendentes.

## Atualização após reinício — 23/09/2026, 20:56 UTC

O usuário reiniciou; COL04 não lista mais o filtro antigo. Corrector instalado como `oem188.inf`, serviço `GalaxyPenCol01Corrector` RUNNING, acima de `PenS2Helper` e `mshidkmdf` em COL01. Instalação terminou sem nova reinicialização. COL01 e COL04 estão OK. A interface real abriu conectada e desligada: 1 leitura recebida, 0 concluídas, 1 pendente, 0 relatórios. O próximo passo é movimentar a PW500 fisicamente; estes contadores iniciais não comprovam tráfego nem falha. Registro: `build-ninja/col01-install-after-reboot-20260923.json` e transcrição `.json.log`. As seções abaixo registram a situação anterior e a implementação; o bloqueio de descarga antiga foi resolvido.

## Resultado real anterior (COL04)

### Primeiro teste COL01 desligado — 23/09/2026

O usuário aproximou a PW500, desenhou traços laranja e salvou `ponta-20260923-180107.csv`. O arquivo contém 4.439 eventos em 13.968 ms, sendo 3.710 eventos de contato. Todos os eventos indicam inversão/borracha; a pressão nos contatos permaneceu em 1024. A interface real mostrou 15.128 relatórios de caneta, 15.129 leituras concluídas, 1 pendente, 0 relatórios alterados e 0 falhas de envio/acesso. Isso comprova que o novo gancho COL01 observa tráfego real e que o comportamento original é preservado nesta captura, não que a correção já funciona. A correção de ponta e a pressão continuavam desmarcadas após a tentativa de clique remoto; a próxima etapa é ativar manualmente apenas ponta/borracha, levantar a ponta e comparar novos traços.

O usuário usou a PW500 na tela com o Probe COL04 instalado; contadores permaneceram em zero. A pilha confirmou `GalaxyPenPassThrough` acima de `mshidkmdf`. No Paint, aproximar a PW500 seleciona borracha; pressionar botão volta temporariamente ao lápis. Esse resultado não é evidência de interceptação pelo filtro.

O INF `oem78.inf` foi removido, retornando 3010: sucesso com reinicialização necessária. A pilha ainda lista o filtro; reinício apenas do dispositivo foi recusado pela pendência. A revisão atual não instalou outro pacote por cima e não reiniciou a máquina.

## Descritor e capturas

O scanner decodifica o descritor HID real de COL01 inteiramente em memória, sem transmitir relatórios:

| Estado | Tip | Invert | Eraser | InRange |
| --- | ---: | ---: | ---: | ---: |
| 0x20 (S Pen hover) | 0 | 0 | 0 | 1 |
| 0x21 (S Pen contato) | 1 | 0 | 0 | 1 |
| 0x28 (PW500 hover) | 0 | 1 | 0 | 1 |
| 0x2C (PW500 contato) | 0 | 1 | 1 | 1 |

A correspondência explica o Paint escolher borracha. Ela não identifica inequivocamente cada botão nem mede o mecanismo elétrico da caneta.

A pressão COL01 é um campo de 16 bits em bytes 6–7 com intervalo lógico 0–4095. As capturas mostram saturação predominante e queda sob força. A escala Windows 0–1024 não é o campo bruto. Inversão/ganho não recuperam força que o digitizador não informou.

## O que foi corrigido no código

- Motor COL01 integrado à variante Corrector: flags 0x28→0x20 e 0x2C→0x21, pressão opcional e parâmetros em tempo de execução.
- Descarte do dispositivo de controle ao remover o último filtro, para não reter o driver por esse objeto.
- Desarme ao fechar, suspender, remover ou expirar autorização; bloqueio de leituras de configuração anterior.
- Probe rejeita ativação; interface valida protocolo/capacidades e não permite contornar pelo menu.
- Contadores de entrada, conclusão, pendência, falha e alteração; último estado/pressão antes e depois.
- Campo de teste independente: eventos Windows reais, desenho, pressão, ferramenta, CSV e proteção de dados não salvos.
- Atualizador por dispositivo, sem força nem reinício automático, bloqueando sobreposição com o filtro COL04 antigo.
- Textos UTF-8, ganho exibido corretamente, instância única e mensagem de bandeja corrigida.

## Verificações offline

Ninja e sete CTests; replay somente leitura de 11 capturas, 85.494 relatórios COL01 e 75.073 normalizações; laboratório de ponta/pressão/posição no descritor real. Os testes temporais são sintéticos e separados do replay. Os pacotes pessoais passam WDK/análise estática, verificações INF, assinatura/catálogo e hashes. A interface foi inspecionada visualmente em modo claramente simulado, sem acesso ao driver.

Não confundir testes offline com estabilidade em kernel real. Interceptação/ferramenta foram confirmadas em 0.3.0 e a troca local para 0.3.1 funcionou; cancelamento, suspensão/retomada e pressão dinâmica seguem pendentes.

## Histórico do marco de interceptação — resolvido em 0.3.0

Leituras de coleção HID usam `IRP_MJ_READ`; o tráfego HIDClass/minidriver usa `IOCTL_HID_READ_REPORT` em outra camada. O gancho COL01 foi posteriormente confirmado pelas capturas descritas acima. Se uma nova versão ficar sem relatórios após movimento físico, não forçar ativação nem ampliar o filtro por tentativa e erro.

Referências: [coleções HID](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/opening-hid-collections), [IOCTL_HID_READ_REPORT](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/hidport/ni-hidport-ioctl_hid_read_report).

## Próximo aceite

Validar a nova pressão conforme a seção 0.3.1 acima, depois aplicativo externo e ciclo de vida. Remoção antiga, interceptação e ferramenta no Paint já foram confirmadas. Não exigir novas capturas históricas longas nem declarar pressão dinâmica resolvida apenas pelo mínimo artificial.

Veja [plano](PLAN.md), [campo de teste](P007-MANUAL-MODE.md) e [atualização](P007-STACK-GATE.md).
