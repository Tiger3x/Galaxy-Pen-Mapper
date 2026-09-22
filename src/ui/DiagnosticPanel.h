#pragma once

#include <windows.h>
#include "../input/PenCapture.h"

class DiagnosticPanel {
public:
    void draw(HDC dc, const PenState& state, unsigned long events);
};
