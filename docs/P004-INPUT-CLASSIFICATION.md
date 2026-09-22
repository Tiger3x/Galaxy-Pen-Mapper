# P004 — Input Classification

## Capturas analisadas

As quatro sessões comparativas iniciais e 15 sessões direcionadas foram gravadas pelo Galaxy Pen Diagnostic Studio:

| Sessão | Caminho | Dispositivo Raw principal | Report ID |
| --- | --- | --- | --- |
| `S_Pen` | S Pen diretamente na tela | `WCOM016C`, usage `0x0D/0x02` | `0x02` |
| `Huion500` | PW500 diretamente na tela | `WCOM016C`, usage `0x0D/0x02` | `0x02` |
| `HUION500_MESA SEM DRIVER` | PW500 na HS611 sem driver Huion | `VID_256C&PID_006D`, usage `0x0D/0x02` | `0x0A` |
| `HUION500_MESA COM DRIVER` | PW500 na HS611 com driver Huion | `tablethid`, usage `0x0D/0x01` | `0x05` |

As sessões direcionadas cobriram hover, pressão, cada botão isolado em hover e ponta + botão. Os dois testes de ponta + botão na tela foram repetidos. A sessão rotulada `P004_HS611_NODRIVER_HOVER` foi feita com o driver instalado; o próprio arquivo confirma o caminho com driver pelo relatório `0x05`, usage `0x0D/0x01` e VID/PID `00FF:BACC`.

## Resultado principal: pressão da PW500 na tela

A pressão fixa em `1024` não é introduzida pelo monitor. O relatório Raw HID do digitizador WCOM já chega saturado:

```text
PW500 hover:   02 28 ... 00 00 ...
PW500 contato: 02 2C ... FF 0F ...
```

Os bytes 6–7 do relatório `0x02`, em little-endian, contêm a pressão bruta. Na primeira captura eles permaneceram em `0x0FFF` (`4095`) durante todo contato, exatamente como observado na tela: pressão Windows fixa em `1024`.

O teste direcionado de pressão revelou uma nuance: houve alguma variação, mas apenas na faixa superior. Foram 10.747 amostras em contato, com pressão Windows entre `800` e `1024`; a mediana foi `1024` e 7.444 amostras (`69,3%`) ficaram totalmente saturadas. A pressão bruta variou de `3201` a `4095`. O usuário observou a direção dessa variação: o contato leve já começa em `1024` e só com força excessiva o valor **cai** para `800` e arredores. Portanto, além de comprimido e saturado, o trecho que ainda varia responde no sentido contrário ao esperado. Os CSVs confirmam a faixa de valores; a relação entre força aplicada e direção da mudança vem da observação durante o teste.

Na S Pen, os mesmos bytes variam. Um exemplo medido foi pressão bruta `0x02BB` (`699`) convertida em pressão Windows `174`. A mediana da razão Raw/Windows foi `4,005`, confirmando a escala aproximada de 12 para 10 bits.

Conclusão: a PW500 é detectada por ressonância na tela, mas o digitizador/firmware não decodifica corretamente seu curso de pressão. Uma correção sobre `WM_POINTER` poderia inverter e reescalar apenas a pequena faixa que ainda varia. A região fixa em `1024` já perdeu a informação de força e não pode ser reconstruída a partir desse relatório.

## Classificação dos relatórios

### WCOM `0x02` — tela do Galaxy Book3 360

| Campo | S Pen | PW500 na tela | Interpretação |
| --- | --- | --- | --- |
| Byte 1 em hover | `0x20` | `0x28` | ferramenta/modo diferente |
| Byte 1 em contato | `0x21` | `0x2C` | ponta normal versus borracha/invertida |
| Bytes 6–7 | variável | `0C81`–`0FFF` no teste direcionado; frequentemente `FF 0F` | pressão little-endian de 12 bits, severamente comprimida |
| Windows `penFlags` | `0` | `2` em hover; contato alternou entre `6` e `0` por aproximação | PW500 é frequentemente interpretada como `INVERTED`/`ERASER`, mas a classificação não é estável |

