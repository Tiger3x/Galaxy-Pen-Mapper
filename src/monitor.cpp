#define _WIN32_WINNT 0x0A00

#include <windows.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int IDC_START_CAPTURE = 1001;
constexpr int IDC_STOP_CAPTURE = 1002;
constexpr int IDC_CLEAR_STATUS = 1003;

std::ofstream g_log;
std::string g_logFilename;
bool g_captureEnabled = false;
bool g_penInsideCaptureArea = false;
unsigned long long g_eventCount = 0;

std::wstring g_status = L"Pronto. Pressione Iniciar captura.";
std::wstring g_pointerFlags = L"-";
std::wstring g_penFlags = L"-";
std::wstring g_latestRaw = L"-";
UINT32 g_pressure = 0;
INT32 g_tiltX = 0;
INT32 g_tiltY = 0;
UINT32 g_rotation = 0;
POINT g_penPoint{};

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
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr);

    if (size <= 0) return {};

    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        out.data(),
        size,
        nullptr,
        nullptr);

    return out;
}

std::wstring WidenAscii(const std::string& value) {
    return std::wstring(value.begin(), value.end());
}

std::wstring RawDeviceName(HANDLE device) {
    UINT chars = 0;
    GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, nullptr, &chars);
    if (chars == 0) return L"";

    std::vector<wchar_t> buffer(static_cast<size_t>(chars) + 1, L'\0');
    if (GetRawInputDeviceInfoW(
            device,
            RIDI_DEVICENAME,
            buffer.data(),
            &chars) == static_cast<UINT>(-1)) {
        return L"";
    }

    return buffer.data();
}

