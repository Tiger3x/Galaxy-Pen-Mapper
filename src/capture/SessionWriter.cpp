#include "SessionWriter.h"

#include <chrono>
#include <array>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace {

std::string utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::string csv(std::string value) {
    std::string escaped = "\"";
    for (const char character : value) {
        if (character == '\"') escaped += '\"';
        escaped += character;
    }
    return escaped + "\"";
}

std::string timestamp(const char* format) {
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_s(&local, &time);

    std::ostringstream result;
    result << std::put_time(&local, format);
    if (std::string_view(format) == "%Y-%m-%d %H:%M:%S") {
        result << '.' << std::setw(3) << std::setfill('0') << milliseconds.count();
    }
    return result.str();
}

void writeRow(std::ostream& output, const std::array<std::string, 33>& fields) {
    for (size_t index = 0; index < fields.size(); ++index) {
        if (index != 0) output << ',';
        output << csv(fields[index]);
    }
    output << '\n';
}

} // namespace

bool SessionWriter::start(const std::wstring& name, const std::wstring& details) {
    stop();
    session = utf8(name.empty() ? L"Sem nome" : name);
    description = utf8(details);

    std::error_code error;
    const std::filesystem::path captures = std::filesystem::current_path() / "captures";
    std::filesystem::create_directories(captures, error);
    if (error) return false;

    outputPath = captures / ("pen-events-" + timestamp("%Y%m%d-%H%M%S") + ".csv");
    output.open(outputPath, std::ios::out | std::ios::trunc);
    if (!output) return false;

    output << "session,description,timestamp,event,device,vid,pid,usage_page,usage,report_index,raw_hex,pointer_id,pointer_flags,pen_flags,pressure,tilt_x,tilt_y,rotation,x,y,input_source_device,input_source_origin,message_wparam,message_lparam,mouse_flags,mouse_button_flags,mouse_button_data,mouse_dx,mouse_dy,key_make_code,key_flags,key_vkey,raw_extra_information\n";
    output << csv(session) << ',' << csv(description) << ',' << csv(timestamp("%Y-%m-%d %H:%M:%S")) << ",SESSION_START,,,,,,,,,,,,,,,,,\n";
    output.flush();
    return true;
}

void SessionWriter::writePointer(const std::string& event, const PenState& state) {
    if (!active()) return;
    output << csv(session) << ',' << csv(description) << ',' << csv(timestamp("%Y-%m-%d %H:%M:%S")) << ',' << event
           << ",WINDOWS_POINTER,,,,,,,"
           << state.pointerId << ',' << csv(std::to_string(state.pointerFlags)) << ',' << csv(std::to_string(state.penFlags)) << ','
           << state.pressure << ',' << state.tiltX << ',' << state.tiltY << ',' << state.rotation << ','
           << state.position.x << ',' << state.position.y << '\n';
    output.flush();
}

void SessionWriter::writeRaw(const RawHidReport& report) {
    if (!active()) return;
    output << csv(session) << ',' << csv(description) << ',' << csv(timestamp("%Y-%m-%d %H:%M:%S")) << ",RAW_HID,"
           << csv(utf8(report.deviceName)) << ',' << report.vendorId << ',' << report.productId << ','
           << report.usagePage << ',' << report.usage << ',' << report.reportIndex << ',' << csv(report.bytes)
           << ",,,,,,,,,,\n";
    output.flush();
}

void SessionWriter::writeRawMouse(const RawMouseEvent& event) {
    if (!active()) return;
    std::array<std::string, 33> fields{};
    fields[0] = session;
    fields[1] = description;
    fields[2] = timestamp("%Y-%m-%d %H:%M:%S");
    fields[3] = "RAW_MOUSE";
    fields[4] = utf8(event.deviceName);
    fields[24] = std::to_string(event.flags);
    fields[25] = std::to_string(event.buttonFlags);
    fields[26] = std::to_string(event.buttonData);
    fields[27] = std::to_string(event.deltaX);
    fields[28] = std::to_string(event.deltaY);
    fields[32] = std::to_string(event.extraInformation);
    writeRow(output, fields);
    output.flush();
}

void SessionWriter::writeRawKeyboard(const RawKeyboardEvent& event) {
    if (!active()) return;
    std::array<std::string, 33> fields{};
    fields[0] = session;
    fields[1] = description;
    fields[2] = timestamp("%Y-%m-%d %H:%M:%S");
    fields[3] = "RAW_KEYBOARD";
    fields[4] = utf8(event.deviceName);
    fields[22] = std::to_string(event.message);
    fields[32] = std::to_string(event.extraInformation);
    fields[29] = std::to_string(event.makeCode);
    fields[30] = std::to_string(event.flags);
    fields[31] = std::to_string(event.virtualKey);
    writeRow(output, fields);
    output.flush();
}

void SessionWriter::writeWindowInput(const std::string& event, WPARAM wParam, LPARAM lParam,
                                     unsigned sourceDevice, unsigned sourceOrigin) {
    if (!active()) return;
    std::array<std::string, 33> fields{};
    fields[0] = session;
    fields[1] = description;
    fields[2] = timestamp("%Y-%m-%d %H:%M:%S");
    fields[3] = event;
    fields[4] = "WINDOW_MESSAGE";
    fields[20] = std::to_string(sourceDevice);
    fields[21] = std::to_string(sourceOrigin);
    fields[22] = std::to_string(static_cast<unsigned long long>(wParam));
    fields[23] = std::to_string(static_cast<unsigned long long>(lParam));
    writeRow(output, fields);
    output.flush();
}

void SessionWriter::writeWindowState(const std::string& event, WPARAM wParam, LPARAM lParam) {
    writeWindowInput(event, wParam, lParam, 0, 0);
}

void SessionWriter::stop() {
    if (!active()) return;
    output << csv(session) << ',' << csv(description) << ',' << csv(timestamp("%Y-%m-%d %H:%M:%S")) << ",SESSION_STOP,,,,,,,,,,,,,,,,,\n";
    output.flush();
    output.close();
}
