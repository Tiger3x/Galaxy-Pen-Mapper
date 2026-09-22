#define _WIN32_WINNT 0x0A00

#include <windows.h>
#include <hidusage.h>

#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::ofstream g_log;
std::wstring g_status = L"Aguardando eventos da caneta...";
unsigned long long g_eventCount = 0;

std::string Timestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t tt = system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &tt);

    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return out.str();
}

std::string LogFilename() {
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &now);

    std::ostringstream out;
    out << "pen-events-" << std::put_time(&tm, "%Y%m%d-%H%M%S") << ".csv";
    return out.str();
}

std::string Csv(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (char ch : value) {
        if (ch == '"') escaped.push_back('"');
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

std::string Narrow(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
        nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};

    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
        out.data(), size, nullptr, nullptr);
    return out;
}

std::wstring RawDeviceName(HANDLE device) {
    UINT chars = 0;
    GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, nullptr, &chars);
    if (chars == 0) return L"";

    std::vector<wchar_t> buffer(static_cast<size_t>(chars) + 1, L'\0');
    if (GetRawInputDeviceInfoW(
            device, RIDI_DEVICENAME, buffer.data(), &chars) == static_cast<UINT>(-1)) {
        return L"";
    }
    return buffer.data();
}

RID_DEVICE_INFO RawDeviceInfo(HANDLE device) {
    RID_DEVICE_INFO info{};
    info.cbSize = sizeof(info);
    UINT size = sizeof(info);
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICEINFO, &info, &size) == static_cast<UINT>(-1)) {
        info.dwType = static_cast<DWORD>(-1);
    }
    return info;
}

std::string HexBytes(const BYTE* data, size_t size) {
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0');
    for (size_t i = 0; i < size; ++i) {
        if (i) out << ' ';
        out << std::setw(2) << static_cast<unsigned>(data[i]);
    }
    return out.str();
}

std::string PointerFlagsText(UINT32 flags) {
    struct FlagName { UINT32 flag; const char* name; };
    static constexpr FlagName names[] = {
        {POINTER_FLAG_NEW, "NEW"},
        {POINTER_FLAG_INRANGE, "INRANGE"},
        {POINTER_FLAG_INCONTACT, "INCONTACT"},
        {POINTER_FLAG_FIRSTBUTTON, "FIRSTBUTTON"},
        {POINTER_FLAG_SECONDBUTTON, "SECONDBUTTON"},
        {POINTER_FLAG_THIRDBUTTON, "THIRDBUTTON"},
        {POINTER_FLAG_PRIMARY, "PRIMARY"},
        {POINTER_FLAG_CONFIDENCE, "CONFIDENCE"},
        {POINTER_FLAG_CANCELED, "CANCELED"},
        {POINTER_FLAG_DOWN, "DOWN"},
        {POINTER_FLAG_UPDATE, "UPDATE"},
        {POINTER_FLAG_UP, "UP"}
    };

    std::ostringstream out;
    bool first = true;
    for (const auto& item : names) {
        if ((flags & item.flag) != 0) {
            if (!first) out << '|';
            out << item.name;
            first = false;
        }
    }
    return out.str();
}

std::string PenFlagsText(PEN_FLAGS flags) {
    std::ostringstream out;
    bool first = true;
    auto append = [&](const char* value) {
        if (!first) out << '|';
        out << value;
        first = false;
    };

    if ((flags & PEN_FLAG_BARREL) != 0) append("BARREL");
    if ((flags & PEN_FLAG_INVERTED) != 0) append("INVERTED");
    if ((flags & PEN_FLAG_ERASER) != 0) append("ERASER");
    return out.str();
}

void WriteHeader() {
    g_log
        << "timestamp,event,device,vid,pid,usage_page,usage,report_index,raw_hex,"
        << "pointer_id,pointer_flags,pen_flags,pressure,tilt_x,tilt_y,rotation,x,y\n";
    g_log.flush();
}

