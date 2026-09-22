# Galaxy Pen Mapper — Development Plan

## Goal

Understand the Galaxy Book3 360 pen input path first, then remap the Huion PW500 safely with the smallest required intervention layer.

## Phases

### P001 — HID Scanner
Enumerate Windows HID interfaces and identify Digitizer-class devices, VID/PID, usages and report sizes.

**Exit criterion:** we can identify the Galaxy digitizer-related HID interfaces from a scanner output.

### P002 — Pen Event Monitor
Create a Win32 Raw Input monitor for live HID reports and decoded pen state where Windows exposes it.

**Exit criterion:** capture timestamped reports for hover, tip, pressure and barrel-button actions.

### P003 — Comparative Capture
Record controlled sequences with:
1. Samsung S Pen;
2. Huion PW500 directly on the Galaxy display;
3. PW500 through the Huion HS611.

**Exit criterion:** produce comparable logs showing which fields differ.

### P004 — Input Classification
Map report bits/usages to tip, barrel buttons, pressure, proximity and other states. Determine where the double-click/disable behavior originates.

### P005 — User-mode Remapper
Prototype configurable remapping without a kernel driver whenever Windows exposes enough information.

### P006 — Virtual HID
If required, emit a clean virtual pen/mouse device while suppressing or avoiding duplicate interpreted events.

### P007 — Filter Driver (only if necessary)
Use a signed Windows driver/filter only if the bad interpretation occurs below user mode and cannot be corrected safely above it.

### P008 — Profiles and UI
Per-application profiles (Blender, Photoshop, Windows), tray app, autostart and diagnostic export.

## Safety rule

Do not install or replace the Galaxy Book digitizer driver during early research. P001–P005 must remain reversible and non-destructive.