O padrão `0x28 → 0x2C` coincide com a interpretação incorreta do Windows. A PW500 não segue o mesmo padrão `0x20 → 0x21` da S Pen.

### Botões da PW500 diretamente na tela

Em hover, os dois botões mantiveram o Raw HID em `0x28`, sem pressão e sem bits de botão distintos. Cada série de presses reiniciou repetidamente o objeto `WM_POINTER`: foram 21 `POINTER_ENTER` e 21 `POINTER_LEAVE` em cada teste.

Com a ponta fisicamente apoiada, os testes dos dois botões produziram pares adicionais `POINTER_UP`/`POINTER_DOWN`. Uma leitura anterior considerou apenas os relatórios WCOM `0x2C` imediatamente antes e depois desses pares e concluiu, incorretamente, que o contato Raw era contínuo. A inspeção do **intervalo inteiro** mostrou dois padrões: relatórios intermediários `0x28` com pressão zero, ou uma pausa sem relatórios WCOM gravados. Por exemplo, na repetição do botão 1, entre `14:16:04.346` (`POINTER_UP`) e `14:16:04.396` (`POINTER_DOWN`) há 19 relatórios `0x28` e nenhum `0x2C`. Em outro acionamento do botão 1, o último `0x2C` antes da interrupção é de `14:14:01.157` e o próximo é de `14:14:01.325`: 168 ms sem relatório WCOM gravado. No botão 2 também aparecem esses dois padrões; um intervalo sem relatório vai de `14:14:44.186` a `14:14:44.381` (195 ms).

Os dois botões apresentaram o mesmo comportamento no caminho WCOM e não puderam ser diferenciados por um bit Raw HID. A recriação do contato explica cliques extras, duplo clique e interrupções percebidas como “desativar” a caneta.

O usuário explica que, ao pressionar um botão, a própria PW500 troca o sinal da ponta pelo sinal do botão. Como a tela do Galaxy não tem ação correspondente mapeada, a ponta fica sem ação até o sinal normal retornar. Os relatórios `0x28` e as pausas de captura durante o acionamento são compatíveis com essa explicação e corrigem a hipótese anterior de um problema apenas na interpretação de `WM_POINTER`. A captura do computador, porém, não observa o sinal EMR diretamente: ela não revela qual código a caneta emitiu durante as pausas, nem permite provar a troca elétrica de sinal. Nos relatórios que o WCOM expõe, os dois botões continuam sem identificador distinto.

### HS611 sem driver — relatório `0x0A`

| Byte 1 | Estado observado | Resultado no Windows |
| --- | --- | --- |
| `0xC0` | hover | sem contato, pressão zero |
| `0xC1` | ponta | contato e pressão variável |
| `0xC2` | botão lateral | `PEN_FLAG_BARREL`, sem contato |
| `0xC3` | ponta + botão lateral | pressão bruta presente; `PEN_FLAG_BARREL` |
| `0xC4` | segundo botão | contato/clique com pressão zero |

Os bytes 6–7 guardam pressão little-endian. A razão Raw/Windows mediana foi `8,011`, consistente com redução para o intervalo de 10 bits.

O estado `0xC4` é especialmente importante: foram observados cinco `POINTER_DOWN` e 261 `POINTER_UPDATE` em contato, todos com pressão zero. Isso explica como o segundo botão pode provocar clique ou duplo clique sem a ponta tocar a superfície.

### HS611 com driver — relatório `0x05`

| Byte 1 | Estado observado |
| --- | --- |
| `0x02` | hover |
| `0x03` | contato |

Os bytes 6–7 mantêm pressão variável, com razão Raw/Windows mediana `8,009`. No teste direcionado a pressão Windows percorreu `0`–`919`, sem saturação em `1024`.

Nos testes isolados de botão em hover, o relatório permaneceu `0x05/0x02`, `penFlags` permaneceu zero e não houve `POINTER_DOWN`. Nos testes com ponta + botão, o relatório permaneceu limitado a `0x02`/`0x03`; todas as transições de contato Windows coincidiram com transições Raw hover/contato. Portanto, os botões não estão expostos pela coleção Digitizer `0x05`.

