# Galaxy Pen Mapper — Development Plan

## Goal

Understand the Galaxy Book3 360 pen input path first, then remap the Huion PW500 safely with the smallest required intervention layer.

## Phases

### P001 — HID Scanner ✅
Enumerate Windows HID interfaces and identify Digitizer-class devices, VID/PID, usages and report sizes.

**Exit criterion:** scanner builds successfully and can identify Galaxy digitizer-related HID interfaces from a real-machine capture.

### P002 — Pen Event Monitor ✅
Capture Digitizer Raw Input reports plus Windows-interpreted pen events (`WM_POINTER` / `GetPointerPenInfo`) with timestamps.

**Exit criterion:** Windows build validated. Real-hardware CSV capture on the Galaxy Book is the next validation step before P003.

### P003 — Comparative Capture ✅
Record controlled, labeled sequences with:
1. Samsung S Pen;
2. Huion PW500 directly on the Galaxy display;
3. PW500 through the Huion HS611.

**Exit criterion:** achieved with four real-hardware captures: S Pen on screen, PW500 on screen, and PW500 through HS611 with and without the Huion driver.

### P004 — Input Classification 🟡
Map report bits/usages to tip, barrel buttons, pressure, proximity and other states. Determine where the double-click/disable behavior originates.

**Current result:** the PW500 screen path compresses pressure into the top of the scale and is predominantly saturated; the user observed that its residual pressure value falls under heavy force. Both pen buttons recreate the Windows contact while Raw HID remains in contact. Without the Huion driver, HS611 state `0xC4` produces contact/click with zero pressure. With the driver, the buttons leave the Digitizer collection: button 1 configured as `E` produces injected keyboard input, and button 2 configured as right click produces injected mouse input. The unexpected contact cycles in both tip + button tests still need to be isolated before closing P004. See [P004 classification](P004-INPUT-CLASSIFICATION.md).

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
