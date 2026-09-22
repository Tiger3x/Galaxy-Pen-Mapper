# P002 / P002.1 — Pen Event Monitor

## P002.1: Window-only capture

The event monitor is now a native Windows GUI executable with no console window.

It records only while all of these conditions are true:

1. the user pressed **Iniciar captura**;
2. the Galaxy Pen Event Monitor window is the foreground window;
3. the pen is inside the bordered **ÁREA DE TESTE DA CANETA**.

Events outside the capture area are ignored. Raw Input no longer uses `RIDEV_INPUTSINK`, so the application does not intentionally request background HID input.

## What is observed

Two input layers are kept for diagnosis:

- **RAW_HID** — Digitizer reports from HID Usage Page `0x0D`, when exposed by Windows Raw Input;
- **WINDOWS_POINTER** — Windows-interpreted pen state via `WM_POINTER` and `GetPointerPenInfo`.

The live panel shows:

- pointer flags;
- pen flags such as BARREL / INVERTED / ERASER;
- pressure;
- tilt;
- event count;
- a preview of the most recent raw HID report.

## Controls

- **Iniciar captura** — creates a new timestamped CSV and starts recording;
- **Parar** — closes the CSV cleanly;
- **Limpar painel** — clears the live values and counter.

CSV format remains suitable for P003 comparative analysis.

## Safety

P002.1 is observation-only. It does not install a driver, replace the Samsung digitizer driver, inject input, block input, or remap buttons.
