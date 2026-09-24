#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>

#include "GalaxyPenToolFlags.h"
#include "GalaxyPenEngine.h"

#include <algorithm>
#include <cwctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
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

bool RunWcomFlagLab(HANDLE device) {
    PHIDP_PREPARSED_DATA preparsed = nullptr;
    if (!HidD_GetPreparsedData(device, &preparsed)) return false;

    HIDP_CAPS caps{};
    const bool valid = HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS &&
                       caps.UsagePage == 0x0D && caps.Usage == 0x02 &&
                       caps.InputReportByteLength == 15;
    if (!valid) {
        HidD_FreePreparsedData(preparsed);
        return false;
    }

    std::wcout << L"Decodificação em memória dos bytes de estado observados em COL01:\n";
    const auto decode = [&](std::vector<char>& report, const wchar_t* label) {
        USAGE usages[16]{};
        ULONG count = static_cast<ULONG>(std::size(usages));
        const NTSTATUS status = HidP_GetUsages(HidP_Input, 0x0D, 0, usages, &count,
                                               preparsed, report.data(),
                                               static_cast<ULONG>(report.size()));
        if (status != HIDP_STATUS_SUCCESS) {
            std::wcout << label << L"0x" << std::hex
                       << static_cast<unsigned>(static_cast<unsigned char>(report[1]))
                       << L": falha 0x" << static_cast<unsigned long>(status)
                       << std::dec << L"\n";
            return false;
        }
        const auto active = [&](USAGE usage) {
            return std::find(usages, usages + count, usage) != usages + count;
        };
        std::wcout << label << L"0x" << std::hex
                   << static_cast<unsigned>(static_cast<unsigned char>(report[1])) << std::dec
                   << L": ponta=" << active(0x42)
                   << L", invertido=" << active(0x3C)
                   << L", borracha=" << active(0x45)
                   << L", alcance=" << active(0x32)
                   << L", botão lateral=" << active(0x44)
                   << L"\n";
        return true;
    };

    bool success = true;
    constexpr unsigned char observedFlags[] = {0x20, 0x21, 0x28, 0x2C};
    for (const unsigned char flag : observedFlags) {
        std::vector<char> report(caps.InputReportByteLength);
        report[0] = 0x02;
        report[1] = static_cast<char>(flag);
        success = decode(report, L"  observado ") && success;
        if (GalaxyPenNormalizePw500ToolFlags(
                reinterpret_cast<unsigned char*>(report.data()), report.size()) != 0) {
            success = decode(report, L"    proposta ") && success;
        }
    }

    HidD_FreePreparsedData(preparsed);
    return success;
}

