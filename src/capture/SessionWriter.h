#pragma once

#include "../input/PenCapture.h"
#include "../input/RawHidCapture.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

class SessionWriter {
public:
    bool start(const std::wstring& name, const std::wstring& description);
    void writePointer(const std::string& event, const PenState& state);
    void writeRaw(const RawHidReport& report, bool windowForeground, unsigned rawInputCode,
                  const std::string& signature, std::optional<unsigned> rawPressure);
    void writeRawMouse(const RawMouseEvent& event, bool windowForeground, unsigned rawInputCode);
    void writeRawKeyboard(const RawKeyboardEvent& event, bool windowForeground, unsigned rawInputCode);
    void writeForegroundSample(bool windowForeground);
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
