# Galaxy Pen Mapper

Use a **Huion PW500 na tela do Samsung Galaxy Book3 360**, com correção de ponta/borracha, pressão invertida e sensibilidade ajustável pelo aplicativo.

Projeto experimental de uso pessoal, desenvolvido e testado em uma máquina específica. Não é um driver oficial Samsung, Wacom ou Huion, nem um pacote com assinatura de produção para instalação geral.

## Versão atual: 0.3.2

A ponta foi validada na área de teste e no Paint, sem ativar a borracha involuntariamente. O usuário relatou redução importante da força necessária com ganho de 800%; a versão 0.3.2 amplia o ajuste até **3.200%**. A atualização foi instalada na máquina de desenvolvimento sem reiniciar o Windows e aprovada pelo usuário para continuidade do projeto.

| Recurso | Situação |
| --- | --- |
| Correção de ponta/borracha | Implementada e validada na máquina de teste |
| Inversão e amplificação da pressão | Implementadas; melhora de conforto relatada em 800% |
| Sensibilidade | 100–3.200%, padrão 800%, passos de 10% |
| Pressão inicial | 1–25%, padrão 5%; mínimo artificial, não força medida |
| Controle manual pela bandeja | Ativar/desativar; inicia desligado |
| Área de desenho e captura CSV | Eventos reais de caneta, contato e pressão |
| Botões laterais | Ainda sem mapeamento; próxima investigação |

O ganho ideal depende do comportamento observado. A aprovação de uso pessoal não equivale a certificação, compatibilidade com outras máquinas ou validação de todos os aplicativos e ciclos de suspensão/retomada.

## Como funciona

O projeto combina um **filtro KMDF** na coleção de caneta `WCOM016C COL01` e um **aplicativo Windows** que controla a correção. O driver Samsung continua instalado.

- O filtro normaliza os estados conhecidos da PW500, para que a ponta seja reconhecida como caneta em vez de borracha.
- Opcionalmente, inverte a pressão e amplia a variação recebida.
- O aplicativo permite ajustar o ganho, ativar/desativar a correção e observar o resultado.
- Fechar o aplicativo libera o controle e desativa a correção. Não é um remapeamento baseado apenas em injeção de mouse.

O modo é manual e **não distingue automaticamente PW500 de S Pen**. Desative a correção ao usar outra caneta. Botões configurados no driver da mesa Huion não se tornam automaticamente comandos da tela Samsung.

## Uso no computador já preparado

É necessário ter o driver correspondente instalado. O EXE sozinho não substitui o filtro.

1. Abra `GalaxyPenMapperTray.exe` da versão 0.3.2 e confirme a solicitação de administrador.
2. Aproxime a caneta da tela para liberar os controles após uma leitura real.
3. Marque **Corrigir ponta / borracha**.
4. Para testar pressão, marque **Inverter e ampliar pressão (experimental)**.
5. Comece em **800%**. Se precisar de mais ganho, experimente **1.200%**, depois **1.600%** e **2.000%**. Não é necessário ir direto ao máximo.
6. Mantenha **Pressão inicial em 5%** durante a comparação e levante a ponta após cada alteração.

Faça traços leves a moderados, sem forçar a tela. Se o traço engrossar muito cedo ou perder gradação, reduza o ganho. O objetivo é usar o menor ganho confortável.

Na área branca, **azul** indica caneta e **laranja**, borracha/inversão. Use **Salvar captura CSV** para guardar o teste. O painel distingue a pressão do driver (0–4095) da pressão entregue pelo Windows (0–1024); essas leituras não são amostras sincronizadas.

Alterar opções no painel **não exige reinstalação nem reinício**. A ativação é manual; não presuma que ela será retomada após fechar o app ou suspender o computador.

## Limites importantes

A PW500 apresentou pressão invertida e frequentemente saturada nessa tela. O ganho amplia pequenas variações existentes, mas não recupera informação quando o valor de entrada permanece constante. Ganhos maiores também podem amplificar ruído e atingir o máximo mais cedo.

O mínimo inicial evita que um contato conhecido seja convertido em pressão praticamente zero, mas **não mede a força aplicada**. O projeto não promete a mesma resposta de uma caneta oficialmente compatível.

O ensaio antes/depois do componente Samsung encontrou mudança nas flags de ferramenta, mas não na pressão dos pares sem ambiguidade. Isso não isola a causa entre processamento inferior, firmware e compatibilidade física. Os detalhes e limites estão no [relatório A/B](docs/P007-AB-LIVE-RESULT.md).

## Compilar os aplicativos

Requisitos: Windows x64, Visual Studio 2022 Build Tools com C++, CMake e Ninja. No terminal de desenvolvimento x64, na raiz do repositório:

```powershell
cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-ninja
ctest --test-dir build-ninja --output-on-failure
```

