# Galaxy Pen Mapper

Experimental Windows project to investigate and remap partially compatible EMR/HID pens on the Samsung Galaxy Book3 360.

The first hardware target is the **Huion PW500** (from the HS611) used directly on the Galaxy Book3 360 display, compared against the native Samsung S Pen.

## Current status

- **P001 — HID Scanner: implemented and build-validated**
- **P002 — Pen Event Monitor: implemented and build-validated**
- **P003 — Comparative Capture: completed with four hardware paths**
- **P004 — Input Classification: in progress with pressure and button fields identified**

### P001 — GalaxyPenHidScanner.exe

Enumerates HID interfaces and reports VID/PID, Usage Page/Usage, report sizes and Digitizer candidates.

### P002.1 — GalaxyPenEventMonitor.exe

Native window-only test bench. Captures:

- raw Digitizer HID reports through Windows Raw Input when available;
- interpreted pen events through `WM_POINTER` / `GetPointerPenInfo`;
- pressure, tilt, buttons/pen flags, coordinates and timestamps;
- CSV logs for later S Pen vs PW500 comparison;
- only while capture is enabled, the window is active, and the pen is inside the bordered test area.

> A passive EMR pen normally does not enumerate as a separate USB/HID device on the display. The digitizer is the HID device; P002 observes how that digitizer reports each pen.

## Build

Requirements:

- Windows 10/11
- Visual Studio 2022 or Build Tools with Desktop development with C++
- CMake 3.21+

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Executables:

```text
build\Release\GalaxyPenHidScanner.exe
build\Release\GalaxyPenEventMonitor.exe
```

## Documentation

- [Development plan](docs/PLAN.md)
- [P001 — HID Scanner](docs/P001-HID-SCANNER.md)
- [P002 — Pen Event Monitor](docs/P002-PEN-EVENT-MONITOR.md)
- [P004 — Input Classification](docs/P004-INPUT-CLASSIFICATION.md)