bool RunPenReportLab(HANDLE device, UCHAR reportId = 0x1A) {
    PHIDP_PREPARSED_DATA preparsed = nullptr;
    if (!HidD_GetPreparsedData(device, &preparsed)) {
        std::wcout << L"  Laboratório: não foi possível ler a descrição HID.\n";
        return false;
    }

    HIDP_CAPS caps{};
    const bool matchingCollection = HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS &&
                                    caps.UsagePage == 0x0D && caps.Usage == 0x02 &&
                                    caps.InputReportByteLength == 15;
    if (!matchingCollection) {
        std::wcout << L"  Laboratório: coleção ou tamanho de relatório inesperado.\n";
        HidD_FreePreparsedData(preparsed);
        return false;
    }

    bool pressureCapability = false;
    USHORT count = caps.NumberInputValueCaps;
    std::vector<HIDP_VALUE_CAPS> values(count);
    if (count != 0 && HidP_GetValueCaps(HidP_Input, values.data(), &count, preparsed) == HIDP_STATUS_SUCCESS) {
        for (USHORT index = 0; index < count; ++index) {
            const auto& value = values[index];
            if (value.ReportID == reportId && value.UsagePage == 0x0D && !value.IsRange &&
                value.NotRange.Usage == 0x30 && value.LogicalMin == 0 && value.LogicalMax == 4095 &&
                value.ReportCount == 1) {
                pressureCapability = true;
                break;
            }
        }
    }
    if (!pressureCapability) {
        std::wcout << L"  Laboratório: campo de pressão não corresponde ao esperado.\n";
        HidD_FreePreparsedData(preparsed);
        return false;
    }

    std::wcout << L"  Laboratório: relatório 0x" << std::hex << static_cast<unsigned>(reportId)
               << std::dec << L" criado somente em memória; nada será enviado ao dispositivo.\n";
    bool success = true;
    for (const ULONG requested : {0UL, 699UL, 4095UL}) {
        std::vector<char> report(caps.InputReportByteLength);
        const NTSTATUS initialized = HidP_InitializeReportForID(HidP_Input, reportId, preparsed,
                                                                 report.data(), static_cast<ULONG>(report.size()));
        const NTSTATUS written = initialized == HIDP_STATUS_SUCCESS ?
            HidP_SetUsageValue(HidP_Input, 0x0D, 0, 0x30, requested, preparsed,
                               report.data(), static_cast<ULONG>(report.size())) : initialized;
        ULONG observed = 0;
        const NTSTATUS read = written == HIDP_STATUS_SUCCESS ?
            HidP_GetUsageValue(HidP_Input, 0x0D, 0, 0x30, &observed, preparsed,
                               report.data(), static_cast<ULONG>(report.size())) : written;
        const bool roundTrip = read == HIDP_STATUS_SUCCESS && observed == requested &&
                               static_cast<unsigned char>(report[0]) == reportId;
        std::wcout << L"  Pressão solicitada=" << requested << L", lida=" << observed
                   << L", resultado=" << (roundTrip ? L"OK" : L"FALHOU") << L", bytes=";
        for (unsigned char byte : report) {
            std::wcout << L' ' << std::hex << std::uppercase << std::setw(2) << std::setfill(L'0')
                       << static_cast<unsigned>(byte);
        }
        std::wcout << std::dec << std::setfill(L' ') << L"\n";
        success = success && roundTrip;
    }

    std::vector<char> contact(caps.InputReportByteLength);
    const ULONG reportSize = static_cast<ULONG>(contact.size());
    USAGE tipSwitch = 0x42;
    ULONG tipCount = 1;
    const bool contactReady =
        HidP_InitializeReportForID(HidP_Input, reportId, preparsed, contact.data(), reportSize) == HIDP_STATUS_SUCCESS &&
        HidP_SetUsageValue(HidP_Input, 0x01, 0, 0x30, 15000, preparsed, contact.data(), reportSize) == HIDP_STATUS_SUCCESS &&
        HidP_SetUsageValue(HidP_Input, 0x01, 0, 0x31, 8000, preparsed, contact.data(), reportSize) == HIDP_STATUS_SUCCESS &&
        HidP_SetUsageValue(HidP_Input, 0x0D, 0, 0x30, 699, preparsed, contact.data(), reportSize) == HIDP_STATUS_SUCCESS &&
        HidP_SetUsages(HidP_Input, 0x0D, 0, &tipSwitch, &tipCount, preparsed,
                       contact.data(), reportSize) == HIDP_STATUS_SUCCESS;
    if (contactReady) {
        const auto before = contact;
        std::wcout << L"  Contato sintético antes de alterar pressão, bytes=";
        for (unsigned char byte : before) {
            std::wcout << L' ' << std::hex << std::uppercase << std::setw(2) << std::setfill(L'0')
                       << static_cast<unsigned>(byte);
        }
        std::wcout << std::dec << std::setfill(L' ') << L"\n";
        ULONG x = 0, y = 0, pressure = 0;
        const bool modified =
            HidP_SetUsageValue(HidP_Input, 0x0D, 0, 0x30, 1234, preparsed,
                               contact.data(), reportSize) == HIDP_STATUS_SUCCESS &&
            HidP_GetUsageValue(HidP_Input, 0x01, 0, 0x30, &x, preparsed,
                               contact.data(), reportSize) == HIDP_STATUS_SUCCESS &&
            HidP_GetUsageValue(HidP_Input, 0x01, 0, 0x31, &y, preparsed,
                               contact.data(), reportSize) == HIDP_STATUS_SUCCESS &&
            HidP_GetUsageValue(HidP_Input, 0x0D, 0, 0x30, &pressure, preparsed,
                               contact.data(), reportSize) == HIDP_STATUS_SUCCESS;
        bool otherBytesIntact = true;
        for (size_t index = 0; index < contact.size(); ++index) {
            if (index != 6 && index != 7 && contact[index] != before[index]) otherBytesIntact = false;
        }
        const bool preserved = modified && otherBytesIntact && x == 15000 && y == 8000 &&
                               pressure == 1234 && before[6] != contact[6] && before[7] != contact[7];
        std::wcout << L"  Mudança de pressão com posição e ponta presentes: "
                   << (preserved ? L"outros campos preservados" : L"FALHOU") << L"\n";
        std::wcout << L"  Contato sintético depois de alterar pressão, bytes=";
        for (unsigned char byte : contact) {
            std::wcout << L' ' << std::hex << std::uppercase << std::setw(2) << std::setfill(L'0')
                       << static_cast<unsigned>(byte);
        }
        std::wcout << std::dec << std::setfill(L' ') << L"\n";
        success = success && preserved;
    } else {
        std::wcout << L"  Laboratório: não foi possível montar contato com posição e ponta.\n";
        success = false;
    }

    if (reportId == 0x02 && contactReady) {
        // Run the exact driver engine against a report built by the HID parser.
        GALAXY_PEN_ENGINE engine{1, 0, 0, 0, 0, 4500, 5};
        contact[1] = 0x2C;
        const bool pressureWritten = HidP_SetUsageValue(HidP_Input, 0x0D, 0, 0x30, 4000,
            preparsed, contact.data(), reportSize) == HIDP_STATUS_SUCCESS;
        const auto before = contact;
        auto hover = contact;
        hover[1] = 0x28;
        hover[6] = hover[7] = 0;
        GalaxyPenEngineConfigure(&engine, 1, 1, 4500, 5, 100);
        GalaxyPenEngineProcess(&engine, reinterpret_cast<unsigned char*>(hover.data()), hover.size(), engine.Generation, 101);
        const bool changed = GalaxyPenEngineProcess(&engine, reinterpret_cast<unsigned char*>(contact.data()),
            contact.size(), engine.Generation, 102) != 0;
        ULONG x = 0, y = 0, pressure = 0;
        USAGE usages[16]{};
        ULONG usageCount = static_cast<ULONG>(std::size(usages));
        bool correct = pressureWritten && changed &&
            HidP_GetUsageValue(HidP_Input, 0x01, 0, 0x30, &x, preparsed, contact.data(), reportSize) == HIDP_STATUS_SUCCESS &&
            HidP_GetUsageValue(HidP_Input, 0x01, 0, 0x31, &y, preparsed, contact.data(), reportSize) == HIDP_STATUS_SUCCESS &&
            HidP_GetUsageValue(HidP_Input, 0x0D, 0, 0x30, &pressure, preparsed, contact.data(), reportSize) == HIDP_STATUS_SUCCESS &&
            HidP_GetUsages(HidP_Input, 0x0D, 0, usages, &usageCount, preparsed, contact.data(), reportSize) == HIDP_STATUS_SUCCESS;
        const auto active = [&](USAGE usage) { return std::find(usages, usages + usageCount, usage) != usages + usageCount; };
        correct = correct && x == 15000 && y == 8000 && pressure == 632 && active(0x42) && !active(0x3C) && !active(0x45);
        for (size_t i = 0; i < contact.size(); ++i) {
            if (i != 1 && i != 6 && i != 7 && contact[i] != before[i]) correct = false;
        }
        std::wcout << L"  Motor COL01 (ponta + pressão + posição preservada): " << (correct ? L"OK" : L"FALHOU") << L"\n";
        success = success && correct;
    }
    HidD_FreePreparsedData(preparsed);
    return success;
}

