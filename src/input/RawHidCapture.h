#pragma once

#include <windows.h>

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

class RawHidCapture {
public:
    bool initialize(HWND window);
    std::vector<RawHidReport> process(HRAWINPUT input) const;
};