void LogRawInput(HRAWINPUT handle) {
    UINT size = 0;
    if (GetRawInputData(handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 ||
        size == 0) {
        return;
    }

    std::vector<BYTE> buffer(size);
    if (GetRawInputData(
            handle, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size) {
        return;
    }

    const RAWINPUT* raw = reinterpret_cast<const RAWINPUT*>(buffer.data());
    if (raw->header.dwType != RIM_TYPEHID) return;

    const RID_DEVICE_INFO info = RawDeviceInfo(raw->header.hDevice);
    if (info.dwType != RIM_TYPEHID || info.hid.usUsagePage != 0x0D) return;

    const std::wstring deviceName = RawDeviceName(raw->header.hDevice);
    const RAWHID& hid = raw->data.hid;

    for (DWORD report = 0; report < hid.dwCount; ++report) {
        const BYTE* bytes = hid.bRawData + (static_cast<size_t>(report) * hid.dwSizeHid);
        const std::string hex = HexBytes(bytes, hid.dwSizeHid);

        g_log << Csv(Timestamp()) << ",RAW_HID,"
              << Csv(Narrow(deviceName)) << ','
              << info.hid.dwVendorId << ','
              << info.hid.dwProductId << ','
              << info.hid.usUsagePage << ','
              << info.hid.usUsage << ','
              << report << ','
              << Csv(hex)
              << ",,,,,,,,,\n";

        ++g_eventCount;

        std::wostringstream status;
        status << L"RAW HID  |  UsagePage 0x"
               << std::hex << std::uppercase << info.hid.usUsagePage
               << L" Usage 0x" << info.hid.usUsage
               << std::dec << L"  |  " << hid.dwSizeHid << L" bytes"
               << L"  |  eventos: " << g_eventCount;
        g_status = status.str();
    }

    g_log.flush();
}

void LogPenPointer(UINT message, WPARAM wParam, HWND hwnd) {
    const UINT32 pointerId = GET_POINTERID_WPARAM(wParam);

    POINTER_INPUT_TYPE type{};
    if (!GetPointerType(pointerId, &type) || type != PT_PEN) return;

    POINTER_PEN_INFO pen{};
    if (!GetPointerPenInfo(pointerId, &pen)) return;

    const char* eventName = "POINTER";
    if (message == WM_POINTERDOWN) eventName = "POINTER_DOWN";
    else if (message == WM_POINTERUP) eventName = "POINTER_UP";
    else if (message == WM_POINTERUPDATE) eventName = "POINTER_UPDATE";
    else if (message == WM_POINTERENTER) eventName = "POINTER_ENTER";
    else if (message == WM_POINTERLEAVE) eventName = "POINTER_LEAVE";

    const auto pointerFlags = PointerFlagsText(pen.pointerInfo.pointerFlags);
    const auto penFlags = PenFlagsText(pen.penFlags);

    const UINT32 pressure =
        (pen.penMask & PEN_MASK_PRESSURE) != 0 ? pen.pressure : 0;
    const INT32 tiltX =
        (pen.penMask & PEN_MASK_TILT_X) != 0 ? pen.tiltX : 0;
    const INT32 tiltY =
        (pen.penMask & PEN_MASK_TILT_Y) != 0 ? pen.tiltY : 0;
    const UINT32 rotation =
        (pen.penMask & PEN_MASK_ROTATION) != 0 ? pen.rotation : 0;

    g_log << Csv(Timestamp()) << ',' << eventName
          << ",\"WINDOWS_POINTER\",,,,,,,"
          << pointerId << ','
          << Csv(pointerFlags) << ','
          << Csv(penFlags) << ','
          << pressure << ','
          << tiltX << ','
          << tiltY << ','
          << rotation << ','
          << pen.pointerInfo.ptPixelLocation.x << ','
          << pen.pointerInfo.ptPixelLocation.y << '\n';
    g_log.flush();

    ++g_eventCount;

    const std::string eventText(eventName);
    const std::wstring eventWide(eventText.begin(), eventText.end());

    std::wostringstream status;
    status << L"CANETA  |  " << eventWide
           << L"  |  pressão " << pressure
           << L"  |  tilt " << tiltX << L"," << tiltY
           << L"  |  eventos: " << g_eventCount;
    if (!penFlags.empty()) {
        status << L"  |  " << std::wstring(penFlags.begin(), penFlags.end());
    }
    g_status = status.str();

    InvalidateRect(hwnd, nullptr, TRUE);
}

bool RegisterDigitizerRawInput(HWND hwnd) {
    RAWINPUTDEVICE device{};
    device.usUsagePage = 0x0D;
    device.usUsage = 0;
    device.dwFlags = RIDEV_PAGEONLY | RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    device.hwndTarget = hwnd;

    if (!RegisterRawInputDevices(&device, 1, sizeof(device))) {
        std::wcerr << L"RegisterRawInputDevices falhou: " << GetLastError() << L"\n";
        return false;
    }
    return true;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            if (RegisterDigitizerRawInput(hwnd)) {
                g_status = L"Monitor ativo. Aproxime a S Pen ou PW500 e desenhe nesta janela.";
            } else {
                g_status = L"Raw Input indisponível; WM_POINTER continuará sendo monitorado.";
            }
            return 0;

        case WM_INPUT:
            LogRawInput(reinterpret_cast<HRAWINPUT>(lParam));
            InvalidateRect(hwnd, nullptr, TRUE);
            return DefWindowProcW(hwnd, message, wParam, lParam);

        case WM_INPUT_DEVICE_CHANGE:
            g_status = wParam == GIDC_ARRIVAL
                ? L"Dispositivo Raw Input conectado."
                : L"Dispositivo Raw Input removido.";
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;

        case WM_POINTERDOWN:
        case WM_POINTERUP:
        case WM_POINTERUPDATE:
        case WM_POINTERENTER:
        case WM_POINTERLEAVE:
            LogPenPointer(message, wParam, hwnd);
            return DefWindowProcW(hwnd, message, wParam, lParam);

        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT rect{};
            GetClientRect(hwnd, &rect);

            SetBkMode(dc, TRANSPARENT);

            RECT titleRect = rect;
            titleRect.left += 24;
            titleRect.top += 24;
            DrawTextW(
                dc,
                L"Galaxy Pen Event Monitor — P002",
                -1,
                &titleRect,
                DT_LEFT | DT_TOP | DT_SINGLELINE);

            RECT infoRect = rect;
            infoRect.left += 24;
            infoRect.right -= 24;
            infoRect.top += 70;
            const wchar_t* instructions =
                L"Use a caneta dentro desta janela.\n\n"
                L"Teste hover, toque, pressão, botão inferior, botão superior e traços.\n"
                L"O programa grava WM_POINTER interpretado pelo Windows e HID bruto quando disponível.\n"
                L"Feche a janela para finalizar o arquivo CSV.";
            DrawTextW(dc, instructions, -1, &infoRect, DT_LEFT | DT_TOP | DT_WORDBREAK);

            RECT statusRect = rect;
            statusRect.left += 24;
            statusRect.right -= 24;
            statusRect.top += 190;
            DrawTextW(dc, g_status.c_str(), -1, &statusRect, DT_LEFT | DT_TOP | DT_WORDBREAK);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

} // namespace

int wmain() {
    SetConsoleOutputCP(CP_UTF8);

    const std::string filename = LogFilename();
    g_log.open(filename, std::ios::out | std::ios::trunc);
    if (!g_log) {
        std::cerr << "Não foi possível criar o arquivo de log.\n";
        return 1;
    }
    WriteHeader();

    std::cout << "Galaxy Pen Event Monitor - P002\n";
    std::cout << "Log: " << filename << "\n";
    std::cout << "Feche a janela do monitor para encerrar.\n";

    const wchar_t* className = L"GalaxyPenEventMonitorWindow";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = className;
    wc.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        std::wcerr << L"RegisterClassW falhou: " << GetLastError() << L"\n";
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0,
        className,
        L"Galaxy Pen Event Monitor - P002",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        960,
        640,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);

    if (!hwnd) {
        std::wcerr << L"CreateWindowExW falhou: " << GetLastError() << L"\n";
        return 1;
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    g_log.flush();
    g_log.close();

    std::cout << "Captura finalizada. Eventos registrados: " << g_eventCount << "\n";
    std::cout << "Arquivo salvo em: " << filename << "\n";
    return 0;
}