void PrintInputCapabilities(PHIDP_PREPARSED_DATA preparsed, const HIDP_CAPS& caps) {
    USHORT valueCount = caps.NumberInputValueCaps;
    std::vector<HIDP_VALUE_CAPS> values(valueCount);
    if (valueCount != 0 && HidP_GetValueCaps(HidP_Input, values.data(), &valueCount, preparsed) == HIDP_STATUS_SUCCESS) {
        for (USHORT index = 0; index < valueCount; ++index) {
            const auto& value = values[index];
            std::wcout << L"  Input value " << index << L": report=0x" << std::hex
                       << static_cast<unsigned>(value.ReportID) << L", page=0x" << value.UsagePage
                       << L", usage=0x" << (value.IsRange ? value.Range.UsageMin : value.NotRange.Usage)
                       << std::dec;
            if (value.IsRange) std::wcout << L"..0x" << std::hex << value.Range.UsageMax << std::dec;
            std::wcout << L", logical=" << value.LogicalMin << L".." << value.LogicalMax
                       << L", physical=" << value.PhysicalMin << L".." << value.PhysicalMax
                       << L", bits=" << value.BitSize << L", count=" << value.ReportCount
                       << L", link=" << value.LinkCollection << L"\n";
        }
    }

    USHORT buttonCount = caps.NumberInputButtonCaps;
    std::vector<HIDP_BUTTON_CAPS> buttons(buttonCount);
    if (buttonCount != 0 && HidP_GetButtonCaps(HidP_Input, buttons.data(), &buttonCount, preparsed) == HIDP_STATUS_SUCCESS) {
        for (USHORT index = 0; index < buttonCount; ++index) {
            const auto& button = buttons[index];
            std::wcout << L"  Input button " << index << L": report=0x" << std::hex
                       << static_cast<unsigned>(button.ReportID) << L", page=0x" << button.UsagePage
                       << L", usage=0x" << (button.IsRange ? button.Range.UsageMin : button.NotRange.Usage)
                       << std::dec;
            if (button.IsRange) std::wcout << L"..0x" << std::hex << button.Range.UsageMax << std::dec;
            std::wcout << L", link=" << button.LinkCollection << L"\n";
        }
    }
}

