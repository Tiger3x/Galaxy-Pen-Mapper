# P007 — Instalação, atualização local e limites

**Versão atual 0.3.2:** instalada como `oem34.inf`, com hash do SYS conferido, dispositivo OK e sem reiniciar Windows. Pacote em `build-sensitivity/driver/packages/col01-corrector`; o 0.3.1 de `build-ninja` foi preservado para retorno. Os comandos abaixo usam explicitamente o pacote atual. Os dois parágrafos seguintes registram atualizações anteriores.

**Troca real 0.3.0 → 0.3.1 concluída em 23/09/2026:** pacote anterior removido, COL01 reiniciada e novo pacote instalado sem reiniciar o PC. `oem188.inf` foi reutilizado e declara 0.3.1.0; Tray conectado ao canal V3, COL01/COL04 OK. Log: `build-ninja/col01-pressure-update-031-20260923.json.log`. A troca aprovada não garante ausência de reboot em toda situação futura. Os parágrafos seguintes preservam o histórico.

**Atualização após reinício:** a pendência descrita abaixo foi resolvida. COL04 está sem o filtro antigo e o Corrector foi instalado como `oem188.inf`, carregando em COL01 sem outro reinício. A descarga/troca de uma versão COL01 já instalada ainda não foi testada; a primeira instalação bem-sucedida não comprova esse cenário.

## Histórico anterior à instalação COL01

COL04 ainda lista `GalaxyPenPassThrough` após a remoção de `oem78.inf`, que retornou 3010 (sucesso com reinicialização necessária). Um reinício apenas do dispositivo foi anteriormente recusado por essa pendência. O novo instalador bloqueia instalação/atualização enquanto esse filtro antigo permanecer carregado. Não reinicia Windows automaticamente.

A pilha COL01 contém o filtro Samsung `PenS2Helper` e `mshidkmdf`. O projeto não remove nem substitui esses componentes. Interceptação e normalização de ferramenta foram confirmadas em 0.3.0, inclusive pelo usuário no Paint.

## Pacotes atuais

Os INFs atuais `col01-probe` e `col01-corrector` declaram versão 0.3.2.0 e correspondem especificamente a `HID\VEN_WCOM&DEV_016C&Col01`. O pacote Corrector 0.3.2 foi compilado e instalado na máquina de teste. Configuração protocolo 4 (24 bytes); estatísticas protocolo 5 (96 bytes). O canal de controle V4 impede conexão com o app V3 anterior. Backups 0.3.0/0.3.1 são históricos, não o pacote atual.

O empacotador executa Ninja, sete testes, laboratório de descritor em memória, WDK com análise estática, InfVerif, Inf2Cat e assinatura/verificação pessoal. Nenhuma dessas etapas comprova interceptação real.

## Sem reiniciar a cada ajuste

O driver permanece instalado; ponta/borracha, pressão e sensibilidade são opções em tempo de execução. O modo desligado já serve para observar. Não há necessidade de alternar Probe e Corrector em cada teste.

Para atualizar o binário, o código agora elimina o dispositivo de controle quando o último dispositivo filtrado é removido. Isso corrige uma causa de retenção do driver antigo. Identificadores abertos, leituras pendentes e restrições do Windows ainda podem impedir descarga imediata.

## Procedimento administrativo

Na raiz do repositório, PowerShell elevado:

```powershell
$package = "$PWD\build-sensitivity\driver\packages\col01-corrector"

# Apenas verificar (não instala)
.\tools\install-col01-driver.ps1 -Variant Corrector -PackageDirectory $package

# Primeira instalação, SOMENTE após limpar a pilha antiga
.\tools\install-col01-driver.ps1 -Variant Corrector -PackageDirectory $package -Install

# Troca explícita do novo pacote, tentando reinício apenas da caneta
.\tools\install-col01-driver.ps1 -Variant Corrector -PackageDirectory $package -Update
```

Antes da troca, salve capturas e feche o Tray. O atualizador:

1. Verifica versão, hashes, assinaturas e alvo exato.
2. Bloqueia filtro COL04 antigo, dispositivo com problema e aplicação aberta.
3. Resolve os nomes publicados apenas dos dois INFs COL01 do projeto, com fornecedor/classe esperados.
4. Remove esses pacotes sem força, reinicia somente a coleção COL01 e exige pilha limpa e dispositivo saudável.
5. Instala o novo pacote; informa eventual código 3010 e não reinicia o sistema.

Se a remoção exigir reinício, a instalação seguinte é interrompida. Se a instalação falhar após remover o filtro novo, não reinstale um filtro antigo automaticamente: preserve a pilha Samsung e examine o relatório. Nunca use remoção da classe HID, do controlador I²C ou de pacotes Samsung para contornar a pendência.

Nenhum script altera TESTSIGNING, Secure Boot, certificados confiáveis ou BitLocker. A política de assinatura pessoal precisa estar configurada previamente. Não usar kdmapper nem forçar descarga.

## Critérios de aceite

- Pilha antiga limpa; apenas um filtro do projeto.
- Leituras reais COL01 aumentam; desligado não muda entrada.
- Ao ativar ponta/borracha, hover não desenha e contato desenha sem borracha involuntária.
- Campo interno e aplicativo externo concordam; saída do driver sozinha não basta.
- Pressão opcional cresce na faixa de sinal disponível, sem exigir força excessiva.
- Desativar/fechar/suspender/retomar não deixa contato preso nem reativa sozinho.
- Atualização local e descarregamento comprovados em hardware; se Windows exigir reinício, reportar o limite honestamente.

Referências: [PnPUtil](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/pnputil-command-syntax), [ciclo de vida de dispositivos de controle KMDF](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/using-control-device-objects).
