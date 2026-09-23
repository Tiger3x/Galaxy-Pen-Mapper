# P007 — Pen-stack gate before any on-device filter test

## Read-only findings on the Galaxy Book3 360

| Layer | Observed device | Current driver/stack | Implication |
| --- | --- | --- | --- |
| WCOM transport parent | `ACPI\WCOM016C\1` | `hidi2c.inf`; `mshidkmdf`, `hidi2c`, ACPI | A filter here could affect multiple WCOM collections, not just the pen. It is not the first attachment candidate. |
| Captured helper collection | `HID\WCOM016C&Col01\...` | Separate Raw Input source with report `0x02` | Its pressure samples are diagnostic evidence, not proof of `COL04` bytes. |
| Windows pen collection | `HID\WCOM016C&Col04\5&a6b5543&0&0003` | `input.inf`, matching `HID_DEVICE_UP:000D_U:0002`; stack reports `mshidkmdf` | Candidate for a **device-specific upper filter**, not a HIDClass-wide filter. Its descriptor advertises 15-byte input report `0x1A`. |

The `COL04` hardware IDs include `HID\VEN_WCOM&DEV_016C&Col04` and `HID\WCOM016C&Col04`. The device currently reports no device-specific upper/lower filter. A hardware-ID match would still cover *all* matching devices, not exclusively this one instance. The eventual package must be reviewed for that scope. Microsoft's [device-specific filter guidance](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/installing-a-filter-driver) and [declarative AddFilter guidance](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/inf-addfilter-directive) describe a possible packaging route; no INF has been authored or installed yet.

## Offline builds

`GalaxyPenPassThrough.vcxproj` has three x64 configurations:

| Build | Behavior in source | Status |
| --- | --- | --- |
| Debug, Release | No I/O queue. KMDF automatically forwards unhandled I/O in a filter. | Compiled; not run or bound to any device. |
| Probe | A parallel read queue forwards each read, samples only successful 15-byte report IDs in a completion callback, and returns the lower driver's status and transfer count without editing the output buffer. Other I/O remains unhandled. | Compiled; not run or bound to any device. Kernel debug logging is rate-limited and does not retain pressure or coordinates. |

The Probe build is a **research variant**, not a shipping mode or pressure correction. Its behavior under cancellation, power transitions, and actual HID traffic has not been tested. It can show whether completed `IRP_MJ_READ` requests with ID `0x1A` traverse the chosen location, but only after a correctly scoped, safe attachment. An absence of Probe messages alone would not prove the pen uses another path: debugger logging, attachment, and activity must be checked first. Microsoft's [KMDF forwarding documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/forwarding-i-o-requests) and [completion guidance](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/completing-i-o-requests) are the basis for this source design.

## Go/no-go before deployment

1. Validate the pass-through and Probe variants on an isolated, nonessential HID test target with a matching driver model, including cancellation and suspend/resume. This checks the code path but does **not** validate WCOM report interception.
2. Create and statically verify a device-specific package; do not use a HIDClass-wide filter. Confirm the intended match and that the Microsoft function driver is not replaced. Keep a tested rollback package and keyboard/touchpad access independent of the pen.
3. Establish a compatible signing/test machine. The current `.sys` files are unsigned. [Microsoft requires a signature for x64 kernel code](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/windows-driver-signing-tutorial); test-signing can require boot-policy changes and a restart, with Secure Boot and BitLocker considerations. None of those settings have been changed. Elevated checks of this machine's Secure Boot/BitLocker state were unavailable in this session, so treat them as unknown.
4. Only after those gates, request explicit user approval for a controlled Galaxy-device attachment. Record a baseline with the existing monitor, verify pass-through without duplicate/absent pen events, then use Probe to see whether live `COL04` report `0x1A` actually appears. Do not enable a pressure transform before that evidence exists.

The currently saturated/reversed PW500 pressure readings remain a separate data-quality limit: a filter cannot infer force lost before Windows receives it.