| Executável | Finalidade |
| --- | --- |
| `GalaxyPenMapperTray.exe` | Controle do driver e teste de desenho |
| `GalaxyPenEventMonitor.exe` | Captura de Raw HID, WM_POINTER e teclado/mouse para investigação |
| `GalaxyPenHidScanner.exe` | Enumeração HID e laboratório de descritor em memória |
| `GalaxyPenMapperUiCheck.exe` | Simulação visual de desenvolvimento; não acessa o driver |

O workflow do GitHub compila os aplicativos, executa testes e disponibiliza os aplicativos como artefato. **Ele não produz nem instala um driver assinado.**

## Driver: compilação e instalação pessoal

**Atenção:** instalar um filtro de entrada modifica a pilha de dispositivos do Windows. Preserve uma forma alternativa de entrada e um procedimento de recuperação. Não instale em outra máquina presumindo que o hardware seja equivalente.

Os scripts de empacotamento usam caminhos locais do Visual Studio/WDK, o dispositivo de desenvolvimento e um certificado de teste pessoal já existente. Os scripts de instalação verificam o certificado esperado e o alvo exato. **Não são um instalador portátil para terceiros.** Clonar o repositório não fornece a chave privada, o certificado confiável nem a preparação de assinatura do Windows.

Na máquina preparada, o pacote 0.3.2 pode ser gerado separadamente para preservar os binários anteriores:

```powershell
.\tools\package-col01-driver.ps1 -Variant Corrector -BuildDirectory "$PWD\build-sensitivity"
```

O empacotador executa Ninja, testes, laboratório HID em memória, build WDK, análise estática, validação INF e assinatura/verificação do pacote. Não instala.

Para verificar o pacote e a máquina **sem instalar**:

```powershell
$package = "$PWD\build-sensitivity\driver\packages\col01-corrector"
.\tools\install-col01-driver.ps1 -Variant Corrector -PackageDirectory $package
```

Com a captura salva e o aplicativo fechado, em PowerShell elevado, escolha **apenas a ação adequada**:

```powershell
# Primeira instalação, somente em pilha livre e máquina previamente preparada:
.\tools\install-col01-driver.ps1 -Variant Corrector -PackageDirectory $package -Install

# Atualização de um filtro COL01 do projeto já instalado:
.\tools\install-col01-driver.ps1 -Variant Corrector -PackageDirectory $package -Update
```

App e driver 0.3.2 usam o canal **V4**. Não misture o app antigo V3 com o driver novo. Não instale Probe e Corrector simultaneamente, nem utilize o antigo filtro COL04.

O atualizador tenta reiniciar apenas a coleção da caneta. Se o Windows exigir reinicialização, ele interrompe o fluxo e informa a pendência; **não reinicia o PC automaticamente**. Não remove o driver Samsung, não força descarregamento e não altera políticas de segurança. Veja [instalação, atualização e limites](docs/P007-STACK-GATE.md).

## Verificação

- Sete testes automatizados cobrem captura, transformação, protocolo e proteções do motor.
- A transformação de pressão é testada em toda a faixa bruta, nos 25 pisos e em todos os passos de ganho até 3.200%.
- O motor verifica ausência de pressão artificial em hover, preservação dos campos não envolvidos, troca de configuração e expiração do controle.
- A versão pessoal 0.3.2 passou build WDK, análise estática e validação do pacote; sua instalação real foi confirmada pelo estado do dispositivo e pelo hash do binário.

Essas verificações não substituem testes físicos de estabilidade, suspensão/retomada e comportamento em outros aplicativos. Capturas e pacotes locais não fazem parte do repositório.

## Próximos passos

1. **Botões laterais:** revisar os sinais já capturados na tela e verificar se os dois botões podem ser identificados de forma confiável. Depois, implementar mapeamento opcional sem prejudicar ponta e pressão.
2. Validar estabilidade de uso, desligamento e suspensão/retomada.
3. Evoluir perfis e conveniência da bandeja, mantendo o controle manual.

Uma pausa no sinal não é, por si só, identificação de um botão. Se os sinais forem indistinguíveis, essa limitação será registrada antes de prometer dois mapeamentos independentes.

## Documentação

- [Sensibilidade 0.3.2 e roteiro de teste](docs/P007-SENSITIVITY-032.md)
- [Modo manual e área de teste](docs/P007-MANUAL-MODE.md)
- [Instalação e atualização](docs/P007-STACK-GATE.md)
- [Próxima etapa: botões](docs/P009-BUTTONS-PLAN.md)
- [Plano e histórico do projeto](docs/PLAN.md)
- [Resultados de ponta e pressão](docs/P007-LIVE-RESULT.md)
- [Diagnóstico antes/depois do Samsung](docs/P007-AB-LIVE-RESULT.md)

Os documentos de diagnóstico preservam experimentos e caminhos locais históricos. Eles não são instruções para instalar filtros de investigação durante o uso normal.