void PrintDevice(
    DWORD index,
    const std::wstring& path,
    const std::wstring& description,
    HANDLE device,
    bool detailed) {
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
        if (detailed) PrintInputCapabilities(preparsed, caps);
    } else {
        std::wcout << L"  HidP_GetCaps falhou: 0x"
                   << std::hex << static_cast<unsigned long>(status) << std::dec << L"\n";
    }

    HidD_FreePreparsedData(preparsed);
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    SetConsoleOutputCP(CP_UTF8);
    const bool reportLab = argc == 2 && std::wstring_view(argv[1]) == L"--wcom-pen-report-lab";
    const bool flagLab = argc == 2 && std::wstring_view(argv[1]) == L"--wcom-flag-lab";
    const bool col01Lab = argc == 2 && std::wstring_view(argv[1]) == L"--wcom-col01-report-lab";
    const bool detailedPen = reportLab || flagLab || col01Lab || (argc == 2 && std::wstring_view(argv[1]) == L"--wcom-pen-caps");
    if (argc != 1 && !detailedPen) {
        std::wcerr << L"Uso: GalaxyPenHidScanner.exe [--wcom-pen-caps | --wcom-pen-report-lab | --wcom-col01-report-lab | --wcom-flag-lab]\n";
        return 2;
    }

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

        if (detailedPen) {
            std::wstring path = detail->DevicePath;
            std::transform(path.begin(), path.end(), path.begin(), [](wchar_t ch) { return std::towlower(ch); });
            if (reportLab && path.find(L"wcom016c&col04") == std::wstring::npos) continue;
            if ((flagLab || col01Lab) && path.find(L"wcom016c&col01") == std::wstring::npos) continue;
            if (!reportLab && path.find(L"wcom016c&col01") == std::wstring::npos &&
                path.find(L"wcom016c&col04") == std::wstring::npos) continue;
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

        if (flagLab) {
            std::wcout << L"COL01: " << description << L"\n";
        } else {
            PrintDevice(index, detail->DevicePath, description, handle, detailedPen);
        }
        const bool labPassed = flagLab ? RunWcomFlagLab(handle) :
            (col01Lab ? RunPenReportLab(handle, 0x02) : (!reportLab || RunPenReportLab(handle)));
        CloseHandle(handle);
        ++found;
        if (!labPassed) {
            SetupDiDestroyDeviceInfoList(devices);
            return 1;
        }
    }

    SetupDiDestroyDeviceInfoList(devices);

    std::wcout << L"\n============================================================\n";
    std::wcout << L"HIDs abertos e analisados: " << found << L"\n";
    if (reportLab) std::wcout << L"Laboratório em memória da coleção WCOM016C&COL04.\n";
    else if (flagLab || col01Lab) std::wcout << L"Laboratório em memória da coleção WCOM016C&COL01.\n";
    else if (detailedPen) std::wcout << L"Consulta somente leitura às coleções WCOM016C&COL01 e COL04.\n";
    else std::wcout << L"Procure especialmente por entradas marcadas como CANDIDATO DIGITIZER.\n";

    return detailedPen && found == 0 ? 1 : 0;
}
