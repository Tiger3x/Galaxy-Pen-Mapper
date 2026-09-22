#pragma once

#include "../input/PenCapture.h"
#include "../input/RawHidCapture.h"

#include <filesystem>
#include <fstream>
#include <string>

class SessionWriter {
public:
    bool start(const std::wstring& name, const std::wstring& description);
    void writePointer(const std::string& event, const PenState& state);
    void writeRaw(const RawHidReport& report);
    void writeRawMouse(const RawMouseEvent& event);
    void writeRawKeyboard(const RawKeyboardEvent& event);
    void writeWindowInput(const std::string& event, WPARAM wParam, LPARAM lParam,
                          unsigned sourceDevice, unsigned sourceOrigin);
    void writeWindowState(const std::string& event, WPARAM wParam, LPARAM lParam);
    void stop();
    bool active() const { return output.is_open(); }
    const std::filesystem::path& path() const { return outputPath; }

private:
    std::ofstream output;
    std::filesystem::path outputPath;
    std::string session;
    std::string description;
};
