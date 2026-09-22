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
Keep direct use on the Galaxy display as the first question. At the user's request, prioritize the tip and pressure; side-button remapping is not part of the current success criterion. The user chose a manual tray toggle for PW500 mode, so automatic S Pen/PW500 recognition is no longer a requirement for the correction path. Verify whether the original erroneous pen events can be replaced rather than supplemented. Raw Input observation alone is not suppression. Do not promise pressure correction in other applications until this gate passes.

**Completed focus check:** the combined P002.5 screen capture `pen-events-20260922-184037.csv` contains 412 foreground samples, all `1`, and 5,817 Raw HID rows, all received as foreground input. The WCOM stream itself reports `0x00` releases and `0x28` hover intervals during pointer-contact interruptions. A focus-only recording artifact is not the explanation. The user tested button 1 before button 2, but the exact transition is inferred rather than logged. Further button-specific work is parked.

**P005A retrospective gate:** the existing S Pen and PW500 tip captures are sufficient to move on without requesting another pair of captures. The [conservative pen-pattern probe](P005A-PEN-SIGNATURE.md) separates those labeled sessions in replay, but is now diagnostic only: the manual toggle, not this heuristic, chooses whether correction is active.

**P005B user-mode gate:** [input-isolation review](P005B-INPUT-ISOLATION.md) found no documented, reliable device-specific way for this observer to stop the original PW500 pen stream from reaching unrelated Windows applications. `RIDEV_NOLEGACY` is limited to mouse and keyboard; a synthetic pen adds input rather than replacing the original. A program controlling its own canvas can choose how to handle its `WM_POINTER` messages, but that does not isolate Blender, Photoshop or the desktop. Cross-process hooks have coverage and stability limits and are not a suitable default replacement mechanism. Therefore P006 remains disabled for the general desktop goal; do not inject a second pen stream merely to demonstrate it is possible.

**Next research gate:** P007 architecture review, without installing or changing a driver. The machine has an I²C HID parent `ACPI\WCOM016C\1`, a `COL01` helper collection that supplies the captured Raw report `0x02`, and a separate Windows pen collection `COL04` whose descriptor advertises report `0x1A`. Determine whether a narrowly scoped filter can modify the original `COL04` report in place, with no second pen stream, and leave it unchanged when manually disabled. Automatic pen identification is not required. Establish a safe development/test environment before any deployment decision. The saturated Raw samples still do not contain the lost force information. See [manual PW500 mode](P007-MANUAL-MODE.md).

The HS611 with driver is an independent, opt-in remapper branch. The Huion driver already assigns each button; build additional remapping only if the user wants behavior the driver cannot supply. In that case, use uniquely identifiable assignments where available, preserve physical keyboard/mouse use, prevent duplicate actions, and provide an immediate off switch.

**Exit criterion:** the general-desktop user-mode path is closed unless a specific supported isolation mechanism is found. A target-application integration remains a separate optional branch, not proof of system-wide replacement. Do not substitute the easier HS611 result for direct-display success.

### P006 — Synthetic Pen Feasibility (conditional)
Paused for the general-desktop goal. Resume only if P005 finds a credible way to exclude the original stream. Injecting a second pen stream alone creates duplicates. A degraded-pressure experiment may invert and expand only the narrow varying range or simulate pressure; it cannot recover force information from saturated samples.

### P007 — Driver Path (only if necessary)
The P005B review established that general-desktop replacement is such a need. Evaluate a narrowly scoped input filter first; a virtual HID source is a fallback only if original input can also be excluded. The I²C HID parent exposes several collections, so the location and pass-through behavior of a filter must be validated before touching the live device. Per the user's choice, do not block on automatic pen classification: a tray-controlled PW500 mode will be off by default. Its intended users may not have an S Pen, so S Pen behavior while the mode is on is not a product criterion. Off must still leave the original input unchanged. A driver can change routing and interpretation, but cannot reconstruct pressure values that the digitizer never reports. Installing or replacing the Galaxy digitizer driver remains outside the current research step. [P007 mode and safety contract](P007-MANUAL-MODE.md).

### P008 — Profiles and UI
The manual tray on/off control is part of P007's safety requirement, not a late optional profile feature. After the input mechanism passes its on-device test, add optional per-application profiles (Blender, Photoshop, Windows), autostart and diagnostic export. Do not build profiles around an unverified remapping mechanism.

## Safety rule

Do not install or replace the Galaxy Book digitizer driver during early research. P001–P006 must remain reversible and non-destructive. Keep the diagnostic monitor observation-only.
