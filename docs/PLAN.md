# Galaxy Pen Mapper — Development Plan

## Goal

Understand the Galaxy Book3 360 pen input path, then improve the Huion PW500 where the captured signal and Windows input APIs allow it. Direct use on the Galaxy display remains the primary target; use through the HS611 is a separate, better-supported path.

## Evidence and feasibility

| Route | What the captures establish | Practical limit |
| --- | --- | --- |
| PW500 on Galaxy display | WCOM reports pen position, but pressure starts near full scale, falls only under excessive force, and is usually saturated. Both side buttons can restart Windows contact without an equivalent Raw HID contact change. | The missing pressure range and distinct button identities are absent from the observed reports. Software can offer only an explicitly degraded pressure mode unless new input evidence appears. |
| PW500 through HS611 with Huion driver | Pressure varies normally. Configured button 1 arrives as injected `E`; configured button 2 arrives as injected right click. | The Digitizer report does not identify the side buttons. A remapper must distinguish their configured output from ordinary keyboard/mouse use and avoid duplicate actions. |
| PW500 through HS611 without Huion driver | Raw button states are visible, but the second button can be interpreted as contact with zero pressure. | A user-mode observer cannot assume it can suppress the original pen interpretation system-wide. |

Windows exposes [Raw Input registration](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawinputdevice) for observation in an application and [synthetic pointer input](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createsyntheticpointerdevice) for generating pen events. Neither API, by itself, establishes system-wide replacement of the original digitizer stream. Microsoft's [Virtual HID Framework](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/virtual-hid-framework--vhf-) is a driver route, not a user-mode shortcut.

## Phases

### P001 — HID Scanner ✅
Enumerate Windows HID interfaces and identify Digitizer-class devices, VID/PID, usages and report sizes.

**Exit criterion:** scanner builds successfully and can identify Galaxy digitizer-related HID interfaces from a real-machine capture.

### P002 — Pen Event Monitor ✅
Capture Digitizer Raw Input reports plus Windows-interpreted pen events (`WM_POINTER` / `GetPointerPenInfo`) with timestamps.

**Exit criterion:** Windows build and real-hardware CSV captures validated on the Galaxy Book. The P002.4 monitor also records keyboard/mouse channels and Windows input-source metadata.

### P003 — Comparative Capture ✅
Record controlled, labeled sequences with:
1. Samsung S Pen;
2. Huion PW500 directly on the Galaxy display;
3. PW500 through the Huion HS611.

**Exit criterion:** achieved with four real-hardware captures: S Pen on screen, PW500 on screen, and PW500 through HS611 with and without the Huion driver.

### P004 — Input Classification 🟡
Map report bits/usages to tip, barrel buttons, pressure, proximity and other states. Determine where the double-click/disable behavior originates.

**Current result:** the main pen and button paths are identified. The screen pressure response is reversed in its small usable range and predominantly saturated. On the HS611, the driver emits the configured button actions as injected keyboard/mouse input. The two tip + button captures contain 42 Raw HID contact cycles each. A new control capture held a single HS611 contact for 28.14 seconds without any Raw contact restart, then recorded shorter contacts near button events. See [P004 classification](P004-INPUT-CLASSIFICATION.md).

**Exit criterion:** identify the input channels and establish the steady-tip baseline. Both are complete. Confirm whether the pen tip physically remained down during the later button phase before assigning those short contacts to the driver. Tilt and distance mapping remain optional unless a later feature needs them.

### P005 — User-mode Feasibility Gate
Keep direct use on the Galaxy display as the first question: verify whether a user-mode program can distinguish the PW500 from the S Pen reliably and prevent or replace the original erroneous pen events in a target application. Raw Input observation alone is not suppression. Do not promise pressure correction or button remapping in other applications until this gate passes. The current WCOM reports do not distinguish the two side buttons.

The HS611 with driver is an independent, opt-in remapper branch. The Huion driver already assigns each button; build additional remapping only if the user wants behavior the driver cannot supply. In that case, use uniquely identifiable assignments where available, preserve physical keyboard/mouse use, prevent duplicate actions, and provide an immediate off switch.

**Exit criterion:** demonstrate the chosen user-mode mechanism in the target application without unintended keys, clicks or duplicate pen actions. If the screen's original pen stream cannot be isolated, document that limit and decide with the user whether a driver-level investigation is worthwhile. Do not substitute the easier HS611 result for direct-display success.

### P006 — Synthetic Pen Feasibility (conditional)
If P005 finds a credible way to exclude the original stream, test Windows user-mode synthetic pen input in a controlled application. Injecting a second pen stream alone creates duplicates. A degraded-pressure experiment may invert and expand only the narrow varying range or simulate pressure; it cannot recover force information from saturated samples.

### P007 — Driver Path (only if necessary)
Evaluate a signed virtual HID source or input filter only for a confirmed need that user-mode input cannot meet, such as replacing or suppressing the original pen stream. A driver can change routing and interpretation, but cannot reconstruct pressure values that the digitizer never reports. Installing or replacing the Galaxy digitizer driver is outside the early prototype.

### P008 — Profiles and UI
After the input mechanism passes its on-device test, add per-application profiles (Blender, Photoshop, Windows), tray control, autostart and diagnostic export. Do not build profiles around an unverified remapping mechanism.

## Safety rule

Do not install or replace the Galaxy Book digitizer driver during early research. P001–P006 must remain reversible and non-destructive. Keep the diagnostic monitor observation-only.
