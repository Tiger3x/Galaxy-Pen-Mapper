#pragma once

#include <windows.h>
#include "../input/PenCapture.h"
#include "../input/PenSignature.h"

#include <string>

class DiagnosticPanel {
public:
    RECT captureArea(HWND window) const;
    bool contains(HWND window, POINT screenPoint) const;
    void draw(HDC dc, HWND window, const PenState& state, unsigned long long events,
              bool recording, bool penInside, const std::wstring& status,
              const std::wstring& latestRaw, PenSignature signature,
              std::optional<unsigned> rawPressure) const;
};
