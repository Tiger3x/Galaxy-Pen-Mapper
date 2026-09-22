# P005A — Pen-tip pattern probe

## Purpose

Check whether the Galaxy display's WCOM reports can distinguish a normal S Pen tip from the Huion PW500 tip well enough to justify the next user-mode experiment. The side buttons are deliberately out of scope. This observer does not suppress the original pen stream, inject a replacement, or repair pressure.

## Why a single report is insufficient

The labeled S Pen capture `pen-events-20260922-133214.csv` contains 10,861 WCOM reports. Most use mode `0x20/0x21`, but 459 use `0x28/0x2C` in short bursts (the longest observed burst lasted 264 ms). The PW500 reference uses `0x28/0x2C` throughout. The monitor therefore displays a **pattern**, not a guaranteed physical pen identity.

P002.6 recognizes the Galaxy WCOM collection (`WCOM016C`, VID 11551, PID 337, usage `0x0D/0x02`) and applies these conservative rules:

- S Pen pattern: at least 20 reports in mode `0x20/0x21` during one active observation period. Short subsequent `0x28/0x2C` bursts do not change that label.
- PW500 candidate: at least one second of `0x28/0x2C`, at least 20 contact reports, and at least half of those contact reports with raw pressure of 4000 or more. Any `0x20/0x21` in the same period prevents a PW500 label.
- Unknown/mixed: hover-only, insufficient contact, prolonged mixed modes or no WCOM report for 1.5 seconds. A contradictory mode immediately clears a prior PW500 candidate.

These are empirical criteria for a controlled test, not proof that every possible S Pen action is excluded. In replay of existing captures, the S Pen session produced no PW500-candidate rows; the initial PW500 session produced no S-Pen-pattern rows. A PW500 hover-only capture correctly remained inconclusive. Prospective hardware tests are still required.

## Two tip-only captures

Run `build-ninja\GalaxyPenEventMonitor.exe` P002.6. Use the same bordered screen area for both pens, without pressing side buttons or using the HS611. Keep the monitor in the foreground.

1. Label the first session `P005A_SPEN_TIP`. With the S Pen, hover for about 2 seconds, then make several light-to-firm strokes for at least 10 seconds. Observe the **PADRÃO OBSERVADO** and **PRESSÃO WIN / RAW** fields. Stop the capture.
2. Label the second session `P005A_PW500_TIP`. Repeat with the PW500 directly on the Galaxy screen. Use ordinary light pressure first; there is no need to force the nib to chase a lower number. Stop the capture.

Save both CSVs from `build-ninja\captures`. Success for this stage means the S Pen never receives a PW500-candidate label, the PW500 reaches a candidate label during normal tip contact, and neither classification depends on a side-button action. If the result fails or stays inconclusive, stop before attempting input replacement and inspect the conflicting Raw rows.

## Gate after this test

Even a correct pattern label only observes input. It does not prevent the original erroneous contact or pressure from reaching another application. The next independent P005 gate is a controlled test of original-stream isolation/no duplicates. Synthetic pen injection or pressure transformation stays disabled until that gate passes.
