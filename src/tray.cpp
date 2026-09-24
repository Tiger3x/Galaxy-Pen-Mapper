#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include "ui/TipTestPanel.h"
#include "../driver/shared/GalaxyPenClientState.h"

namespace {
#ifdef GALAXY_PEN_UI_CHECK
constexpr wchar_t kClassName[] = L"GalaxyPenMapperUiCheckV2";
constexpr wchar_t kMutexName[] = L"Local\\GalaxyPenMapperUiCheckV2";
#else
constexpr wchar_t kClassName[] = L"GalaxyPenMapperWindowV2";
constexpr wchar_t kMutexName[] = L"Local\\GalaxyPenMapperTrayV2";
#endif
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT_PTR kTimer = 1;
constexpr int kEnable = 101, kSensitivity = 102, kPressure = 103;
constexpr int kExit = 105, kTrayToggle = 106, kTrayShow = 107;
constexpr int kSaveTip = 108, kClearTip = 109, kMaintenance = 110;
constexpr int kFloor = 111;
constexpr UINT kTrayIconId = 1;
HWND gWindow{}, gEnable{}, gPressure{}, gSlider{}, gStatus{}, gDetails{}, gSensitivityLabel{};
HWND gTipPanel{}, gTelemetry{};
HWND gFloorSlider{}, gFloorLabel{};
HANDLE gDevice = INVALID_HANDLE_VALUE;
bool gEnabled = false, gInTray = false, gStatusValid = false;
bool gMaintenance = false;
unsigned gLastReportCount = 0;
ULONGLONG gLastReportTime = 0;
GALAXY_PEN_PROBE_STATS gStats{};
GALAXY_PEN_CONFIG gConfig{};
std::wstring gLastStatus, gLastDetails;
std::wstring gLastTelemetry;
std::wstring gLastSensitivity;
std::wstring gLastFloor;

void SetTextIfChanged(HWND control, const std::wstring& text, std::wstring& previous) {
    if (text != previous) { SetWindowTextW(control, text.c_str()); previous = text; }
}
bool CanEnable() { return gStatusValid && GalaxyPenCanEnable(&gStats, sizeof(gStats)); }
void SetControls() {
    EnableWindow(gEnable, CanEnable());
    const bool pressureAvailable = CanEnable() && (gStats.Capabilities & GALAXY_PEN_CAP_PRESSURE);
    EnableWindow(gPressure, pressureAvailable);
    EnableWindow(gSlider, pressureAvailable && SendMessageW(gPressure, BM_GETCHECK, 0, 0) == BST_CHECKED);
    EnableWindow(gFloorSlider, pressureAvailable && SendMessageW(gPressure, BM_GETCHECK, 0, 0) == BST_CHECKED);
    const auto checked = gEnabled ? BST_CHECKED : BST_UNCHECKED;
    if (SendMessageW(gEnable, BM_GETCHECK, 0, 0) != checked)
        SendMessageW(gEnable, BM_SETCHECK, checked, 0);
}
void Disconnect(const wchar_t* reason) {
    gEnabled = false;
    gStatusValid = false;
    if (gDevice != INVALID_HANDLE_VALUE) { CloseHandle(gDevice); gDevice = INVALID_HANDLE_VALUE; }
    SetControls();
    SetTextIfChanged(gStatus, reason, gLastStatus);
    SetTextIfChanged(gDetails, L"", gLastDetails);
    SetTextIfChanged(gTelemetry, L"Dados brutos indisponíveis.", gLastTelemetry);
}
void UpdateSensitivityLabel() {
    const auto position = SendMessageW(gSlider, TBM_GETPOS, 0, 0);
    const auto label = L"Sensibilidade: " + std::to_wstring(position * 10) + L"%";
    SetTextIfChanged(gSensitivityLabel, label, gLastSensitivity);
    const auto floor = SendMessageW(gFloorSlider, TBM_GETPOS, 0, 0);
    SetTextIfChanged(gFloorLabel, L"Pressão inicial: " + std::to_wstring(floor) + L"% (mínimo artificial)", gLastFloor);
}
bool RefreshDriver() {
    if (gMaintenance) return false;
#ifdef GALAXY_PEN_UI_CHECK
    // Test-only executable: no device is opened and no IOCTL is ever issued.
    gStats = {};
    gStats.Version = GALAXY_PEN_PROBE_PROTOCOL_VERSION;
    gStats.Capabilities = GALAXY_PEN_CAP_OBSERVE;
    gStats.AttachedDevices = gStats.ReadyForCorrection = 1;
    gStats.StartedReads = 10; gStats.CompletedReads = gStats.Report02Reads = 9;
    gStats.PendingReads = 1; gStats.LastBytes = 15; gStats.LastReportId = 2;
    gConfig = {GALAXY_PEN_PROTOCOL_VERSION, 0, GALAXY_PEN_DEFAULT_SENSITIVITY, 0, 0, 5};
#else
    if (gDevice == INVALID_HANDLE_VALUE) {
        gDevice = CreateFileW(GALAXY_PEN_CONTROL_PATH, GENERIC_READ | GENERIC_WRITE,
                              0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (gDevice == INVALID_HANDLE_VALUE) {
            Disconnect(L"Driver atualizado indisponível. Correção desligada.");
            return false;
        }
    }
    DWORD bytes = 0;
    if (!DeviceIoControl(gDevice, GALAXY_PEN_IOCTL_QUERY_PROBE_STATS,
                         nullptr, 0, &gStats, sizeof(gStats), &bytes, nullptr)) {
        Disconnect(L"Comunicação perdida. Correção desligada; tentando reconectar.");
        return false;
    }
    if (!GalaxyPenValidStats(&gStats, bytes)) {
        Disconnect(L"Versão incompatível do driver. Ativação bloqueada.");
        return false;
    }
    if (!DeviceIoControl(gDevice, GALAXY_PEN_IOCTL_QUERY_CONFIG,
                         nullptr, 0, &gConfig, sizeof(gConfig), &bytes, nullptr) ||
        !GalaxyPenValidConfig(&gConfig, bytes)) {
        Disconnect(L"Estado do driver inválido. Ativação bloqueada.");
        return false;
    }
#endif
    gStatusValid = true;
    gEnabled = gConfig.Enabled != 0;
    if (gEnabled && !CanEnable()) {
        Disconnect(L"Estado inconsistente do driver. Correção desligada.");
        return false;
    }
    const auto pressureChecked = gConfig.PressureEnabled ? BST_CHECKED : BST_UNCHECKED;
    if (SendMessageW(gPressure, BM_GETCHECK, 0, 0) != pressureChecked)
        SendMessageW(gPressure, BM_SETCHECK, pressureChecked, 0);
    if (SendMessageW(gSlider, TBM_GETPOS, 0, 0) != gConfig.SensitivityPermille / 100)
        SendMessageW(gSlider, TBM_SETPOS, TRUE, gConfig.SensitivityPermille / 100);
    if (SendMessageW(gFloorSlider, TBM_GETPOS, 0, 0) != gConfig.ContactFloorPercent)
        SendMessageW(gFloorSlider, TBM_SETPOS, TRUE, gConfig.ContactFloorPercent);
    UpdateSensitivityLabel();
    SetControls();
    std::wstring state;
    if ((gStats.Capabilities & GALAXY_PEN_CAP_TOOL) == 0)
        state = L"Somente diagnóstico — esta sonda não altera a caneta.";
    else if (!CanEnable())
        state = L"Aguardando leitura real da caneta para liberar o teste.";
    else if (!gEnabled)
        state = L"Leitura disponível. Correção experimental desligada.";
    else if (gConfig.StateFlags & GALAXY_PEN_STATE_WAIT_FOR_LIFT)
        state = L"Armado — levante a ponta para iniciar o modo de teste.";
    else
        state = gConfig.PressureEnabled
            ? L"Pressão experimental: mínimo artificial + variação disponível."
            : L"Teste de ponta ligado; pressão original. Confira no aplicativo.";
#ifdef GALAXY_PEN_UI_CHECK
    state = L"SIMULAÇÃO VISUAL — contadores fictícios. Ativação bloqueada.";
#endif
    SetTextIfChanged(gStatus, state, gLastStatus);
    wchar_t details[400]{};
    swprintf_s(details, L"Leituras: recebidas %u | concluídas %u | pendentes %u\r\n"
        L"Relatórios da caneta: %u | alterados: %u\r\n"
        L"Falhas de envio: %u | falhas de acesso: %u\r\n"
        L"Último: estado 0x%08X | %u bytes | ID 0x%02X",
        gStats.StartedReads, gStats.CompletedReads, gStats.PendingReads,
        gStats.Report02Reads, gStats.ModifiedReports, gStats.ForwardFailures,
        gStats.BufferFailures, gStats.LastStatus, gStats.LastBytes, gStats.LastReportId);
    SetTextIfChanged(gDetails, details, gLastDetails);
    if (gStats.Report02Reads != gLastReportCount) {
        gLastReportCount = gStats.Report02Reads; gLastReportTime = GetTickCount64();
    }
    std::wstring telemetry;
    if (gStats.SnapshotValid) {
        wchar_t values[260]{};
        swprintf_s(values, L"Último relatório do driver (escala 0–4095):\r\n"
            L"Estado: 0x%02X → 0x%02X | Pressão: %u → %u\r\n%s",
            gStats.RawFlags, gStats.OutputFlags, gStats.RawPressure, gStats.OutputPressure,
            GetTickCount64() - gLastReportTime > 2000 ? L"Sem nova leitura há mais de 2 segundos." : L"Entrada original → saída encaminhada");
        telemetry = values;
    } else telemetry = L"Sem amostra real do driver.";
    if (gStats.ContactReports) {
        telemetry += L"\r\nContato bruto: " + std::to_wstring(gStats.ContactRawMin) + L" a " + std::to_wstring(gStats.ContactRawMax) +
            L" | saturados: " + std::to_wstring(gStats.SaturatedContactReports) + L"/" + std::to_wstring(gStats.ContactReports);
        telemetry += gStats.ContactRawMin == gStats.ContactRawMax
            ? L"\r\nSem variação bruta neste intervalo. Não force a tela."
            : L"\r\nVariação bruta detectada; confira o efeito no traço.";
    }
    SetTextIfChanged(gTelemetry, telemetry, gLastTelemetry);
    return true;
}
bool SendConfig(bool enabled) {
#ifdef GALAXY_PEN_UI_CHECK
    (void)enabled;
    return false;
#else
    if (gDevice == INVALID_HANDLE_VALUE || !gStatusValid || (enabled && !CanEnable())) {
        SetControls();
        return false;
    }
    GALAXY_PEN_CONFIG config{};
    config.Version = GALAXY_PEN_PROTOCOL_VERSION;
    config.Enabled = enabled ? 1u : 0u;
    config.PressureEnabled = SendMessageW(gPressure, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1u : 0u;
    config.SensitivityPermille = static_cast<unsigned>(SendMessageW(gSlider, TBM_GETPOS, 0, 0)) * 100u;
    config.ContactFloorPercent = static_cast<unsigned>(SendMessageW(gFloorSlider, TBM_GETPOS, 0, 0));
    DWORD returned = 0;
    if (!DeviceIoControl(gDevice, GALAXY_PEN_IOCTL_SET_CONFIG, &config, sizeof(config),
                         nullptr, 0, &returned, nullptr)) {
        Disconnect(L"O driver recusou a alteração. Correção desligada.");
        return false;
    }
    return RefreshDriver();
#endif
}
void AddTrayIcon() {
    if (gInTray) return;
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data); data.hWnd = gWindow; data.uID = kTrayIconId;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = kTrayMessage; data.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(data.szTip, L"Galaxy Pen Mapper — experimental");
    if (Shell_NotifyIconW(NIM_ADD, &data)) {
        data.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &data);
        gInTray = true;
    }
}
void RemoveTrayIcon() {
    if (!gInTray) return;
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data); data.hWnd = gWindow; data.uID = kTrayIconId;
    Shell_NotifyIconW(NIM_DELETE, &data); gInTray = false;
}
void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | (gEnabled ? MF_CHECKED : MF_UNCHECKED) |
        (CanEnable() ? MF_ENABLED : MF_GRAYED), kTrayToggle,
        gEnabled ? L"Desativar correção" : L"Ativar correção experimental");
    AppendMenuW(menu, MF_STRING, kTrayShow, L"Abrir");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kExit, L"Fechar (desliga correção)");
    POINT point{}; GetCursorPos(&point); SetForegroundWindow(gWindow);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, gWindow, nullptr);
    DestroyMenu(menu);
}
void CreateControls(HWND window) {
    CreateWindowW(L"STATIC", L"Galaxy Pen Mapper 0.3.2 — PW500 na tela", WS_CHILD | WS_VISIBLE,
                  18, 14, 480, 24, window, nullptr, nullptr, nullptr);
    gEnable = CreateWindowW(L"BUTTON", L"Corrigir ponta / borracha", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        20, 48, 350, 24, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEnable)), nullptr, nullptr);
    gPressure = CreateWindowW(L"BUTTON", L"Inverter e ampliar pressão (experimental)", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        20, 78, 440, 24, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kPressure)), nullptr, nullptr);
    gSensitivityLabel = CreateWindowW(L"STATIC", L"Sensibilidade: 800%", WS_CHILD | WS_VISIBLE,
        20, 111, 400, 22, window, nullptr, nullptr, nullptr);
    gSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
        20, 135, 440, 36, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSensitivity)), nullptr, nullptr);
    SendMessageW(gSlider, TBM_SETRANGE, TRUE, MAKELONG(GALAXY_PEN_MIN_SENSITIVITY / 100, GALAXY_PEN_MAX_SENSITIVITY / 100));
    SendMessageW(gSlider, TBM_SETPOS, TRUE, GALAXY_PEN_DEFAULT_SENSITIVITY / 100);
    SendMessageW(gSlider, TBM_SETTICFREQ, 40, 0);
    gFloorLabel = CreateWindowW(L"STATIC", L"Pressão inicial: 5% (mínimo artificial)", WS_CHILD | WS_VISIBLE,
        20, 174, 475, 22, window, nullptr, nullptr, nullptr);
    gFloorSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
        20, 198, 440, 36, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kFloor)), nullptr, nullptr);
    SendMessageW(gFloorSlider, TBM_SETRANGE, TRUE, MAKELONG(1, 25));
    SendMessageW(gFloorSlider, TBM_SETPOS, TRUE, 5);
    SendMessageW(gFloorSlider, TBM_SETTICFREQ, 5, 0);
    gStatus = CreateWindowW(L"STATIC", L"Verificando driver...", WS_CHILD | WS_VISIBLE,
        20, 251, 480, 44, window, nullptr, nullptr, nullptr);
    gDetails = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        20, 300, 490, 90, window, nullptr, nullptr, nullptr);
    gTelemetry = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        20, 395, 485, 110, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Sensibilidade até 3200%. Comece em 1200%; reduza se engrossar cedo. O mínimo artificial não mede força. Contadores reiniciam ao mudar opções.",
        WS_CHILD | WS_VISIBLE, 20, 517, 475, 50, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Liberar driver para manutenção", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        20, 577, 440, 28, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMaintenance)), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Teste da ponta — entrada recebida pelo Windows", WS_CHILD | WS_VISIBLE,
        525, 14, 445, 24, window, nullptr, nullptr, nullptr);
    HWND tipStatus = CreateWindowW(L"STATIC", L"Aproxime e toque a caneta na área branca. Mouse e toque são ignorados.", WS_CHILD | WS_VISIBLE,
        525, 414, 445, 60, window, nullptr, nullptr, nullptr);
    gTipPanel = CreateTipTestPanel(window, 525, 48, 445, 350, tipStatus);
    CreateWindowW(L"STATIC", L"Azul: caneta. Laranja: borracha/invertida. A espessura segue a pressão recebida. Captura apenas nesta área; não altera o Paint.",
        WS_CHILD | WS_VISIBLE, 525, 484, 445, 52, window, nullptr, nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Salvar captura CSV", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        525, 552, 200, 30, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSaveTip)), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Limpar teste", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        742, 552, 180, 30, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kClearTip)), nullptr, nullptr);
    EnumChildWindows(window, [](HWND child, LPARAM) -> BOOL {
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE); return TRUE;
    }, 0);
    SetControls();
}
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateControls(window); SetTimer(window, kTimer, 500, nullptr); RefreshDriver(); return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case kEnable:
            if (CanEnable()) SendConfig(SendMessageW(gEnable, BM_GETCHECK, 0, 0) == BST_CHECKED);
            else SetControls();
            return 0;
        case kPressure:
            if (CanEnable()) SendConfig(gEnabled);
            return 0;
        case kTrayToggle:
            if (CanEnable()) SendConfig(!gEnabled);
            return 0;
        case kSaveTip: SaveTipTestPanel(gTipPanel); return 0;
        case kClearTip: ClearTipTestPanel(gTipPanel); return 0;
        case kMaintenance:
            gMaintenance = SendMessageW(GetDlgItem(window, kMaintenance), BM_GETCHECK, 0, 0) == BST_CHECKED;
            if (gMaintenance) Disconnect(L"Driver liberado. Correção desligada; reconexão suspensa.");
            else RefreshDriver();
            return 0;
        case kExit: SendMessageW(window, WM_CLOSE, 0, 0); return 0;
        case kTrayShow: RemoveTrayIcon(); ShowWindow(window, SW_RESTORE); return 0;
        }
        break;
    case WM_HSCROLL:
        if (reinterpret_cast<HWND>(lParam) == gSlider || reinterpret_cast<HWND>(lParam) == gFloorSlider) {
            UpdateSensitivityLabel();
            if (CanEnable()) SendConfig(gEnabled);
            return 0;
        }
        break;
    case WM_TIMER:
        if (wParam == kTimer && RefreshDriver() && gEnabled) SendConfig(true);
        return 0;
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) { AddTrayIcon(); if (gInTray) ShowWindow(window, SW_HIDE); }
        return 0;
    case kTrayMessage:
        if (LOWORD(lParam) == WM_LBUTTONDBLCLK || LOWORD(lParam) == NIN_SELECT ||
            LOWORD(lParam) == NIN_KEYSELECT) { RemoveTrayIcon(); ShowWindow(window, SW_RESTORE); }
        else if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU) ShowTrayMenu();
        return 0;
    case WM_CLOSE:
        if (TipTestPanelHasUnsavedData(gTipPanel)) {
            int answer = MessageBoxW(window, L"Salvar a captura da ponta antes de fechar?", L"Captura não salva", MB_YESNOCANCEL | MB_ICONQUESTION);
            if (answer == IDCANCEL || (answer == IDYES && !SaveTipTestPanel(gTipPanel))) return 0;
        }
        DestroyWindow(window); return 0;
    case WM_DESTROY:
        KillTimer(window, kTimer);
        // Closing the exclusive control handle disables the mode in kernel.
        if (gDevice != INVALID_HANDLE_VALUE) { CloseHandle(gDevice); gDevice = INVALID_HANDLE_VALUE; }
        RemoveTrayIcon(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    HANDLE singleInstance = CreateMutexW(nullptr, FALSE, kMutexName);
    if (!singleInstance) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND previous = FindWindowW(kClassName, nullptr);
        if (previous) { ShowWindow(previous, SW_RESTORE); SetForegroundWindow(previous); }
        CloseHandle(singleInstance); return 0;
    }
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_BAR_CLASSES};
    InitCommonControlsEx(&controls);
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProc; windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kClassName;
    if (!RegisterClassW(&windowClass)) { CloseHandle(singleInstance); return 1; }
    gWindow = CreateWindowExW(WS_EX_APPWINDOW, kClassName, L"Galaxy Pen Mapper",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1010, 655, nullptr, nullptr, instance, nullptr);
    if (!gWindow) { CloseHandle(singleInstance); return 1; }
    ShowWindow(gWindow, showCommand); UpdateWindow(gWindow);
    MSG message{};
    BOOL result;
    while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        if (!IsDialogMessageW(gWindow, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    CloseHandle(singleInstance);
    return result == -1 ? 1 : static_cast<int>(message.wParam);
}
