# Galaxy Pen Mapper — Development Plan

## Goal

Understand the Galaxy Book3 360 pen input path, then improve the Huion PW500 where the captured signal and Windows input APIs allow it. Direct use on the Galaxy display remains the primary target; use through the HS611 is a separate, better-supported path.

## Evidence and feasibility

| Route | What the captures establish | Practical limit |
| --- | --- | --- |
| PW500 on Galaxy display | WCOM reports pen position, but pressure starts near full scale, falls only under excessive force, and is usually saturated. During side-button actions with the tip held down, Windows contact restarts while WCOM reports either switch to `0x28`/zero pressure or pause. | The missing pressure range and distinct button identities are absent from the observed reports. A button-signal substitution proposed by the user is consistent with the captures, but the EMR signal is not measured directly. Software can offer only an explicitly degraded pressure mode unless new input evidence appears. |
| PW500 through HS611 with Huion driver | Pressure varies normally. Configured button 1 arrives as injected `E`; configured button 2 arrives as injected right click. A steady tip is stable without buttons; during button use, the exposed Raw Digitizer contact can pause while the physical tip stays down. The user reports this as longstanding, normal HS611 behavior. | The Digitizer report does not identify the side buttons. Remapping their keyboard/mouse output alone will not preserve uninterrupted tip contact; this is not a defect to fix unless a requested workflow needs different behavior. |
| PW500 through HS611 without Huion driver | Raw button states are visible, but the second button can be interpreted as contact with zero pressure. | A user-mode observer cannot assume it can suppress the original pen interpretation system-wide. |

Windows exposes [Raw Input registration](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawinputdevice) for observation in an application and [synthetic pointer input](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createsyntheticpointerdevice) for generating pen events. Neither API, by itself, establishes system-wide replacement of the original digitizer stream. Microsoft's [Virtual HID Framework](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/virtual-hid-framework--vhf-) is a driver route, not a user-mode shortcut.

## Phases

### P001 — HID Scanner ✅
Enumerate Windows HID interfaces and identify Digitizer-class devices, VID/PID, usages and report sizes.

**Exit criterion:** scanner builds successfully and can identify Galaxy digitizer-related HID interfaces from a real-machine capture.

### P002 — Pen Event Monitor ✅
Capture Digitizer Raw Input reports plus Windows-interpreted pen events (`WM_POINTER` / `GetPointerPenInfo`) with timestamps.

**Exit criterion:** Windows build and real-hardware CSV captures validated on the Galaxy Book. The P002.5 monitor records keyboard/mouse channels and Windows input-source metadata, and separates Digitizer Raw HID observation from foreground state.

### P003 — Comparative Capture ✅
Record controlled, labeled sequences with:
1. Samsung S Pen;
2. Huion PW500 directly on the Galaxy display;
3. PW500 through the Huion HS611.

**Exit criterion:** achieved with four real-hardware captures: S Pen on screen, PW500 on screen, and PW500 through HS611 with and without the Huion driver.

### P004 — Input Classification ✅
Map report bits/usages to tip, barrel buttons, pressure, proximity and other states. Determine where the double-click/disable behavior originates.

**Result:** the main pen and button paths are identified. The screen pressure response is reversed in its small usable range and predominantly saturated. During screen-button use, WCOM contact is not continuously preserved: the recorded reports can change to hover/zero pressure or stop temporarily. On the HS611, the driver emits configured button actions as injected keyboard/mouse input. The control capture held one Raw contact for 28.14 seconds without buttons. The user intentionally lifted the tip after that baseline, then kept it physically down during button use; the later button phase nevertheless contains Raw contact releases. For button 1, three Raw releases occurred within 5, 2 and 2 ms of the injected `E` releases. See [P004 classification](P004-INPUT-CLASSIFICATION.md).

**Exit criterion:** input channels, steady-tip baseline and physical-tip state during button use are established. The user confirms that tip interruption during HS611 button actions has long been normal in their workflow; do not classify it as a new fault. The exact internal mechanism need not be isolated for the primary Galaxy display goal. Tilt and distance mapping remain optional unless a later feature needs them.

### P005 — User-mode Feasibility Gate
Keep direct use on the Galaxy display as the first question: verify whether a user-mode program can distinguish the PW500 from the S Pen reliably and prevent or replace the original erroneous pen events in a target application. Raw Input observation alone is not suppression. Do not promise pressure correction or button remapping in other applications until this gate passes. The current WCOM reports do not distinguish the two side buttons.

**Immediate verification milestone:** run the two [P005 screen-button captures](P005-SCREEN-BUTTON-TEST.md) with P002.5. For each button, determine whether apparent WCOM gaps occur with `window_foreground=1` throughout, whether Raw reports continue with a changed payload, and whether any observed report distinguishes button 1 from button 2. Classify gaps as capture-focus artifact, digitizer-report interruption, or inconclusive; do not infer an EMR-level mechanism from a missing Raw report. This milestone is complete only after both CSVs are reviewed and the findings are recorded. Then decide whether a user-mode identification/suppression experiment is justified; if not, document the limit before investing in remapping.

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
