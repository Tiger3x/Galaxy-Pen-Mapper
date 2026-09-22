#include "MainWindow.h"

#include <vector>

namespace {

constexpr int kStartCapture = 1001;
constexpr int kStopCapture = 1002;
constexpr int kClearPanel = 1003;
constexpr UINT_PTR kRefreshTimer = 1;
constexpr UINT kRefreshIntervalMs = 16;
constexpr UINT_PTR kForegroundTimer = 2;
constexpr UINT kForegroundIntervalMs = 50;

void setDefaultFont(HWND control) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
}

const char* pointerEventName(UINT message) {
    switch (message) {
        case WM_POINTERDOWN: return "POINTER_DOWN";
        case WM_POINTERUP: return "POINTER_UP";
        case WM_POINTERUPDATE: return "POINTER_UPDATE";
        case WM_POINTERENTER: return "POINTER_ENTER";
        case WM_POINTERLEAVE: return "POINTER_LEAVE";
        default: return "POINTER";
    }
}

const char* windowInputEventName(UINT message) {
    switch (message) {
        case WM_LBUTTONDOWN: return "WINDOW_MOUSE_LEFT_DOWN";
        case WM_LBUTTONUP: return "WINDOW_MOUSE_LEFT_UP";
        case WM_RBUTTONDOWN: return "WINDOW_MOUSE_RIGHT_DOWN";
        case WM_RBUTTONUP: return "WINDOW_MOUSE_RIGHT_UP";
        case WM_MBUTTONDOWN: return "WINDOW_MOUSE_MIDDLE_DOWN";
        case WM_MBUTTONUP: return "WINDOW_MOUSE_MIDDLE_UP";
        case WM_XBUTTONDOWN: return "WINDOW_MOUSE_X_DOWN";
        case WM_XBUTTONUP: return "WINDOW_MOUSE_X_UP";
        case WM_MOUSEWHEEL: return "WINDOW_MOUSE_WHEEL";
        case WM_MOUSEHWHEEL: return "WINDOW_MOUSE_HWHEEL";
        case WM_KEYDOWN: return "WINDOW_KEY_DOWN";
        case WM_KEYUP: return "WINDOW_KEY_UP";
        case WM_SYSKEYDOWN: return "WINDOW_SYSKEY_DOWN";
        case WM_SYSKEYUP: return "WINDOW_SYSKEY_UP";
        case WM_APPCOMMAND: return "WINDOW_APPCOMMAND";
        case WM_HOTKEY: return "WINDOW_HOTKEY";
        default: return "WINDOW_INPUT";
    }
}

} // namespace

bool MainWindow::create(HINSTANCE instance, int showCommand) {
    const wchar_t* className = L"GalaxyPenDiagnosticStudio";
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_CROSS);

    if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

    HWND window = CreateWindowExW(0, className, L"Galaxy Pen Diagnostic Studio — P002.5",
                                  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 1000, 720, nullptr, nullptr, instance, this);
    if (!window) return false;
    ShowWindow(window, showCommand);
    UpdateWindow(window);
    return true;
}

LRESULT CALLBACK MainWindow::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    return self ? self->handle(window, message, wParam, lParam) : DefWindowProcW(window, message, wParam, lParam);
}

