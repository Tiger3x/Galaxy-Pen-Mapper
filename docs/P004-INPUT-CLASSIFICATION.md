# P004 — Input Classification

## Capturas analisadas

As quatro sessões comparativas iniciais e 14 sessões direcionadas foram gravadas pelo Galaxy Pen Diagnostic Studio:

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

O teste direcionado de pressão revelou uma nuance: houve alguma variação, mas apenas na faixa superior. Foram 10.747 amostras em contato, com pressão Windows entre `800` e `1024`; a mediana foi `1024` e 7.444 amostras (`69,3%`) ficaram totalmente saturadas. A pressão bruta variou de `3201` a `4095`. Portanto, a tela comprime quase todo o curso da PW500 nos 22% superiores da escala e satura a maior parte das amostras. Isso continua inadequado para desenho com pressão progressiva.

Na S Pen, os mesmos bytes variam. Um exemplo medido foi pressão bruta `0x02BB` (`699`) convertida em pressão Windows `174`. A mediana da razão Raw/Windows foi `4,005`, confirmando a escala aproximada de 12 para 10 bits.

Conclusão: a PW500 é detectada por ressonância na tela, mas o digitizador/firmware não decodifica corretamente seu curso de pressão. Uma correção sobre `WM_POINTER` pode reescalar a pequena faixa restante, mas não consegue reconstruir a maior parte do curso perdida pela compressão e saturação do relatório de entrada.

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

Com a ponta apoiada, as duas repetições produziram o resultado decisivo. Em cada botão ocorreram seis pares adicionais `POINTER_UP`/`POINTER_DOWN` enquanto os relatórios WCOM imediatamente anteriores e posteriores continuavam em `0x2C`, com pressão bruta diferente de zero e normalmente saturada em `4095`. Ou seja, o Windows encerra e recria o contato lógico sem a ponta realmente sair do contato bruto.

Os dois botões apresentaram o mesmo comportamento no caminho WCOM e não puderam ser diferenciados por um bit Raw HID. A recriação do contato explica cliques extras, duplo clique e interrupções percebidas como “desativar” a caneta.

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

Nos testes isolados de botão em hover, o relatório permaneceu `0x05/0x02`, `penFlags` permaneceu zero e não houve `POINTER_DOWN`. Nos testes com ponta + botão, o relatório permaneceu limitado a `0x02`/`0x03`; todas as transições de contato Windows coincidiram com transições Raw hover/contato. Portanto, os botões não estão expostos pela coleção Digitizer `0x05`. O driver deve encaminhá-los por mouse, teclado, comando de aplicativo ou outro dispositivo virtual.

## Estado da P004

Confirmado:

- localização e escala da pressão nos três formatos observados;
- compressão severa e saturação predominante da PW500 na tela antes de `WM_POINTER`;
- classificação instável da PW500 na tela, frequentemente como invertida/borracha;
- interrupção e recriação do contato Windows pelos dois botões da PW500 na tela, mesmo com contato Raw contínuo;
- ponta e botão lateral principal da HS611 sem driver;
- segundo botão da HS611 sem driver gerando contato com pressão zero;
- transformação do protocolo Raw HID pelo driver Huion;
- ausência dos dois botões na coleção Digitizer exposta pelo driver Huion.

Ainda precisa de captura direcionada:

- verificar se o driver Huion envia os botões por mouse, teclado, comando de aplicativo ou dispositivo virtual;
- distinguir quais bytes restantes representam tilt e distância/proximidade.

## Implicação para a P005

Um remapeador em modo usuário pode detectar e amortecer as recriações indevidas de contato quando correlacionar `WM_POINTER` com o Raw HID contínuo. Ele não pode reconstruir o curso real da PW500 diretamente na tela: a maior parte do sinal já chega comprimida ou saturada. Para esse caminho, as opções são reescalar a faixa residual com qualidade limitada, usar pressão simulada ou aceitar pressão quase binária.

Para a HS611 com driver, o próximo monitor registra Raw Mouse, Raw Keyboard, mensagens de mouse/teclado e a origem declarada pelo Windows. Essa captura determinará qual canal deve alimentar o remapeador P005.
