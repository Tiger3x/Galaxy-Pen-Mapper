# P005 — PW500 screen-button verification

## Goal

Resolve whether the previously observed Raw WCOM gaps during PW500 side-button presses on the Galaxy Book3 360 screen were caused by the monitor's foreground-only capture rule. This test does not measure the pen's EMR signal directly and does not yet implement remapping.

## Two short captures

Use the new `build-ninja\GalaxyPenEventMonitor.exe` (P002.5), with the PW500 directly on the Galaxy display. Keep the monitor visible and do not deliberately switch windows. Leave the Huion driver/button assignments as they are; this test is on the screen, not the HS611.

1. Set session label `P005_SCREEN_PW500_BTN1` and click **Iniciar captura**. Hover in the bordered area for 3 seconds. Press and release button 1 five times while hovering. Pause for 3 seconds. Touch and hold the tip in the bordered area for 3 seconds, then press and release button 1 five times without lifting the tip. Release the tip and click **Parar**.
2. Repeat the same sequence for button 2 with session label `P005_SCREEN_PW500_BTN2`.

Allow about one second between button presses. Avoid moving the pen far from the test area; if it is necessary to lift the tip, note when this happened. The files are saved in `build-ninja\captures`. Send the two new CSVs for analysis.

## Completion criterion

For both files, compare the button phases against `FOREGROUND_SAMPLE`, `RAW_HID`, `POINTER_*` and `WINDOW_*` rows. A Raw gap with foreground samples remaining `1` is not explained by the old focus gate, but it alone does not prove exactly what signal the pen transmitted. If reports continue, compare their payloads across buttons. Record whether the two buttons are distinguishable and classify each gap as a focus artifact, a digitizer-report interruption, or inconclusive. Only then proceed to the broader P005 user-mode feasibility decision.
