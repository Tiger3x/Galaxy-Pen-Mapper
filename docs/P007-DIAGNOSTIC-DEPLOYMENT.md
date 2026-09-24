# Diagnóstico A/B — candidato de implantação e recuperação

## Estado atual — ensaio encerrado, Corrector restaurado às 20:25

Captura `build-ninja/captures/diagnostic-ab-20260923-01` concluída. Restauração `restore-attempt-20260923-02.json` aprovada sem reboot: Corrector 0.3.1 carregado como `oem34.inf`, dispositivo OK, lista/pilha originais e hash conferidos. Diário `e9575c94a0fb49b3a0178e7dc93c4a41` em `restored`. Nenhum observador permanece anexado; chaves de serviços residuais bloqueiam nova instalação, não o uso do Corrector. App de bandeja fechado. Ver [análise e limites do experimento](P007-AB-LIVE-RESULT.md). A seção seguinte descreve o estado histórico antes da captura.

## Estado atual — instalado às 19:52 de 23/09/2026

Após o reinício confirmado pelo usuário (boot 19:49:22), Check confirmou ausência dos serviços residuais, Corrector saudável, app fechado e pacote íntegro. A tentativa elevada 04 concluiu sem outro reinício do Windows. Resultado: `build-diagnostic/install-attempt-20260923-04.json`, `Completed=true`, sem erro.

Pilha efetiva validada pelo instalador e pelo coletor `--preflight`: `GalaxyPenDiagA > PenS2Helper > GalaxyPenDiagB > mshidkmdf`. Dispositivo `OK / CM_PROB_NONE`. Lista UpperFilters `GalaxyPenDiagB, PenS2Helper, GalaxyPenDiagA` também confirmada pela leitura nativa independente. Testes simulados de implantação e regressão de busca do pacote aprovados novamente.

Recuperação desta instalação: `build-diagnostic/recovery/e9575c94a0fb49b3a0178e7dc93c4a41`, diário `installed-awaiting-physical-validation`. Para restaurar, usar esse diretório com `manage-pressure-diagnostic.ps1 -Action Restore`, elevado e app/coletor fechados; não reutilizar o diário restaurado da tentativa anterior.

Nenhuma captura foi iniciada. A leitura de relatórios, pareamento e sensibilidade confortável continuam sem validação física. A correção normal de ferramenta/pressão está temporariamente ausente; possível reconhecimento como borracha não indica regressão do Corrector. Próximo passo: usuário pronto para 30 segundos com pressão confortável; não pedir força excessiva. Analisar antes/depois e restaurar o Corrector ao fim do ensaio.

## Histórico de implantação real em 23/09/2026

**Estado final desta sessão:** Corrector 0.3.1 restaurado e carregado; observadores não anexados; nenhuma captura A/B realizada. A terceira tentativa (`install-attempt-20260923-03.json`) parou antes de qualquer nova mutação por serviços residuais. A desinstalação nativa removeu o pacote `oem194.inf`, mas o Windows manteve GalaxyPenDiagA/B com `Start=4`, `DeleteFlag=1`, estado STOPPED. Não apagar registros nem forçar descarga. O Check agora informa serviços presentes e `InstallBlocked`; o próximo passo conservador é o usuário reiniciar quando conveniente, verificar que esses registros desapareceram e só então tentar a instalação corrigida. O Windows não retornou NeedReboot nas operações desta sessão; a recomendação de reinício decorre dos registros pendentes observados, não de um retorno 3010. Nenhum reinício do PC foi executado.

Usuário autorizou a instalação elevada. A primeira tentativa parou antes de qualquer mutação: a variável `$matches` da busca de pacote colidia com a variável automática `$Matches` do PowerShell. Renomeada para `$ownedPackages`; teste de regressão `DiagnosticPackageLookupTest.ps1` aprovado.

