# Galaxy Pen Mapper — plano atualizado (23/09/2026)

## Marco atual e próxima etapa

O usuário aprovou a evolução da versão 0.3.2 e autorizou publicar código e documentação no GitHub. A melhora de conforto em 800% foi relatada; o ganho exato usado após a ampliação não foi informado. Não converter essa aprovação em medição de força, compatibilidade geral ou aprovação de todos os níveis até 3.200%.

Próxima etapa funcional: [P009 — botões laterais](P009-BUTTONS-PLAN.md), começando pela evidência já capturada e sem alterar a base funcional de ponta/pressão. Validação de ciclo de vida e aplicativos adicionais continua separada. Perfis/autostart permanecem posteriores. Os registros abaixo preservam a sequência histórica.

## Histórico de implementação e testes

**0.3.2 instalada sem reinício do Windows:** atualização real aprovada, `oem34.inf` versão 0.3.2.0, hash do SYS correspondente, pilha correta e dispositivo OK. App novo em `build-sensitivity/driver/packages/col01-corrector` aberto para ajuste manual. Primeiro teste sugerido: 1.200%, piso 5%, depois aumentar só se necessário. Validação de conforto acima de 800% ainda pendente; não declarar pressão totalmente resolvida.

**Novo relato de melhora em 800% e revisão 0.3.2:** usuário conseguiu usar bem menos força e pediu maior sensibilidade. Limite ampliado para 3.200%, padrão 800%, mínimo artificial 5% preservado. App/driver compilados em `build-sensitivity`; sete testes e pacote assinado aprovados. Uso real acima de 800% ainda pendente. Iniciar comparação em 1.200%, sem força excessiva. O relato atual atualiza o rumo: não tratar as sessões anteriores saturadas como prova de que amplificação nunca ajuda. Ver [alteração e roteiro](P007-SENSITIVITY-032.md).

**Captura A/B concluída e Corrector restaurado:** 9.150 relatórios por observador, sem perdas. Nos 9.138 pares únicos, o Samsung só mudou flags de ferramenta; pressão permaneceu idêntica. A pressão já chegava em 4095 em 7.684/7.685 amostras do grupo de contato anterior ao Samsung. 12 ambiguidades foram excluídas, mantendo o resultado global inconclusivo do coletor e limitando as conclusões ao subconjunto único. Não prometer sensibilidade confortável por ajuste de ganho; saturação anterior ao Samsung demonstrada nesta sessão, causa abaixo dessa fronteira ainda não isolada. Corrector 0.3.1 restaurado (`oem34.inf`), pilha/saúde/hash verificados, sem novo reboot. App de bandeja fechado; ativação manual não presumida. Ver [resultado real A/B](P007-AB-LIVE-RESULT.md). Os itens abaixo são históricos.

**Diagnóstico A/B instalado após reinício do usuário:** pendências de serviços desapareceram; instalação elevada 04 concluída sem novo reboot. Pilha real confirmada `GalaxyPenDiagA > PenS2Helper > GalaxyPenDiagB > mshidkmdf`, dispositivo OK e preflight do coletor aprovado. Corrector temporariamente ausente; borracha pode reaparecer durante o experimento. Ainda não houve captura A/B nem comprovação de leituras/pareamento. Próximo passo: captura de 30 segundos quando o usuário estiver pronto, somente pressão confortável, seguida de análise e restauração. Backup desta instalação: `build-diagnostic/recovery/e9575c94a0fb49b3a0178e7dc93c4a41`. Os estados abaixo são históricos.

**Tentativa diagnóstica autorizada, ainda não concluída:** corrigidos dois erros no instalador (colisão `$Matches` e identificador SetupAPI). O Corrector 0.3.1 foi restaurado e seu binário/pilha confirmados, sem reiniciar Windows; nenhum observador foi anexado e nenhuma captura A/B ocorreu. Nova instalação bloqueada por serviços diagnósticos STOPPED, desativados e marcados para exclusão pelo Windows. Próximo passo seguro: após reinício voluntário, executar Check e conferir ausência dos serviços residuais antes de instalar novamente. Não forçar limpeza, não pedir teste com força elevada. Ver [estado e evidências](P007-DIAGNOSTIC-DEPLOYMENT.md).

