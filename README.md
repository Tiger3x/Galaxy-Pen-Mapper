# Galaxy Pen Mapper

Experimental Windows project to investigate and remap partially compatible EMR/HID pens on the Samsung Galaxy Book3 360.

The first hardware target is the **Huion PW500** (from the HS611) used directly on the Galaxy Book3 360 display, compared against the native Samsung S Pen.

## Current status

**P001 — HID Scanner: implemented**

The scanner enumerates HID interfaces exposed by Windows and prints:

- device path and Windows description;
- Vendor ID / Product ID / Version;
- HID Usage Page / Usage;
- input/output/feature report sizes;
- manufacturer, product and serial strings when available;
- a marker for Digitizer-class devices (Usage Page 0x0D).

> Important: a passive EMR pen normally does not enumerate as a separate USB/HID device. P001 identifies the digitizer(s). P002 will capture live reports so we can compare how the digitizer represents S Pen vs PW500.

## Build

Requirements:

- Windows 10/11
- Visual Studio 2022 or Build Tools with Desktop development with C++
- CMake 3.21+

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\GalaxyPenHidScanner.exe
```

## Roadmap

See [docs/PLAN.md](docs/PLAN.md).
