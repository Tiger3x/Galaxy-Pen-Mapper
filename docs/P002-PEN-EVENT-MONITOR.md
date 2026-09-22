# P002 — Pen Event Monitor

## Objetivo

Capturar simultaneamente duas visões da caneta:

1. **RAW_HID** — bytes que chegam pelas coleções HID Digitizer (Usage Page `0x0D`) quando o Windows permite Raw Input para aquela coleção;
2. **WINDOWS_POINTER** — estado já interpretado pelo Windows através de `WM_POINTER` + `GetPointerPenInfo`.

Essa comparação é essencial: se a PW500 já chega com bits diferentes no HID, o remapeamento pode atuar na tradução; se o HID parecer normal e o erro surgir somente em `WM_POINTER`, o problema está em uma camada posterior da pilha de entrada.

## Executável

`GalaxyPenEventMonitor.exe`

Ao iniciar, ele cria um CSV no diretório atual:

`pen-events-AAAAMMDD-HHMMSS.csv`

## Campos principais

- timestamp;
- tipo do evento;
- device path;
- VID/PID;
- Usage Page / Usage;
- índice do relatório HID;
- bytes brutos em hexadecimal;
- pointer ID;
- pointer flags;
- pen flags;
- pressure;
- tilt X/Y;
- rotation;
- X/Y.

## Teste rápido P002

Faça dentro da janela do monitor:

1. hover sem tocar por alguns segundos;
2. 5 toques simples;
3. um traço começando com pouca pressão e aumentando;
4. botão inferior em hover e depois durante contato;
5. botão superior em hover e depois durante contato;
6. alguns traços normais.

Faça primeiro com a **S Pen** e depois com a **Huion PW500**.

Para a análise comparativa formal, a P003 irá separar as capturas em sessões rotuladas.

## Segurança

P002 é somente observação. Não instala driver, não injeta eventos e não modifica configurações HID.
