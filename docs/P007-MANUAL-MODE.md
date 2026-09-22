# P007 — Manual PW500 mode

## User choice

The user wants a notification-area icon to turn the correction on when using the PW500 directly on the Galaxy screen and off when using the S Pen normally. Automatic classification is no longer required for activation. The P002.6 pen-pattern indicator remains a diagnostic observation only.

"On/off" means changing the **filter's behavior**, not repeatedly installing, disabling or unloading the Windows pen device/driver. The tray application would request a mode change and display the state confirmed by the driver. A tray icon can be implemented with Windows [`Shell_NotifyIcon`](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shell_notifyiconw); a framework driver can expose a separate [device interface](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/using-device-interfaces) for control requests. Those APIs establish a possible control channel, **not** that a safe pen-report filter has already been proven.

## Intended behavior

| State | Pen-report behavior | Tray display |
| --- | --- | --- |
| Off (default) | Pass every report through unchanged, including S Pen and PW500. | Clearly off. |
| On | Apply the explicitly chosen, limited PW500 transform to original pen reports; do not create a second pen stream. | Clearly on only after driver acknowledgement. The S Pen may also be changed in this mode. |
| Driver absent, control link lost, or internal error | Revert to pass-through. A control-app exit/crash must not leave correction stuck on; require a bounded lease or equivalent fail-safe. | Error/off, never falsely on. |

The initial filter experiment should be **pass-through only** in both switch positions. It must establish that the selected device remains functional, the S Pen behaves normally while off, and input is not duplicated before pressure bytes are ever modified. Only then can a pressure transform be enabled behind the same switch. The current PW500 raw pressure is mostly saturated and falls as force increases in its narrow varying region, so any remapping is degraded and cannot reconstruct real force.

Manual activation removes the need for a one-second pen-recognition delay on the first stroke. It does **not** solve original-input isolation, pressure saturation, driver signing or installation risk. A filter on the `COL04` pen collection is a research candidate; the captured `COL01` helper reports must not be assumed to be the bytes received by that filter.

## Safety and validation gates

1. Establish a narrowly scoped filter position and a tested recovery procedure in an isolated environment before binding to the live Galaxy digitizer. Never attach a broad HID-class filter by default.
2. Off must pass input through byte-for-byte. A restart, app crash or failure to renew the on-state must return to off.
3. On must transform the **existing** pen report rather than injecting a duplicate. If an in-place transform is not feasible, stop and redesign before any virtual device is used.
4. The icon must show driver-confirmed state, offer a single obvious off action and avoid silently enabling at login. Switching on while a stroke is in progress should take effect only at a defined safe boundary; off must remain immediately available.
5. Validate on real hardware only after the development package, signing, rollback and recovery plan are ready and the user explicitly approves deployment. No driver is installed or changed by writing this specification.
