#pragma once

#include <windows.h>

#include <optional>
#include <string>
#include <vector>

struct RawHidReport {
    std::wstring deviceName;
    unsigned vendorId = 0;
    unsigned productId = 0;
    unsigned usagePage = 0;
    unsigned usage = 0;
    unsigned reportIndex = 0;
    std::string bytes;
};

struct RawMouseEvent {
    std::wstring deviceName;
    unsigned short flags = 0;
    unsigned short buttonFlags = 0;
    unsigned short buttonData = 0;
    long deltaX = 0;
    long deltaY = 0;
    unsigned long extraInformation = 0;
};

struct RawKeyboardEvent {
    std::wstring deviceName;
    unsigned short makeCode = 0;
    unsigned short flags = 0;
    unsigned short virtualKey = 0;
    unsigned message = 0;
    unsigned long extraInformation = 0;
};

struct RawInputBatch {
    std::vector<RawHidReport> hidReports;
    std::optional<RawMouseEvent> mouse;
    std::optional<RawKeyboardEvent> keyboard;
};

class RawHidCapture {
public:
    bool initialize(HWND window);
    RawInputBatch process(HRAWINPUT input) const;
};
