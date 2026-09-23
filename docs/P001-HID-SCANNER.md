# P001 — HID Scanner

## Purpose

Enumerate HID interfaces visible to Windows and locate the Galaxy Book3 360 digitizer-related interfaces.

## What the scanner records

- Windows device description;
- HID device path;
- VID / PID / version;
- manufacturer, product and serial strings when available;
- HID Usage Page and Usage;
- input/output/feature report lengths;
- input value/button capability counts.

With `--wcom-pen-caps`, the scanner restricts output to the Galaxy WCOM `COL01` and `COL04` interfaces and prints their input value/button capabilities, including report IDs, usages, logical ranges and bit widths. This mode only reads HID descriptors; it does not change a device.

Usage Page `0x0D` is highlighted because it is the HID Digitizers page.

## Important limitation

The Samsung S Pen and Huion PW500 are passive EMR pens. They are not expected to appear as two independent USB devices when used on the Galaxy display. The display digitizer is the HID device; differences between pens, if preserved, should appear in its live reports. That is the job of P002.

## First test

Build Release and run:

```powershell
.\build-ninja\GalaxyPenHidScanner.exe | Tee-Object -FilePath hid-scan.txt
```

Send/save the complete output, especially every entry marked:

```text
>>> CANDIDATO DIGITIZER (Usage Page 0x0D) <<<
```

No driver changes are made by this program.

For the focused, read-only WCOM collection inventory used by P007:

```powershell
.\build-ninja\GalaxyPenHidScanner.exe --wcom-pen-caps
```

For an **offline, in-memory** check of the `COL04` pen-report layout:

```powershell
.\build-ninja\GalaxyPenHidScanner.exe --wcom-pen-report-lab
```

The lab reads the live `COL04` HID descriptor, but creates and edits report `0x1A` only in process memory. It never writes a report to the device. On the tested Galaxy Book3 360, the descriptor reports a 15-byte input report and pressure usage `0x0D:0x30` with logical range 0–4095. The lab round-tripped 0, 699 and 4095; pressure changed bytes 6–7 while the synthetic tip-contact and X/Y fields stayed intact. This confirms the **advertised format**, not the bytes of a live `COL04` report or the feasibility of intercepting one. The earlier Raw Input captures came from the separate `COL01` helper collection.