LRESULT MainWindow::handle(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            createControls(window);
            penCapture.initialize(window);
            if (!rawHidCapture.initialize(window)) status = L"Raw HID não pôde ser registrado; WM_POINTER continua disponível.";
            SetTimer(window, kRefreshTimer, kRefreshIntervalMs, nullptr);
            SetTimer(window, kForegroundTimer, kForegroundIntervalMs, nullptr);
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case kStartCapture: startCapture(window); return 0;
                case kStopCapture: stopCapture(); scheduleRepaint(); return 0;
                case kClearPanel: clearPanel(); scheduleRepaint(); return 0;
                default: break;
            }
            break;
        case WM_INPUT: {
            const bool windowForeground = GetForegroundWindow() == window;
            const unsigned rawInputCode = GET_RAWINPUT_CODE_WPARAM(wParam);
            const auto input = rawHidCapture.process(reinterpret_cast<HRAWINPUT>(lParam));
            if (!input.hidReports.empty()) {
                latestRaw.assign(input.hidReports.back().bytes.begin(), input.hidReports.back().bytes.end());
                if (recording && sessionWriter.active()) {
                    for (const auto& report : input.hidReports) {
                        sessionWriter.writeRaw(report, windowForeground, rawInputCode);
                        ++eventCount;
                    }
                }
                scheduleRepaint();
            }
            if (recording && sessionWriter.active() && windowForeground && input.mouse && input.mouse->buttonFlags != 0) {
                sessionWriter.writeRawMouse(*input.mouse, windowForeground, rawInputCode);
                ++eventCount;
            }
            if (recording && sessionWriter.active() && windowForeground && input.keyboard) {
                sessionWriter.writeRawKeyboard(*input.keyboard, windowForeground, rawInputCode);
                ++eventCount;
            }
            return DefWindowProcW(window, message, wParam, lParam);
        }
        case WM_INPUT_DEVICE_CHANGE:
            status = wParam == GIDC_ARRIVAL ? L"Dispositivo de entrada conectado." : L"Dispositivo de entrada removido.";
            scheduleRepaint();
            return 0;
        case WM_POINTERDOWN:
        case WM_POINTERUP:
        case WM_POINTERUPDATE:
        case WM_POINTERENTER:
        case WM_POINTERLEAVE: {
            const bool wasInside = penInside;
            if (!penCapture.processPointer(message, wParam)) return DefWindowProcW(window, message, wParam, lParam);
            penInside = message != WM_POINTERLEAVE && panel.contains(window, penCapture.state().position);
            if (penInside) status = recording ? L"Gravando eventos da caneta dentro da área de teste." : L"Caneta detectada. Inicie uma captura para gravar.";
            else status = recording ? L"Caneta fora da área; mouse e teclado continuam sendo observados." : L"Caneta fora da área de teste.";
            if (canRecord(window) || (message == WM_POINTERLEAVE && wasInside && recording && sessionWriter.active())) {
                sessionWriter.writePointer(pointerEventName(message), penCapture.state());
                ++eventCount;
            }
            scheduleRepaint();
            return DefWindowProcW(window, message, wParam, lParam);
        }
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_APPCOMMAND:
        case WM_HOTKEY:
            recordWindowInput(window, windowInputEventName(message), wParam, lParam);
            break;
        case WM_ACTIVATE:
            if (recording && sessionWriter.active()) {
                sessionWriter.writeWindowState(LOWORD(wParam) == WA_INACTIVE ? "WINDOW_INACTIVE" : "WINDOW_ACTIVE", wParam, lParam);
                ++eventCount;
            }
            if (LOWORD(wParam) == WA_INACTIVE && recording) {
                status = L"Captura pausada: a janela precisa ficar em primeiro plano.";
                scheduleRepaint();
            }
            return 0;
        case WM_TIMER:
            if (wParam == kForegroundTimer) {
                recordForegroundSample(window);
                return 0;
            }
            if (wParam == kRefreshTimer && repaintPending) {
                repaintPending = false;
                InvalidateRect(window, nullptr, FALSE);
            }
            return 0;
        case WM_SIZE:
            scheduleRepaint();
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);

            RECT client{};
            GetClientRect(window, &client);
            const int width = client.right - client.left;
            const int height = client.bottom - client.top;
            HDC buffer = width > 0 && height > 0 ? CreateCompatibleDC(dc) : nullptr;
            HBITMAP bitmap = buffer ? CreateCompatibleBitmap(dc, width, height) : nullptr;

            if (buffer && bitmap) {
                HGDIOBJ previousBitmap = SelectObject(buffer, bitmap);
                panel.draw(buffer, window, penCapture.state(), eventCount, recording, penInside, status, latestRaw);
                BitBlt(dc, 0, 0, width, height, buffer, 0, 0, SRCCOPY);
                SelectObject(buffer, previousBitmap);
            } else {
                panel.draw(dc, window, penCapture.state(), eventCount, recording, penInside, status, latestRaw);
            }

            if (bitmap) DeleteObject(bitmap);
            if (buffer) DeleteDC(buffer);
            EndPaint(window, &paint);
            return 0;
        }
        case WM_CLOSE:
            stopCapture();
            DestroyWindow(window);
            return 0;
        case WM_DESTROY:
            KillTimer(window, kRefreshTimer);
            KillTimer(window, kForegroundTimer);
            sessionWriter.stop();
            PostQuitMessage(0);
            return 0;
        default: break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void MainWindow::createControls(HWND window) {
    CreateWindowW(L"STATIC", L"Rótulo da sessão", WS_CHILD | WS_VISIBLE, 32, 84, 150, 18, window, nullptr, nullptr, nullptr);
    sessionBox = CreateWindowExW(0, L"EDIT", L"S_Pen", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 32, 104, 210, 28, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Descrição", WS_CHILD | WS_VISIBLE, 260, 84, 150, 18, window, nullptr, nullptr, nullptr);
    descriptionBox = CreateWindowExW(0, L"EDIT", L"Teste controlado", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 260, 104, 250, 28, window, nullptr, nullptr, nullptr);
    HWND start = CreateWindowW(L"BUTTON", L"Iniciar captura", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 530, 103, 142, 30, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStartCapture)), nullptr, nullptr);
    HWND stop = CreateWindowW(L"BUTTON", L"Parar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 682, 103, 100, 30, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStopCapture)), nullptr, nullptr);
    HWND clear = CreateWindowW(L"BUTTON", L"Limpar painel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 792, 103, 140, 30, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kClearPanel)), nullptr, nullptr);
    for (HWND control : {sessionBox, descriptionBox, start, stop, clear}) setDefaultFont(control);
}

void MainWindow::startCapture(HWND window) {
    if (recording) status = L"Uma captura já está em andamento.";
    else if (sessionWriter.start(controlText(sessionBox), controlText(descriptionBox))) {
        recording = true;
        eventCount = 0;
        status = L"Gravando. O CSV será salvo na pasta captures.";
        SetFocus(window);
        recordForegroundSample(window);
    } else status = L"Não foi possível criar o CSV de captura.";
    scheduleRepaint();
}

void MainWindow::stopCapture() {
    if (!recording) return;
    sessionWriter.stop();
    recording = false;
    status = L"Captura finalizada. O CSV foi salvo com segurança.";
}

void MainWindow::clearPanel() {
    eventCount = 0;
    penInside = false;
    latestRaw.clear();
    penCapture.clear();
    status = recording ? L"Gravando. Painel limpo." : L"Painel limpo. Pronto para nova captura.";
}

void MainWindow::scheduleRepaint() {
    repaintPending = true;
}

void MainWindow::recordWindowInput(HWND window, const std::string& event, WPARAM wParam, LPARAM lParam) {
    if (!canRecordSystemInput(window)) return;
    INPUT_MESSAGE_SOURCE source{};
    GetCurrentInputMessageSource(&source);
    sessionWriter.writeWindowInput(event, wParam, lParam,
                                   static_cast<unsigned>(source.deviceType),
                                   static_cast<unsigned>(source.originId));
    ++eventCount;
}

std::wstring MainWindow::controlText(HWND control) const {
    const int length = GetWindowTextLengthW(control);
    std::vector<wchar_t> text(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(control, text.data(), length + 1);
    return text.data();
}

bool MainWindow::canRecord(HWND window) const {
    return recording && sessionWriter.active() && penInside && GetForegroundWindow() == window;
}

bool MainWindow::canRecordSystemInput(HWND window) const {
    return recording && sessionWriter.active() && GetForegroundWindow() == window;
}

void MainWindow::recordForegroundSample(HWND window) {
    if (!recording || !sessionWriter.active()) return;
    sessionWriter.writeForegroundSample(GetForegroundWindow() == window);
}
