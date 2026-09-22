#include "RawHidCapture.h"

#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

namespace {

std::wstring deviceName(HANDLE device) {
    UINT length = 0;
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, nullptr, &length) == static_cast<UINT>(-1) || length == 0) {
        return {};
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, buffer.data(), &length) == static_cast<UINT>(-1)) {
        return {};
    }
    return buffer.data();
}

std::string hexBytes(const BYTE* data, size_t size) {
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0');
    for (size_t index = 0; index < size; ++index) {
        if (index != 0) out << ' ';
        out << std::setw(2) << static_cast<unsigned>(data[index]);
    }
    return out.str();
}

} // namespace

bool RawHidCapture::initialize(HWND window) {
    RAWINPUTDEVICE device{};
    device.usUsagePage = 0x0D; // Digitizers
    device.usUsage = 0;
    device.dwFlags = RIDEV_PAGEONLY | RIDEV_DEVNOTIFY;
    device.hwndTarget = window;
    return RegisterRawInputDevices(&device, 1, sizeof(device)) != FALSE;
}

std::vector<RawHidReport> RawHidCapture::process(HRAWINPUT input) const {
    UINT size = 0;
    if (GetRawInputData(input, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0) {
        return {};
    }

    std::vector<BYTE> buffer(size);
    if (GetRawInputData(input, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size) {
        return {};
    }

    const auto* raw = reinterpret_cast<const RAWINPUT*>(buffer.data());
    if (raw->header.dwType != RIM_TYPEHID) return {};

    RID_DEVICE_INFO info{};
    info.cbSize = sizeof(info);
    UINT infoSize = sizeof(info);
    if (GetRawInputDeviceInfoW(raw->header.hDevice, RIDI_DEVICEINFO, &info, &infoSize) == static_cast<UINT>(-1) ||
        info.dwType != RIM_TYPEHID || info.hid.usUsagePage != 0x0D) {
        return {};
    }

    const RAWHID& hid = raw->data.hid;
    if (hid.dwCount == 0 || hid.dwSizeHid == 0) return {};

    std::vector<RawHidReport> reports;
    reports.reserve(hid.dwCount);
    const std::wstring name = deviceName(raw->header.hDevice);
    for (DWORD index = 0; index < hid.dwCount; ++index) {
        RawHidReport report;
        report.deviceName = name;
        report.vendorId = info.hid.dwVendorId;
        report.productId = info.hid.dwProductId;
        report.usagePage = info.hid.usUsagePage;
        report.usage = info.hid.usUsage;
        report.reportIndex = index;
        report.bytes = hexBytes(hid.bRawData + (static_cast<size_t>(index) * hid.dwSizeHid), hid.dwSizeHid);
        reports.push_back(std::move(report));
    }
    return reports;
}
