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