**Implantação diagnóstica preparada, não executada:** pacote primitivo A/B assinado e verificado; gerenciador Check/Install/Restore com backup íntegro, diário persistente, alvo fixo, comparação da lista original e trava pela pilha real. Estratégia de posicionamento: lista legacy por instância, sem alterar o pacote Samsung. Testes simulados de falha/reboot/restauração aprovados; Check real aprovado. Instalação e estabilidade física não validadas; dependem de autorização específica e revisão da recuperação. Veja [procedimento e limites](P007-DIAGNOSTIC-DEPLOYMENT.md).

**Diagnóstico anterior/posterior ao Samsung preparado:** protótipo A/B e coletor em projeto separado, compilados com Ninja/WDK e testes offline aprovados. Sem instalação, INF ou assinatura; driver 0.3.1 preservado. Posicionamento garantido ao redor do Samsung e rollback de implantação ainda pendentes: o INF base não fornece níveis para simplesmente inserir dois filtros em ordem garantida. O coletor bloqueia a pilha atual antes de iniciar captura. Veja [preparação e limites](P007-DIAGNOSTIC-PREPARATION.md). Não declarar o experimento pronto para instalar.

**Novo resultado com força elevada:** `ponta-20260923-184127.csv` confirmou pressão variável 51–1024, 174 valores distintos em 3.189 contatos, sem borracha/inversão. A variação está demonstrada nessa condição, mas a exigência de muita força relatada pelo usuário mantém o aceite de sensibilidade confortável em aberto. Não repetir com força excessiva. Os resultados de saturação abaixo são históricos da sessão de toque anterior, não prova de ausência absoluta de variação.

**Situação atual 0.3.1:** atualização real concluída sem reiniciar Windows. Mínimo 5% confirmado: 1.542 contatos com pressão Windows 51, sem borracha; fora de contato, zero. O painel confirmou 6.176/6.176 contatos brutos saturados em 4095 antes da correção. Pressão dinâmica não resolvida: ganho não recupera variação ausente nesta entrada. A medição não separa hardware de processamento anterior por outros componentes. Não repetir teste com mais força nem reinstalar por esse resultado. Relato detalhado em [resultado real](P007-LIVE-RESULT.md). Os registros seguintes documentam a sequência anterior.

**Paint validado pelo usuário:** PW500 funcionou como lápis, sem borracha involuntária, após orientação para usar apenas a correção de ponta. Marco de ferramenta em aplicativo externo aprovado. A pendência funcional principal passa a ser pressão; testes de ciclo de vida continuam separados.

**Teste de pressão:** `ponta-20260923-180727.csv` confirmou traço sem borracha, porém pressão Windows 0 em todos os 3.156 contatos. Traço fino não equivale a pressão dinâmica resolvida. Rever o piso de saída/quantização e validar aplicativo externo; respeitar o limite de saturação do sinal original.

**Ponta validada em hardware:** após ativação manual, o novo CSV apresentou 1.359 eventos sem borracha/inversão e 480 de contato; a interface mostrou 2.428 relatórios alterados e traços azuis. Correção de ferramenta até WM_POINTER comprovada. Restam pressão, aplicativo externo e estabilidade de ciclo de vida.

**Avanço após reinício:** rollback COL04 concluído. Corrector 0.3.0 instalado/carregado em COL01 como `oem188.inf`, sem outro reinício; dispositivos OK e Tray conectado/desligado. Aguarda teste físico de leitura e desenho. Isso conclui os passos 1 e 2 da seção de próximo aceite; não conclui a validação real.

## Objetivo

**Primeiro teste real COL01:** leitura confirmada (15.128 relatórios, zero alterações/falhas registradas com modo desligado). O CSV `ponta-20260923-180107.csv` confirmou entrada original como borracha/invertida e pressão de contato 1024. Próximo marco: testar normalização de ponta, ainda sem inversão de pressão.

Usar a PW500 diretamente na tela do Galaxy Book3 360, com ativação manual pela bandeja, ponta reconhecida como caneta e pressão ajustável dentro do sinal disponível. Prioridade: ponta e traço; botões são opcionais. Não exigir distinção automática entre PW500 e S Pen.

