#pragma once

#include <windows.h>
#include <string>

struct PenState {
    bool inRange = false;
    bool inContact = false;
    bool barrel = false;
    unsigned pressure = 0;
    int tiltX = 0;
    int tiltY = 0;
    POINT position{};
};

class PenCapture {
public:
    bool initialize(HWND window);
    const PenState& state() const { return current; }
    void processPointer(UINT message, WPARAM wParam);

private:
    PenState current;
};