RID_DEVICE_INFO RawDeviceInfo(HANDLE device) {
    RID_DEVICE_INFO info{};
    info.cbSize = sizeof(info);
    UINT size = sizeof(info);

    if (GetRawInputDeviceInfoW(
            device,
            RIDI_DEVICEINFO,
            &info,
            &size) == static_cast<UINT>(-1)) {
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
    struct FlagName {
        UINT32 flag;
        const char* name;
    };

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

    return first ? "-" : out.str();
}

std::string PenFlagsText(PEN_FLAGS flags) {
    std::ostringstream out;
    bool first = true;

    const auto append = [&](const char* value) {
        if (!first) out << '|';
        out << value;
        first = false;
    };

    if ((flags & PEN_FLAG_BARREL) != 0) append("BARREL");
    if ((flags & PEN_FLAG_INVERTED) != 0) append("INVERTED");
    if ((flags & PEN_FLAG_ERASER) != 0) append("ERASER");

    return first ? "-" : out.str();
}

RECT CaptureRect(HWND hwnd) {
    RECT client{};
    GetClientRect(hwnd, &client);

    RECT area{};
    area.left = 32;
    area.top = 190;
    area.right = client.right - 32;
    area.bottom = client.bottom - 32;

    if (area.right < area.left + 100) area.right = area.left + 100;
    if (area.bottom < area.top + 100) area.bottom = area.top + 100;

    return area;
}

bool PointInCaptureArea(HWND hwnd, POINT screenPoint) {
    POINT clientPoint = screenPoint;
    ScreenToClient(hwnd, &clientPoint);
    const RECT area = CaptureRect(hwnd);
    return PtInRect(&area, clientPoint) != FALSE;
}

void WriteHeader() {
    g_log
        << "timestamp,event,device,vid,pid,usage_page,usage,report_index,raw_hex,"
        << "pointer_id,pointer_flags,pen_flags,pressure,tilt_x,tilt_y,rotation,x,y\n";
    g_log.flush();
}

void CloseLog() {
    if (g_log.is_open()) {
        g_log.flush();
        g_log.close();
    }
}

bool StartCapture(HWND hwnd) {
    CloseLog();

    g_logFilename = LogFilename();
    g_log.open(g_logFilename, std::ios::out | std::ios::trunc);

    if (!g_log) {
        MessageBoxW(
            hwnd,
            L"Não foi possível criar o arquivo CSV de captura.",
            L"Galaxy Pen Event Monitor",
            MB_OK | MB_ICONERROR);
        return false;
    }

    WriteHeader();
    g_eventCount = 0;
    g_captureEnabled = true;
    g_status = L"GRAVANDO. Somente eventos dentro da área de teste serão salvos.";

    SetFocus(hwnd);
    InvalidateRect(hwnd, nullptr, TRUE);
    return true;
}

void StopCapture(HWND hwnd, bool showMessage) {
    const bool wasCapturing = g_captureEnabled;
    g_captureEnabled = false;
    CloseLog();

    if (wasCapturing) {
        g_status = L"Captura parada. O CSV foi finalizado.";

        if (showMessage) {
            std::wstring message =
                L"Captura concluída.\n\nEventos gravados: " +
                std::to_wstring(g_eventCount) +
                L"\nArquivo: " +
                WidenAscii(g_logFilename);

            MessageBoxW(
                hwnd,
                message.c_str(),
                L"Galaxy Pen Event Monitor",
                MB_OK | MB_ICONINFORMATION);
        }
    }

    InvalidateRect(hwnd, nullptr, TRUE);
}

void LogRawInput(HRAWINPUT handle, HWND hwnd) {
    if (!g_captureEnabled ||
        !g_penInsideCaptureArea ||
        GetForegroundWindow() != hwnd ||
        !g_log.is_open()) {
        return;
    }

    UINT size = 0;
    if (GetRawInputData(
            handle,
            RID_INPUT,
            nullptr,
            &size,
            sizeof(RAWINPUTHEADER)) != 0 ||
        size == 0) {
        return;
    }

    std::vector<BYTE> buffer(size);
    if (GetRawInputData(
            handle,
            RID_INPUT,
            buffer.data(),
            &size,
            sizeof(RAWINPUTHEADER)) != size) {
        return;
    }

    const RAWINPUT* raw = reinterpret_cast<const RAWINPUT*>(buffer.data());
    if (raw->header.dwType != RIM_TYPEHID) return;

    const RID_DEVICE_INFO info = RawDeviceInfo(raw->header.hDevice);
    if (info.dwType != RIM_TYPEHID || info.hid.usUsagePage != 0x0D) return;

    const std::wstring deviceName = RawDeviceName(raw->header.hDevice);
    const RAWHID& hid = raw->data.hid;

    for (DWORD report = 0; report < hid.dwCount; ++report) {
        const BYTE* bytes =
            hid.bRawData + (static_cast<size_t>(report) * hid.dwSizeHid);
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
        g_latestRaw = WidenAscii(hex);
    }

    g_log.flush();
    InvalidateRect(hwnd, nullptr, TRUE);
}

void UpdatePenState(const POINTER_PEN_INFO& pen) {
    g_pressure =
        (pen.penMask & PEN_MASK_PRESSURE) != 0 ? pen.pressure : 0;
    g_tiltX =
        (pen.penMask & PEN_MASK_TILT_X) != 0 ? pen.tiltX : 0;
    g_tiltY =
        (pen.penMask & PEN_MASK_TILT_Y) != 0 ? pen.tiltY : 0;
    g_rotation =
        (pen.penMask & PEN_MASK_ROTATION) != 0 ? pen.rotation : 0;

    g_pointerFlags = WidenAscii(
        PointerFlagsText(pen.pointerInfo.pointerFlags));
    g_penFlags = WidenAscii(PenFlagsText(pen.penFlags));
    g_penPoint = pen.pointerInfo.ptPixelLocation;
}

void HandlePenPointer(UINT message, WPARAM wParam, HWND hwnd) {
    const UINT32 pointerId = GET_POINTERID_WPARAM(wParam);

    POINTER_INPUT_TYPE type{};
    if (!GetPointerType(pointerId, &type) || type != PT_PEN) return;

    POINTER_PEN_INFO pen{};
    if (!GetPointerPenInfo(pointerId, &pen)) return;

    const bool wasInside = g_penInsideCaptureArea;
    const bool isInside =
        message != WM_POINTERLEAVE &&
        PointInCaptureArea(hwnd, pen.pointerInfo.ptPixelLocation);

    g_penInsideCaptureArea = isInside;
    UpdatePenState(pen);

    if (isInside) {
        g_status = g_captureEnabled
            ? L"GRAVANDO — caneta dentro da área de teste."
            : L"Caneta detectada na área. Pressione Iniciar captura para gravar.";
    } else if (message == WM_POINTERLEAVE || wasInside) {
        g_status = g_captureEnabled
            ? L"GRAVANDO pausado — mova a caneta para dentro da área de teste."
            : L"Caneta fora da área de teste.";
    }

    if (!g_captureEnabled || !g_log.is_open()) {
        InvalidateRect(hwnd, nullptr, TRUE);
        return;
    }

    const bool shouldLogLeave =
        message == WM_POINTERLEAVE && wasInside;

    if (!isInside && !shouldLogLeave) {
        InvalidateRect(hwnd, nullptr, TRUE);
        return;
    }

    const char* eventName = "POINTER";
    if (message == WM_POINTERDOWN) eventName = "POINTER_DOWN";
    else if (message == WM_POINTERUP) eventName = "POINTER_UP";
    else if (message == WM_POINTERUPDATE) eventName = "POINTER_UPDATE";
    else if (message == WM_POINTERENTER) eventName = "POINTER_ENTER";
    else if (message == WM_POINTERLEAVE) eventName = "POINTER_LEAVE";

    const std::string pointerFlags =
        PointerFlagsText(pen.pointerInfo.pointerFlags);
    const std::string penFlags = PenFlagsText(pen.penFlags);

    g_log << Csv(Timestamp()) << ',' << eventName
          << ",\"WINDOWS_POINTER\",,,,,,,"
          << pointerId << ','
          << Csv(pointerFlags) << ','
          << Csv(penFlags) << ','
          << g_pressure << ','
          << g_tiltX << ','
          << g_tiltY << ','
          << g_rotation << ','
          << pen.pointerInfo.ptPixelLocation.x << ','
          << pen.pointerInfo.ptPixelLocation.y << '\n';

    g_log.flush();
    ++g_eventCount;
    InvalidateRect(hwnd, nullptr, TRUE);
}

bool RegisterDigitizerRawInput(HWND hwnd) {
    RAWINPUTDEVICE device{};
    device.usUsagePage = 0x0D;
    device.usUsage = 0;
    device.dwFlags = RIDEV_PAGEONLY | RIDEV_DEVNOTIFY;
    device.hwndTarget = hwnd;

    return RegisterRawInputDevices(
               &device,
               1,
               sizeof(device)) != FALSE;
}

void SetDefaultFont(HWND control) {
    SendMessageW(
        control,
        WM_SETFONT,
        reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
        TRUE);
}

void CreateControls(HWND hwnd) {
    HWND start = CreateWindowExW(
        0,
        L"BUTTON",
        L"Iniciar captura",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        32,
        96,
        150,
        36,
        hwnd,
        reinterpret_cast<HMENU>(IDC_START_CAPTURE),
        GetModuleHandleW(nullptr),
        nullptr);

    HWND stop = CreateWindowExW(
        0,
        L"BUTTON",
        L"Parar",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        194,
        96,
        110,
        36,
        hwnd,
        reinterpret_cast<HMENU>(IDC_STOP_CAPTURE),
        GetModuleHandleW(nullptr),
        nullptr);

    HWND clear = CreateWindowExW(
        0,
        L"BUTTON",
        L"Limpar painel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        316,
        96,
        130,
        36,
        hwnd,
        reinterpret_cast<HMENU>(IDC_CLEAR_STATUS),
        GetModuleHandleW(nullptr),
        nullptr);

    SetDefaultFont(start);
    SetDefaultFont(stop);
    SetDefaultFont(clear);
}

void DrawLabelValue(
    HDC dc,
    int x,
    int y,
    const wchar_t* label,
    const std::wstring& value) {

    std::wstring text = std::wstring(label) + L": " + value;
    TextOutW(
        dc,
        x,
        y,
        text.c_str(),
        static_cast<int>(text.size()));
}

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {

    switch (message) {
        case WM_CREATE:
            CreateControls(hwnd);

            if (!RegisterDigitizerRawInput(hwnd)) {
                g_status =
                    L"Raw Input não pôde ser registrado. "
                    L"WM_POINTER continuará disponível.";
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_START_CAPTURE:
                    StartCapture(hwnd);
                    return 0;

                case IDC_STOP_CAPTURE:
                    StopCapture(hwnd, true);
                    return 0;

                case IDC_CLEAR_STATUS:
                    g_eventCount = 0;
                    g_pointerFlags = L"-";
                    g_penFlags = L"-";
                    g_latestRaw = L"-";
                    g_pressure = 0;
                    g_tiltX = 0;
                    g_tiltY = 0;
                    g_rotation = 0;
                    g_status = g_captureEnabled
                        ? L"GRAVANDO. Painel limpo."
                        : L"Painel limpo. Pressione Iniciar captura.";
                    InvalidateRect(hwnd, nullptr, TRUE);
                    return 0;

                default:
                    break;
            }
            break;

        case WM_INPUT:
            LogRawInput(
                reinterpret_cast<HRAWINPUT>(lParam),
                hwnd);
            return DefWindowProcW(hwnd, message, wParam, lParam);

        case WM_INPUT_DEVICE_CHANGE:
            g_status = wParam == GIDC_ARRIVAL
                ? L"Dispositivo de entrada conectado."
                : L"Dispositivo de entrada removido.";
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;

        case WM_POINTERDOWN:
        case WM_POINTERUP:
        case WM_POINTERUPDATE:
        case WM_POINTERENTER:
        case WM_POINTERLEAVE:
            HandlePenPointer(message, wParam, hwnd);
            return DefWindowProcW(hwnd, message, wParam, lParam);

        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE && g_captureEnabled) {
                g_status =
                    L"GRAVANDO pausado — a janela precisa estar ativa.";
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);

            SetBkMode(dc, TRANSPARENT);
            SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));

            TextOutW(
                dc,
                32,
                24,
                L"Galaxy Pen Event Monitor — P002.1",
                34);

            TextOutW(
                dc,
                32,
                52,
                L"Somente a área de teste abaixo é registrada.",
                44);

            const RECT area = CaptureRect(hwnd);

            HBRUSH background = CreateSolidBrush(
                g_captureEnabled ? RGB(245, 252, 245) : RGB(248, 248, 248));
            FillRect(dc, &area, background);
            DeleteObject(background);

            HPEN border = CreatePen(
                PS_SOLID,
                2,
                g_captureEnabled ? RGB(40, 130, 70) : RGB(120, 120, 120));
            HGDIOBJ oldPen = SelectObject(dc, border);
            HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
            Rectangle(dc, area.left, area.top, area.right, area.bottom);
            SelectObject(dc, oldBrush);
            SelectObject(dc, oldPen);
            DeleteObject(border);

            TextOutW(
                dc,
                area.left + 16,
                area.top + 14,
                L"ÁREA DE TESTE DA CANETA",
                23);

            DrawLabelValue(
                dc,
                area.left + 16,
                area.top + 48,
                L"Status",
                g_status);

            DrawLabelValue(
                dc,
                area.left + 16,
                area.top + 78,
                L"Pointer",
                g_pointerFlags);

            DrawLabelValue(
                dc,
                area.left + 16,
                area.top + 108,
                L"Pen flags",
                g_penFlags);

            DrawLabelValue(
                dc,
                area.left + 16,
                area.top + 138,
                L"Pressão",
                std::to_wstring(g_pressure));

            DrawLabelValue(
                dc,
                area.left + 220,
                area.top + 138,
                L"Tilt X/Y",
                std::to_wstring(g_tiltX) +
                    L" / " +
                    std::to_wstring(g_tiltY));

            DrawLabelValue(
                dc,
                area.left + 430,
                area.top + 138,
                L"Eventos",
                std::to_wstring(g_eventCount));

            std::wstring rawPreview = g_latestRaw;
            if (rawPreview.size() > 110) {
                rawPreview.resize(110);
                rawPreview += L"...";
            }

            DrawLabelValue(
                dc,
                area.left + 16,
                area.top + 168,
                L"Último HID RAW",
                rawPreview);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_CLOSE:
            if (g_captureEnabled) {
                StopCapture(hwnd, false);
            }
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            CloseLog();
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand) {

    const wchar_t* className =
        L"GalaxyPenEventMonitorWindow";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    wc.hbrBackground =
        reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(
            nullptr,
            L"Não foi possível registrar a janela do monitor.",
            L"Galaxy Pen Event Monitor",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0,
        className,
        L"Galaxy Pen Event Monitor - P002.1",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1000,
        720,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!hwnd) {
        MessageBoxW(
            nullptr,
            L"Não foi possível criar a janela do monitor.",
            L"Galaxy Pen Event Monitor",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