## Evidência consolidada

As capturas existentes são suficientes para implementar/testar offline: não solicitar repetição ampla. PW500 COL01 apresenta `0x28` (Invert em hover) e `0x2C` (Invert + Eraser no contato, sem Tip normal). O descritor real confirma a interpretação; explica o Paint escolher borracha. S Pen de referência apresenta `0x20/0x21`.

A pressão bruta 0–4095 fica predominantemente saturada e cai com força. Inversão e ganho só aproveitam a variação residual. Não reconstruir pressão física ausente nem pedir força excessiva. Na HS611 com driver a pressão é normal e os botões emitem E/clique direito; as interrupções durante botões são comportamento antigo confirmado pelo usuário. Não confundir essa rota com a tela nem afirmar teste sem driver quando o usuário informa que ele já estava instalado.

## Marcos

| Marco | Estado e critério |
| --- | --- |
| P001/P002 — Scanner e monitor | Implementados; capturas reais e laboratório em memória disponíveis. |
| P003/P004 — Comparação/classificação | Evidência suficiente para ponta; mecanismo elétrico e distinção de botões não comprovados. |
| P005 — Substituição por EXE | Raw Input e injeção sintética não demonstram substituição confiável em todo o desktop. |
| P006 — Caneta sintética | Pausada; não gerar segunda entrada para simular solução. |
| P007A — Filtro COL04 | Rejeitado por ausência de leituras. Remoção e descarga concluídas. |
| P007B — Revisão COL01 | Interceptação e ferramenta confirmadas em 0.3.0; pressão 0.3.1 instalada, aceite físico pendente. |
| P007C — Captura interna da ponta | Confirmada por CSV real e comparação original/corrigido; pressão anterior zero identificada. |
| P007D — Atualização sem reinícios evitáveis | Troca real 0.3.0 → 0.3.1 aprovada com reinício só da coleção. Não garante todas as futuras trocas sem reboot. |
| P008 — Perfis/autostart | Adiados até o caminho real passar. |

## Implementação atual

Motor compartilhado entre testes e filtro: modo off preserva tudo; quando autorizado, normaliza somente flags PW500 conhecidas em report 0x02/15 bytes. Pressão é opção separada. A ativação aguarda levantamento da ponta; mudanças de configuração invalidam leituras antigas; fechamento, suspensão e expiração desarmam. Probe rejeita ativação no kernel. A interface valida versão/capacidades, usa uma única instância e distingue dados de entrada/saída do driver dos eventos recebidos pelo Windows.

O novo dispositivo de controle é criado durante anexação e excluído na remoção do último filtro. Isso corrige o vazamento de ciclo de vida anterior, mas não é prova de atualização sem reinício em todas as condições.

## Verificação realizada

- Ninja e sete CTests.
- Replay de 11 arquivos: 85.494 relatórios COL01, 75.073 normalizações; desligado e demais campos preservados. Temporização testada separadamente.
- Descritor HID real: ponta, pressão e posição validados somente em memória.
- WDK/análise estática, INF, catálogos, assinaturas e hashes.
- Inspeção visual da interface; não substitui teste físico da caneta.

## Próximo passo e conclusão real

1. Concluído: teste 0.3.1 confirmou mínimo positivo e entrada bruta totalmente saturada nesta sessão.
2. Pressão dinâmica permanece pendente; investigar evidência anterior/limitações antes do filtro antes de propor nova alteração. Não substituir força por velocidade ou simulação sem escolha explícita do usuário.
3. Confirmar comportamento da nova pressão em aplicativo externo.
4. Testar desligamento, fechamento e suspensão/retomada. Atualização local já passou uma vez; manter as proteções nas próximas trocas.

A meta só estará concluída após essas verificações em hardware. Não afirmar que o projeto está totalmente funcional só porque compilou. Falhas de interceptação, ciclo PnP e pressão residual continuam sendo riscos explícitos.

Veja [modo e campo de teste](P007-MANUAL-MODE.md), [atualização](P007-STACK-GATE.md) e [resultado real](P007-LIVE-RESULT.md).