A segunda tentativa registrou o pacote diagnóstico e removeu o Corrector, mas bloqueou antes de anexar observadores: o helper consultava `0x13` (BusTypeGUID), não `0x11` (UpperFilters), conforme o `SetupAPI.h` instalado. Nenhuma lista foi escrita. Restore real concluiu com pilha original saudável e hash do Corrector 0.3.1 verificado, sem reboot do Windows. Evidências: `build-diagnostic/install-attempt-20260923-02.json`, `restore-attempt-20260923-01.json`; recuperação `build-diagnostic/recovery/af0da3a977f3417bb7e2fc772eabf8dc`.

O helper agora usa a constante correta tanto na leitura quanto na escrita. A leitura nativa compartilhada foi exposta separadamente, comparada com a propriedade PnP real e incluída no preflight antes de qualquer mutação. `DiagnosticSetupReadTest.ps1` verifica isso sem instalar ou escrever no dispositivo; os testes simulados de instalação/recuperação também passaram. O Check carrega o helper, mas chama somente a leitura.

## Preparação anterior à autorização (histórico)

Candidato assinado e validado offline, **não instalado nem testado fisicamente**. A preparação anterior sem INF permanece histórica. Pacote desta revisão:

`build-diagnostic/packages/candidate-20260923-192218-665`

Inclui dois observadores, coletor, INF primitivo de serviços, catálogo e manifesto de hashes. INF passou InfVerif /u e /k; Inf2Cat não reportou erros/avisos; ambos os SYS e o catálogo foram assinados com o certificado pessoal já existente. A associação SYS/INF ao catálogo foi verificada. Nenhum certificado foi importado e nenhuma política de inicialização foi alterada.

## Estratégia de posicionamento

O pacote primitivo registra apenas os serviços A/B e seus binários no Driver Store. Não possui Manufacturer/Hardware IDs, AddFilter ou AddReg; sua instalação não anexa os observadores automaticamente.

O gerenciador separado usa SetupAPI no **único alvo fixo** `HID\WCOM016C&Col01\5&a6b5543&0&0000`. Somente depois de retirar o Corrector do projeto e confirmar a pilha original Samsung, solicita a lista de filtros por instância:

`GalaxyPenDiagB, PenS2Helper, GalaxyPenDiagA`

A lista legacy é ordenada de baixo para cima. O experimento não presume sucesso a partir desse registro: depois do reinício local da coleção, instalador e coletor exigem a pilha real completa **A > PenS2Helper > B > mshidkmdf**, lida por `DEVPKEY_Device_Stack`. Se a ordem ou saúde não corresponder, não inicia captura. Níveis declarativos existentes, filtros desconhecidos, outra coleção WCOM016C COL01 presente ou pai diferente bloqueiam a operação.

Não altera o INF assinado da Samsung, não remove seu serviço, não substitui seu binário e não cria filtros para toda a classe HID ou para o pai I2C. A alteração temporária da lista da coleção preserva o nome Samsung. Ainda assim é uma mudança real em kernel, com risco de falha do dispositivo ou reinício; não declarar garantia de estabilidade só por compilação.

