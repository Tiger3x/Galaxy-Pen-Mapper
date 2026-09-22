#pragma once

#include <windows.h>
#include <string>

struct PenState {
    UINT32 pointerId = 0;
    UINT32 pointerFlags = 0;
    PEN_FLAGS penFlags = PEN_FLAG_NONE;
    bool inRange = false;
    bool inContact = false;
    bool barrel = false;
    unsigned pressure = 0;
    int tiltX = 0;
    int tiltY = 0;
    unsigned rotation = 0;
    POINT position{};
};

class PenCapture {
public:
    bool initialize(HWND window);
    const PenState& state() const { return current; }
    bool processPointer(UINT message, WPARAM wParam);
    void clear();

private:
    PenState current;
};
