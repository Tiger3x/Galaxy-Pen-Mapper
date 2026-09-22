# P004 — Input Classification

## Capturas analisadas

As quatro sessões foram gravadas pelo Galaxy Pen Diagnostic Studio P002.3:

| Sessão | Caminho | Dispositivo Raw principal | Report ID |
| --- | --- | --- | --- |
| `S_Pen` | S Pen diretamente na tela | `WCOM016C`, usage `0x0D/0x02` | `0x02` |
| `Huion500` | PW500 diretamente na tela | `WCOM016C`, usage `0x0D/0x02` | `0x02` |
| `HUION500_MESA SEM DRIVER` | PW500 na HS611 sem driver Huion | `VID_256C&PID_006D`, usage `0x0D/0x02` | `0x0A` |
| `HUION500_MESA COM DRIVER` | PW500 na HS611 com driver Huion | `tablethid`, usage `0x0D/0x01` | `0x05` |

## Resultado principal: pressão da PW500 na tela

A pressão fixa em `1024` não é introduzida pelo monitor. O relatório Raw HID do digitizador WCOM já chega saturado:

```text
PW500 hover:   02 28 ... 00 00 ...
PW500 contato: 02 2C ... FF 0F ...
```

Os bytes 6–7 do relatório `0x02`, em little-endian, contêm a pressão bruta. Durante todo contato da PW500 eles valem `0x0FFF` (`4095`). O Windows escala `4095 / 4` para `1024` e não recebe valores intermediários.

Na S Pen, os mesmos bytes variam. Um exemplo medido foi pressão bruta `0x02BB` (`699`) convertida em pressão Windows `174`. A mediana da razão Raw/Windows foi `4,005`, confirmando a escala aproximada de 12 para 10 bits.

Conclusão: a PW500 é detectada por ressonância na tela, mas o digitizador/firmware não decodifica corretamente sua pressão. Uma correção apenas sobre `WM_POINTER` não consegue recuperar pressão que não existe no relatório de entrada.

## Classificação dos relatórios

### WCOM `0x02` — tela do Galaxy Book3 360

| Campo | S Pen | PW500 na tela | Interpretação |
| --- | --- | --- | --- |
| Byte 1 em hover | `0x20` | `0x28` | ferramenta/modo diferente |
| Byte 1 em contato | `0x21` | `0x2C` | ponta normal versus borracha/invertida |
| Bytes 6–7 | variável | sempre `FF 0F` | pressão little-endian de 12 bits |
| Windows `penFlags` | `0` | `2` em hover, `6` em contato | PW500 interpretada como `INVERTED`; contato também como `ERASER` |

O padrão `0x28 → 0x2C` coincide com a interpretação incorreta do Windows. A PW500 não segue o mesmo padrão `0x20 → 0x21` da S Pen.

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

Os bytes 6–7 mantêm pressão variável, com razão Raw/Windows mediana `8,009`. Nesta captura, `penFlags` permaneceu zero e os botões não apareceram nesta coleção Digitizer. O driver altera a identidade, usage e formato do dispositivo e pode encaminhar botões por outra coleção ou por eventos sintetizados.

## Estado da P004

Confirmado:

- localização e escala da pressão nos três formatos observados;
- saturação da PW500 na tela antes de `WM_POINTER`;
- PW500 na tela classificada como invertida/borracha;
- ponta e botão lateral principal da HS611 sem driver;
- segundo botão da HS611 sem driver gerando contato com pressão zero;
- transformação do protocolo Raw HID pelo driver Huion.

Ainda precisa de captura direcionada:

- pressionar cada botão isoladamente, sem tocar a ponta, nas quatro rotas;
- repetir com a ponta em contato e com pressão estável;
- verificar se o driver Huion envia os botões por mouse, teclado ou outra coleção HID;
- distinguir quais bytes restantes representam tilt e distância/proximidade.

## Implicação para a P005

Um remapeador em modo usuário pode corrigir os cliques de botão quando os estados Raw HID estiverem disponíveis. Ele não pode reconstruir a pressão real da PW500 diretamente na tela, pois o valor bruto já chega fixo em `4095`. Para esse caminho, só é possível usar pressão simulada, uma curva baseada em outra variável ou aceitar pressão binária.