Fontes de implementação: [filtros ordenados](https://learn.microsoft.com/en-us/windows-hardware/drivers/develop/device-filter-driver-ordering), [propriedade UpperFilters por instância](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/devpkey-device-upperfilters), [pacotes primitivos e instalação/remoção](https://learn.microsoft.com/en-us/windows-hardware/drivers/develop/creating-a-primitive-driver). O caminho legacy evita misturar os dois observadores com ordenação declarativa indefinida. A posição efetiva continua sujeita à verificação real.

## Gerenciador: somente leitura por padrão

`tools/manage-pressure-diagnostic.ps1` aceita `Check`, `Install` ou `Restore`. Check não carrega a API de mutação, não cria backup, não abre canais e não instala. Executado nesta revisão com sucesso: assinaturas/hashes válidos, pilha original saudável, binário instalado correspondente ao pacote 0.3.1. O app estava aberto, portanto precisa ser salvo/fechado antes de qualquer futura instalação.

Install/Restore exigem PowerShell x64 elevado. Não elevam automaticamente, não fecham processos à força e não iniciam a captura. **Instalação ainda depende de autorização específica do usuário.**

## Backup e etapas da instalação futura

Antes de qualquer mutação, Install cria um diretório novo em `build-diagnostic/recovery/<identificador>`, copia e verifica o pacote Corrector 0.3.1 e o candidato diagnóstico. Confere que o INF e SYS originais instalados correspondem ao backup, identifica o nome oem publicado pelo provedor/classe/nome exatos, recusa duplicados e serviços diagnósticos preexistentes.

Um diário persistido antes de cada passo registra o alvo, filtro original, pacote original e estado da operação. Atualizações são feitas em arquivo temporário com flush e substituição atômica, preservando a versão anterior. Restore pode ler a versão anterior se o diário atual estiver ilegível; o diário não substitui verificação de assinaturas, hashes e estado real.

Ordem:

1. Instalar os serviços observadores com `DiInstallDriver`, sem anexar.
2. Confirmar que a pilha continua original.
3. Remover apenas o pacote Corrector identificado, com `DiUninstallDriver`.
4. Solicitar reinício apenas de COL01 e confirmar `PenS2Helper > mshidkmdf`.
5. Alterar a lista de filtros somente se ela ainda for exatamente `PenS2Helper`.
6. Solicitar reinício apenas de COL01 e confirmar a pilha diagnóstica exata.

Qualquer pedido de reboot interrompe a sequência imediatamente. Nenhum `/force`, `/reboot`, desligamento do computador ou mudança de segurança é emitido. Uma falha não desencadeia tentativas cegas: o script informa o diretório de recuperação e para. Não repetir Install sobre estado parcial.

## Restauração futura

Restore exige o diretório de recuperação, dentro do local esperado, com diário e pacotes íntegros:

1. Se a lista ainda for exatamente `B, Samsung, A`, retorna a `Samsung`. Se houver entradas desconhecidas, não sobrescreve.
2. Reinicia somente COL01 quando necessário e confirma a ausência dos observadores antes de remover seus serviços/pacote.
3. Remove apenas o pacote diagnóstico identificado e correspondente ao backup.
4. Reinstala o Corrector do backup se necessário, reinicia COL01 e exige a pilha original saudável.
5. Confere o hash do SYS original restaurado.

O backup nunca é apagado pelo gerenciador. Se houver reboot pendente, aguardar o usuário reiniciar e executar Restore novamente. Se Windows não iniciar, estes scripts online não substituem um ambiente de recuperação: antes do ensaio em kernel, revisar as opções de recuperação da máquina e manter o backup acessível. Não foi prometida restauração automática após falha de inicialização.

## Testes feitos, sem tocar no dispositivo

- Compilação Ninja, teste de pareamento/fila/protocolo e compilação/análise estática WDK A/B aprovados.
- `tests/DiagnosticDeploymentTest.ps1`: backend simulado testa instalação, restauração, repetição de Restore, interrupções/falhas em prefixos, pedidos de reboot e bloqueio de filtros/pilhas desconhecidos. Nenhuma chamada PnP real nesses testes.
- Helper C# SetupAPI compilado; nenhum método mutante foi executado.
- Check real confirmou integridade, pilha e backup compatível; não substitui ensaio de Install/Restore.

## Próximo passo autorizado separadamente

Com app salvo/fechado, consentimento para a troca temporária e recuperação revisada, executar Install elevado. Se a pilha for aprovada, capturar 30 segundos com pressão confortável usando o coletor; não usar força elevada. Durante o diagnóstico a correção normal fica ausente, portanto a PW500 pode voltar a ser reconhecida como borracha. Restaurar o Corrector depois. O experimento pode terminar inconclusivo se o Samsung recriar requisições ou se ambas as fronteiras já estiverem saturadas.
