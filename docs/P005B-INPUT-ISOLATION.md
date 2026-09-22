# P005B — Original-input isolation review

## Decision

The existing captures are enough to advance. A separate new S Pen/PW500 tip recording is not needed for this architectural gate. The P002.6 label distinguishes the labeled recordings in replay, but is a conservative diagnostic clue, not a validated real-time authorization to suppress the physical pen.

For the goal of fixing the PW500 **across normal Windows applications**, the current user-mode observer cannot safely replace the original input. Do not turn on synthetic pen injection: Windows would still deliver the physical PW500 stream, creating a second pen path and potentially duplicate strokes or clicks. This is a conclusion about the documented APIs and the present design, not a proof that no conceivable application-specific integration or privileged component could ever work.

## Evidence and scope

| Mechanism | What it can do | Why it does not pass the general-desktop gate |
| --- | --- | --- |
| Raw Input / `WM_INPUT` | Receive the digitizer's reports, including in the background with `RIDEV_INPUTSINK`. | Registration receives data; it does not remove the original `WM_POINTER` stream from other processes. [`RIDEV_NOLEGACY`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawinputdevice) applies only to mouse and keyboard. |
| `WM_POINTER` handling | An application can process pointer messages aimed at **its own** window. | This is useful for a purpose-built canvas, but cannot decide how Blender, Photoshop or another app handles their windows. Windows warns against selectively consuming only some messages in a pointer sequence. [Microsoft documentation](https://learn.microsoft.com/en-us/windows/win32/inputmsg/wm-pointerdown). |
| Synthetic pen APIs | `CreateSyntheticPointerDevice(PT_PEN)` and `InjectSyntheticPointerInput` can produce pen events. | They simulate input; they have no paired switch to suppress the physical WCOM report. [Creation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createsyntheticpointerdevice), [injection](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-injectsyntheticpointerinput). |
| Cross-process message hooks | Some hooks can inspect or alter another process's message flow. | Global hooks require code in other processes, have 32/64-bit and app/security-boundary constraints, and affect the desktop broadly. They are not a reliable device-specific pen filter for this project. [Microsoft documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowshookexw). |

This makes a controlled canvas experiment possible but insufficient as the next proof: excluding pointer input inside our own window would not establish exclusion in the user's actual applications. No input injection, hooks, device disabling or driver changes were performed in this review.

## Read-only inventory of this Galaxy Book

Windows currently reports:

- `ACPI\WCOM016C\1`: physical HID-over-I²C parent, compatible ID `PNP0C50`, Microsoft `hidi2c` service and `mshidkmdf` upper filter;
- `HID\WCOM016C&COL01\5&A6B5543&0&0000`: "PenS2Helper Device", Digitizer usage `0x0D/0x02`, 15-byte input report ID `0x02`;
- `HID\WCOM016C&COL04\5&A6B5543&0&0003`: child "HID-compliant pen", Microsoft `input.inf`, Digitizer usage `0x0D/0x02`, 15-byte input report ID `0x1A`;
- sibling collections `COL02`, `COL03` and `COL05` under the same parent.

The names and IDs above come from read-only Windows PnP queries and the scanner's new `--wcom-pen-caps` mode on this machine on 2026-09-22. A direct check of the existing S Pen CSV shows its Raw HID `0x02` reports arrive from **`COL01`**, not `COL04`. The P002.6 recognition rule is therefore based on the helper collection. The apparent Windows pen collection `COL04` advertises report `0x1A`, including X/Y, pressure with logical range 0–4095 and a two-bit Digitizer usage `0x38` ("Transducer Index" in the [USB-IF HID usage tables](https://www.usb.org/sites/default/files/hut1_3_0.pdf)). We have **not captured its live `0x1A` bytes**, so cannot say whether that field distinguishes these two physical pens or whether a filter attached only to `COL04` would have enough information. The two collections must not be conflated. Windows documents integrated pen collections (`0x0D/0x02`) as opened for [exclusive system use](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/top-level-collections-opened-by-windows-for-system-use); the successful descriptor query does not imply a user-mode application can read that live system pen stream directly.

Windows HIDClass creates a separate device node for each top-level collection; that is why scoping an eventual pen filter to `COL04` is worth studying, while filtering the I²C parent is higher risk. A separate pen collection does **not** mean a filter is already proven safe or that the WCOM report can be changed at that level. [Top-level collections](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/top-level-collections), [HID over I²C](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/hid-over-i2c-guide).

## P007 research gate — no installation yet

1. Confirm the live `COL04` report path and whether a collection-scoped filter can intercept report `0x1A` *before* Windows creates pen events. Determine whether `COL04` alone carries reliable pen identity; do not assume the `COL01` mode bytes are visible in that filter.
2. Design a fail-open pass-through: unknown pen, mixed mode, startup, errors, or filter shutdown must leave the S Pen and original input unchanged. The 1-second P002.6 candidate delay cannot by itself deliver a corrected first stroke, so identification must be examined further before any implementation.
3. Specify an isolated test setup and recovery procedure before considering any driver binding on the user's only live machine. A driver in the input path can break pen input; it also needs appropriate signing on 64-bit Windows. [Microsoft driver signing](https://learn.microsoft.com/en-us/windows-hardware/drivers/develop/signing-a-driver).
4. Decide with the user whether to pursue this higher-risk driver branch or an explicitly limited integration for one application. Neither path can reconstruct the pressure samples already saturated at 4095.

No driver is installed, replaced or disabled by this milestone. The diagnostic monitor remains observation-only.
