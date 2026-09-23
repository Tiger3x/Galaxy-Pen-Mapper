# Galaxy Pen Mapper

Experimental Windows project to investigate and remap partially compatible EMR/HID pens on the Samsung Galaxy Book3 360.

The first hardware target is the **Huion PW500** (from the HS611) used directly on the Galaxy Book3 360 display. The native Samsung S Pen was a diagnostic reference, not a requirement for people using PW500 correction mode.

## Current status

- **P001 — HID Scanner: implemented and build-validated**
- **P002 — Pen Event Monitor: implemented and build-validated (P002.6)**
- **P003 — Comparative Capture: completed with four hardware paths**
- **P004 — Input Classification: completed; HS611 button-time tip interruption recorded as established behavior**
- **P005A — Tip-first pen-pattern probe: implemented; existing captures suffice for the next gate**
- **P005B — Input isolation: reviewed; no reliable general-desktop user-mode replacement path identified**
- **P007 — Manual PW500 mode selected; offline report-layout check passed and pass-through filter compiled; no driver installed or device changed**

### P001 — GalaxyPenHidScanner.exe

Enumerates HID interfaces and reports VID/PID, Usage Page/Usage, report sizes and Digitizer candidates.

### P002.6 — GalaxyPenEventMonitor.exe

Native window-only test bench. Captures:

- raw Digitizer HID reports through Windows Raw Input when available;
- interpreted pen events through `WM_POINTER` / `GetPointerPenInfo`;
- Raw Mouse, Raw Keyboard and synthesized window input, including Windows input-source metadata;
- pressure, tilt, buttons/pen flags, coordinates and timestamps;
- CSV logs for later S Pen vs PW500 comparison;
- interpreted pen events inside the bordered test area; Raw Mouse and Raw Keyboard button/key events only while the monitor is in the foreground;
- Digitizer Raw HID reports throughout a recording session, even if the monitor loses focus, plus a foreground sample about every 50 ms. The CSV records `window_foreground` and `raw_input_code` for Raw Input rows, with millisecond timestamps.
- a conservative live WCOM pattern indicator for S Pen or probable PW500, plus raw pressure beside Windows pressure. `pen_signature` and `raw_pressure` are appended to Raw HID CSV rows. Hover-only or mixed evidence stays inconclusive; the indicator does not change Windows input.

> A passive EMR pen normally does not enumerate as a separate USB/HID device on the display. The digitizer is the HID device; P002 observes how that digitizer reports each pen. The current captures show that the PW500's true pressure range cannot be recovered from the Galaxy display reports.

## Build

Requirements:

- Windows 10/11
- Visual Studio 2022 or Build Tools with Desktop development with C++
- CMake 3.21+ and Ninja (both included in the installed Visual Studio Build Tools)

From an x64 Visual Studio developer command prompt with CMake and Ninja available:

```text
cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-ninja
ctest --test-dir build-ninja --output-on-failure
```

Executables:

```text
build-ninja\GalaxyPenHidScanner.exe
build-ninja\GalaxyPenEventMonitor.exe
```

The P007 filter source under `driver/` is a separate, non-installable WDK research project. The application and tests use Ninja as above; the kernel driver requires the Visual Studio 2022 WDK toolset and is compiled separately with MSBuild:

```text
MSBuild.exe driver\GalaxyPenPassThrough.vcxproj /p:Configuration=Debug /p:Platform=x64
```

The tested output is an unsigned `.sys` with no INF or install package. Building it does not attach it to the pen. Do not install or bind this prototype to the Galaxy digitizer.

## Documentation

- [Development plan](docs/PLAN.md)
- [P001 — HID Scanner](docs/P001-HID-SCANNER.md)
- [P002 — Pen Event Monitor](docs/P002-PEN-EVENT-MONITOR.md)
- [P004 — Input Classification](docs/P004-INPUT-CLASSIFICATION.md)
- [P005 — PW500 screen-button verification](docs/P005-SCREEN-BUTTON-TEST.md)
- [P005A — Tip-first pen-pattern probe and test](docs/P005A-PEN-SIGNATURE.md)
- [P005B — Original-input isolation review](docs/P005B-INPUT-ISOLATION.md)
- [P007 — Manual PW500 mode and safety contract](docs/P007-MANUAL-MODE.md)