### P004B — destino dos botões configurados no driver

O usuário configurou o botão 1 como tecla `E` e o botão 2 como clique direito. Quatro capturas com o monitor P002.4 confirmaram o caminho:

- `pen-events-20260922-172716.csv`: botão 1 em hover;
- `pen-events-20260922-172808.csv`: botão 2 em hover;
- `pen-events-20260922-172922.csv`: ponta + botão 1;
- `pen-events-20260922-173015.csv`: ponta + botão 2.

| Teste | Eventos atribuídos ao botão | Origem reportada pelo Windows | Digitizer da HS611 |
| --- | --- | --- | --- |
| Botão 1 em hover | 30 acionamentos de `E` (30 down + 30 up; tecla virtual `69`) | teclado, `IMO_INJECTED` (`2`) | somente `0x05/0x02`, sem contato |
| Botão 2 em hover | 27 cliques direitos (27 down + 27 up) | mouse, `IMO_INJECTED` (`2`) | somente `0x05/0x02`, sem contato |
| Ponta + botão 1 | 20 acionamentos de `E` | teclado, `IMO_INJECTED` (`2`) | `0x05/0x02` e `0x05/0x03` |
| Ponta + botão 2 | 36 cliques direitos | mouse, `IMO_INJECTED` (`2`) | `0x05/0x02` e `0x05/0x03` |

Cada evento injetado aparece tanto no Raw Input quanto na mensagem da janela; são duas observações do mesmo acionamento, não dois acionamentos independentes. Os valores `IMDT_KEYBOARD=1`, `IMDT_MOUSE=2` e `IMO_INJECTED=2` foram conferidos no SDK do Windows instalado. O monitor não encontra um `hDevice` físico nos eventos Raw Keyboard/Mouse sintetizados; por isso o campo `device` fica vazio. A configuração fornecida pelo usuário é necessária para atribuir `E` ao botão 1 e clique direito ao botão 2.

Nos dois testes com ponta, houve 42 ciclos `0x02→0x03→0x02` no relatório bruto e 42 pares `POINTER_DOWN`/`POINTER_UP`, acompanhados de 42 pares de clique esquerdo com origem `IMDT_PEN=8`, `IMO_HARDWARE=1`. Essas transições são anteriores à tradução dos botões em tecla ou clique direito. Esses dois arquivos, isoladamente, não registram o estado físico da ponta; a captura P004C e a confirmação do usuário fornecem esse contexto adicional.

### P004C — ponta contínua e depois botões

Em `pen-events-20260922-174526.csv`, identificado como `P004C_HS611_DRIVER_TIP_STEADY`, a primeira fase da ponta gerou um único contato Raw `0x05/0x03` de `17:45:29.433` a `17:45:57.575` (28,14 s; 6.516 relatórios) e um par `POINTER_DOWN`/`POINTER_UP`. No trecho central de 24 s, todos os 5.558 relatórios da HS611 permaneceram em contato; a pressão bruta teve mediana de `2768` e variou de `1116` a `3205`. Portanto, os ciclos repetidos dos testes anteriores **não são uma oscilação espontânea inevitável** durante um contato estável sem botões.

Após essa fase, o arquivo registra mais 14 contatos Raw, de cerca de `0,10` a `1,12` s cada, além de três acionamentos de `E` e seis cliques direitos injetados pelo driver. O usuário confirmou que **levantou intencionalmente a caneta** após o contato longo e voltou a apoiá-la antes de usar os botões. Portanto, a pausa inicial não é uma falha. Confirmou também que, durante os acionamentos dos dois botões, **manteve a ponta fisicamente encostada na mesa**.

