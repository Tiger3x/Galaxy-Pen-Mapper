#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace {

using HidStringGetter = BOOLEAN(WINAPI*)(HANDLE, PVOID, ULONG);

std::wstring ReadRegistryString(
    HDEVINFO devices,
    SP_DEVINFO_DATA& deviceInfo,
    DWORD property) {
    DWORD requiredSize = 0;
    DWORD propertyType = 0;

    SetupDiGetDeviceRegistryPropertyW(
        devices, &deviceInfo, property, &propertyType, nullptr, 0, &requiredSize);

    if (requiredSize == 0) {
        return L"";
    }

    std::vector<BYTE> buffer(requiredSize + sizeof(wchar_t));
    if (!SetupDiGetDeviceRegistryPropertyW(
            devices,
            &deviceInfo,
            property,
            &propertyType,
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            nullptr)) {
        return L"";
    }

    return reinterpret_cast<const wchar_t*>(buffer.data());
}

std::wstring ReadHidString(HANDLE device, HidStringGetter getter) {
    wchar_t buffer[256]{};
    if (!getter(device, buffer, sizeof(buffer))) {
        return L"";
    }
    return buffer;
}

void PrintHex16(const wchar_t* label, USHORT value) {
    std::wcout << L"  " << label << L": 0x"
               << std::uppercase << std::hex << std::setw(4)
               << std::setfill(L'0') << value
               << std::dec << std::setfill(L' ') << L"\n";
}

bool IsDigitizer(const HIDP_CAPS& caps) {
    return caps.UsagePage == 0x0D;
}

void PrintDevice(
    DWORD index,
    const std::wstring& path,
    const std::wstring& description,
    HANDLE device) {
    std::wcout << L"\n============================================================\n";
    std::wcout << L"[" << index << L"] " << (description.empty() ? L"(sem descrição)" : description) << L"\n";
    std::wcout << L"Path: " << path << L"\n";

    HIDD_ATTRIBUTES attributes{};
    attributes.Size = sizeof(attributes);

    if (HidD_GetAttributes(device, &attributes)) {
        PrintHex16(L"VID", attributes.VendorID);
        PrintHex16(L"PID", attributes.ProductID);
        PrintHex16(L"Version", attributes.VersionNumber);
    } else {
        std::wcout << L"  VID/PID: indisponível (erro " << GetLastError() << L")\n";
    }

    const auto manufacturer = ReadHidString(device, HidD_GetManufacturerString);
    const auto product = ReadHidString(device, HidD_GetProductString);
    const auto serial = ReadHidString(device, HidD_GetSerialNumberString);

    if (!manufacturer.empty()) std::wcout << L"  Manufacturer: " << manufacturer << L"\n";
    if (!product.empty()) std::wcout << L"  Product: " << product << L"\n";
    if (!serial.empty()) std::wcout << L"  Serial: " << serial << L"\n";

    PHIDP_PREPARSED_DATA preparsed = nullptr;
    if (!HidD_GetPreparsedData(device, &preparsed)) {
        std::wcout << L"  HID caps: indisponíveis\n";
        return;
    }

    HIDP_CAPS caps{};
    const NTSTATUS status = HidP_GetCaps(preparsed, &caps);
    if (status == HIDP_STATUS_SUCCESS) {
        PrintHex16(L"Usage Page", caps.UsagePage);
        PrintHex16(L"Usage", caps.Usage);
        std::wcout << L"  Input report bytes: " << caps.InputReportByteLength << L"\n";
        std::wcout << L"  Output report bytes: " << caps.OutputReportByteLength << L"\n";
        std::wcout << L"  Feature report bytes: " << caps.FeatureReportByteLength << L"\n";
        std::wcout << L"  Input value caps: " << caps.NumberInputValueCaps << L"\n";
        std::wcout << L"  Input button caps: " << caps.NumberInputButtonCaps << L"\n";

        if (IsDigitizer(caps)) {
            std::wcout << L"  >>> CANDIDATO DIGITIZER (Usage Page 0x0D) <<<\n";
        }
    } else {
        std::wcout << L"  HidP_GetCaps falhou: 0x"
                   << std::hex << static_cast<unsigned long>(status) << std::dec << L"\n";
    }

    HidD_FreePreparsedData(preparsed);
}

} // namespace

int wmain() {
    SetConsoleOutputCP(CP_UTF8);

    GUID hidGuid{};
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO devices = SetupDiGetClassDevsW(
        &hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (devices == INVALID_HANDLE_VALUE) {
        std::wcerr << L"SetupDiGetClassDevsW falhou: " << GetLastError() << L"\n";
        return 1;
    }

    DWORD found = 0;

    for (DWORD index = 0;; ++index) {
        SP_DEVICE_INTERFACE_DATA interfaceData{};
        interfaceData.cbSize = sizeof(interfaceData);

        if (!SetupDiEnumDeviceInterfaces(
                devices, nullptr, &hidGuid, index, &interfaceData)) {
            if (GetLastError() != ERROR_NO_MORE_ITEMS) {
                std::wcerr << L"SetupDiEnumDeviceInterfaces falhou: " << GetLastError() << L"\n";
            }
            break;
        }

        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(
            devices, &interfaceData, nullptr, 0, &requiredSize, nullptr);

        if (requiredSize == 0) {
            continue;
        }

        std::vector<BYTE> detailBuffer(requiredSize);
        auto* detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(detailBuffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        SP_DEVINFO_DATA deviceInfo{};
        deviceInfo.cbSize = sizeof(deviceInfo);

        if (!SetupDiGetDeviceInterfaceDetailW(
                devices,
                &interfaceData,
                detail,
                requiredSize,
                nullptr,
                &deviceInfo)) {
            continue;
        }

        std::wstring description =
            ReadRegistryString(devices, deviceInfo, SPDRP_FRIENDLYNAME);
        if (description.empty()) {
            description = ReadRegistryString(devices, deviceInfo, SPDRP_DEVICEDESC);
        }

        HANDLE handle = CreateFileW(
            detail->DevicePath,
            0,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (handle == INVALID_HANDLE_VALUE) {
            std::wcout << L"\n[" << index << L"] "
                       << (description.empty() ? L"(sem descrição)" : description) << L"\n";
            std::wcout << L"Path: " << detail->DevicePath << L"\n";
            std::wcout << L"  Não foi possível abrir o HID (erro " << GetLastError() << L")\n";
            continue;
        }

        PrintDevice(index, detail->DevicePath, description, handle);
        CloseHandle(handle);
        ++found;
    }

    SetupDiDestroyDeviceInfoList(devices);

    std::wcout << L"\n============================================================\n";
    std::wcout << L"HIDs abertos e analisados: " << found << L"\n";
    std::wcout << L"Procure especialmente por entradas marcadas como CANDIDATO DIGITIZER.\n";

    return 0;
}
