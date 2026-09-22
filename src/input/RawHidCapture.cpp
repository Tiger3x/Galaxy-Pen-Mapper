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
    RAWINPUTDEVICE devices[] = {
        {0x0D, 0x00, RIDEV_PAGEONLY | RIDEV_DEVNOTIFY, window}, // Digitizers
        {0x01, 0x02, RIDEV_DEVNOTIFY, window},                 // Mouse
        {0x01, 0x06, RIDEV_DEVNOTIFY, window},                 // Keyboard
    };
    return RegisterRawInputDevices(devices, ARRAYSIZE(devices), sizeof(RAWINPUTDEVICE)) != FALSE;
}

RawInputBatch RawHidCapture::process(HRAWINPUT input) const {
    RawInputBatch batch;
    UINT size = 0;
    if (GetRawInputData(input, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0) {
        return batch;
    }

    std::vector<BYTE> buffer(size);
    if (GetRawInputData(input, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size) {
        return batch;
    }

    const auto* raw = reinterpret_cast<const RAWINPUT*>(buffer.data());
    const std::wstring name = deviceName(raw->header.hDevice);

    if (raw->header.dwType == RIM_TYPEMOUSE) {
        RawMouseEvent event;
        event.deviceName = name;
        event.flags = raw->data.mouse.usFlags;
        event.buttonFlags = raw->data.mouse.usButtonFlags;
        event.buttonData = raw->data.mouse.usButtonData;
        event.deltaX = raw->data.mouse.lLastX;
        event.deltaY = raw->data.mouse.lLastY;
        event.extraInformation = raw->data.mouse.ulExtraInformation;
        batch.mouse = std::move(event);
        return batch;
    }

    if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        RawKeyboardEvent event;
        event.deviceName = name;
        event.makeCode = raw->data.keyboard.MakeCode;
        event.flags = raw->data.keyboard.Flags;
        event.virtualKey = raw->data.keyboard.VKey;
        event.message = raw->data.keyboard.Message;
        event.extraInformation = raw->data.keyboard.ExtraInformation;
        batch.keyboard = std::move(event);
        return batch;
    }

    if (raw->header.dwType != RIM_TYPEHID) return batch;

    RID_DEVICE_INFO info{};
    info.cbSize = sizeof(info);
    UINT infoSize = sizeof(info);
    if (GetRawInputDeviceInfoW(raw->header.hDevice, RIDI_DEVICEINFO, &info, &infoSize) == static_cast<UINT>(-1) ||
        info.dwType != RIM_TYPEHID || info.hid.usUsagePage != 0x0D) {
        return batch;
    }

    const RAWHID& hid = raw->data.hid;
    if (hid.dwCount == 0 || hid.dwSizeHid == 0) return batch;

    batch.hidReports.reserve(hid.dwCount);
    for (DWORD index = 0; index < hid.dwCount; ++index) {
        RawHidReport report;
        report.deviceName = name;
        report.vendorId = info.hid.dwVendorId;
        report.productId = info.hid.dwProductId;
        report.usagePage = info.hid.usUsagePage;
        report.usage = info.hid.usUsage;
        report.reportIndex = index;
        report.bytes = hexBytes(hid.bRawData + (static_cast<size_t>(index) * hid.dwSizeHid), hid.dwSizeHid);
        batch.hidReports.push_back(std::move(report));
    }
    return batch;
}