Na fase do botão 1, as três liberações de `E` ocorreram a `5`, `2` e `2` ms das respectivas transições Raw `0x03→0x02`. As seis transições Raw mais próximas dos cliques direitos ocorreram de `37` a `35` ms antes ou de `1` a `153` ms depois do início desses cliques. Em três delas, a pressão bruta imediatamente antes da perda de contato ainda era `1847`–`2502`, em vez de diminuir gradualmente até zero. Logo, a perda de contato durante os acionamentos não é apenas uma recriação de `WM_POINTER`: ela já aparece na coleção Digitizer `0x05` exposta pela HS611 com driver, apesar da ponta apoiada. Os dois contatos curtos anteriores ao primeiro `E` não devem ser atribuídos à ação dos botões.

O usuário relata que esse comportamento da HS611 sempre ocorreu em seu uso prolongado e o considera **normal, não um defeito**. Ele atribui a mudança inicial à própria PW500 ao pressionar o botão: suspender a ação da ponta daria prioridade a `E` ou clique direito e evitaria uma combinação indesejada de tecla com clique/arrasto no aplicativo. O sincronismo e algumas quedas abruptas são compatíveis com essa explicação. O monitor, porém, observa apenas os relatórios após processamento pela mesa e pelo driver, não o sinal EMR emitido pela caneta; por isso não consegue atribuir exclusivamente a ela o desligamento interno da ponta. O resultado observado difere por caminho: na HS611 com driver o Raw `0x05` passa a hover, enquanto na tela WCOM aparecem relatórios `0x28` com pressão zero ou pausas de relatórios. Não há motivo para tratar o comportamento habitual da HS611 como falha a corrigir no escopo atual.

## Estado da P004

Confirmado:

- localização e escala da pressão nos três formatos observados;
- compressão severa e saturação predominante da PW500 na tela antes de `WM_POINTER`; o usuário também observou que a pressão residual diminui quando a força aumenta;
- classificação instável da PW500 na tela, frequentemente como invertida/borracha;
- interrupção e recriação do contato Windows pelos dois botões da PW500 na tela, acompanhada de Raw `0x28`/pressão zero ou de uma pausa sem relatórios WCOM gravados; o contato Raw não permanece continuamente em `0x2C`;
- ponta e botão lateral principal da HS611 sem driver;
- segundo botão da HS611 sem driver gerando contato com pressão zero;
- transformação do protocolo Raw HID pelo driver Huion;
- ausência dos dois botões na coleção Digitizer exposta pelo driver Huion;
- encaminhamento do botão 1 para `E` e do botão 2 para clique direito pelo driver, ambos como entrada injetada;
- estabilidade de um contato de 28,14 s sem botões na HS611 com driver;
- interrupções no próprio relatório Raw `0x05` durante a fase de botões, apesar de a ponta permanecer fisicamente apoiada, conforme confirmação do usuário.

Pontos separados da classificação principal:

- distinguir os bytes restantes de tilt e distância/proximidade, se um recurso futuro precisar deles;
- investigar o mecanismo interno da pausa de contato da HS611 somente se um requisito futuro pedir contato contínuo durante a ação do botão. A classificação dos canais de entrada e o controle de contato estável da P004 estão concluídos.

## Implicação para a P005

Um programa em modo usuário pode observar as interrupções de `WM_POINTER` junto com relatórios Raw `0x28` ou lacunas de captura. Como o WCOM não expõe um código distinto para cada botão, essas observações não são, por si, um identificador confiável da ação desejada. Elas também não demonstram que o programa consiga impedir os eventos originais em outros aplicativos. O curso real da pressão da PW500 diretamente na tela não pode ser reconstruído: a maior parte do sinal já chega comprimida ou saturada. Somente depois de demonstrar a substituição do fluxo original, faria sentido experimentar inverter e reescalar a faixa residual com qualidade limitada, usar pressão simulada ou aceitar pressão quase binária.

Na HS611 com driver, a P005 pode identificar as saídas configuradas dos botões pelo teclado ou mouse injetado. Mapear ambos para teclas pouco usadas e distintas, se o driver permitir, facilitaria distinguir os dois botões de atalhos comuns. A configuração atual já foi suficiente para determinar o caminho de cada botão.
