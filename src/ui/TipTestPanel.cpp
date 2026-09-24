#include "TipTestPanel.h"
#include "TipTestCapture.h"
#include <commdlg.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace {
constexpr wchar_t kClass[] = L"GalaxyPenTipTestCanvas";
struct Panel {
    HWND status{};
    TipTestCapture capture;
    ULONGLONG start = GetTickCount64();
    HDC memory{};
    HBITMAP bitmap{};
    HGDIOBJ oldBitmap{};
    int width{}, height{};
    bool lastContact = false;
    bool ownedByWindow = false;
    UINT32 lastId{};
    POINT last{};
    std::wstring lastText;
    ~Panel() {
        if (memory) { SelectObject(memory, oldBitmap); DeleteObject(bitmap); DeleteDC(memory); }
    }
    void Status(const std::wstring& text) {
        if (text != lastText) { SetWindowTextW(status, text.c_str()); lastText = text; }
    }
};
Panel* State(HWND window) { return reinterpret_cast<Panel*>(GetWindowLongPtrW(window, GWLP_USERDATA)); }
void ResetBitmap(HWND window, Panel& p) {
    RECT r{}; GetClientRect(window, &r);
    if (p.memory) { SelectObject(p.memory, p.oldBitmap); DeleteObject(p.bitmap); DeleteDC(p.memory); }
    const HDC dc = GetDC(window);
    p.width = std::max(1L, r.right); p.height = std::max(1L, r.bottom);
    p.memory = CreateCompatibleDC(dc);
    p.bitmap = CreateCompatibleBitmap(dc, p.width, p.height);
    p.oldBitmap = SelectObject(p.memory, p.bitmap);
    ReleaseDC(window, dc);
    FillRect(p.memory, &r, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    p.lastContact = false;
    InvalidateRect(window, nullptr, FALSE);
}
void Observe(HWND window, Panel& p, UINT message, WPARAM wParam) {
    const UINT32 id = GET_POINTERID_WPARAM(wParam);
    POINTER_PEN_INFO info{};
    if (!GetPointerPenInfo(id, &info)) {
        if (message == WM_POINTERLEAVE || message == WM_POINTERCAPTURECHANGED) {
            p.lastContact = false;
            p.Status(L"Ponta fora da área de teste. Traço encerrado.");
        }
        return; // Mouse/touch are never recorded as pen samples.
    }
    POINT point = info.pointerInfo.ptPixelLocation;
    ScreenToClient(window, &point);
    const bool ended = message == WM_POINTERUP || message == WM_POINTERLEAVE ||
        message == WM_POINTERCAPTURECHANGED || (info.pointerInfo.pointerFlags & POINTER_FLAG_CANCELED);
    const bool contact = !ended && (info.pointerInfo.pointerFlags & POINTER_FLAG_INCONTACT);
    const bool hasPressure = (info.penMask & PEN_MASK_PRESSURE) != 0;
    const bool eraser = (info.penFlags & (PEN_FLAG_ERASER | PEN_FLAG_INVERTED)) != 0;
    const TipTestSample sample{GetTickCount64() - p.start, message, id, point.x, point.y,
        info.pointerInfo.pointerFlags, info.penFlags, contact, hasPressure,
        hasPressure ? info.pressure : 0u, info.tiltX, info.tiltY};
    if (!p.capture.Append(sample)) {
        p.lastContact = false;
        p.Status(L"Limite de 50.000 eventos atingido. Salve a captura e limpe para continuar.");
        return;
    }
    if (contact && p.memory) {
        const int thickness = hasPressure ? 2 + static_cast<int>(std::min(info.pressure, 1024u) * 8 / 1024) : 2;
        const COLORREF color = eraser ? RGB(206, 75, 30) : RGB(25, 94, 180);
        HPEN pen = CreatePen(PS_SOLID, thickness, color);
        const HGDIOBJ oldPen = SelectObject(p.memory, pen);
        const bool join = p.lastContact && p.lastId == id && message != WM_POINTERDOWN;
        MoveToEx(p.memory, join ? p.last.x : point.x, join ? p.last.y : point.y, nullptr);
        LineTo(p.memory, point.x, point.y);
        HBRUSH brush = CreateSolidBrush(color);
        const HGDIOBJ oldBrush = SelectObject(p.memory, brush);
        const int radius = std::max(1, thickness / 2);
        Ellipse(p.memory, point.x - radius, point.y - radius, point.x + radius + 1, point.y + radius + 1);
        SelectObject(p.memory, oldBrush); DeleteObject(brush);
        SelectObject(p.memory, oldPen); DeleteObject(pen);
        InvalidateRect(window, nullptr, FALSE);
    }
    p.lastContact = contact; p.lastId = id; p.last = point;
    p.Status(std::wstring(contact ? L"Contato" : L"Sem contato") + L" | " + (eraser ? L"Borracha / invertida" : L"Caneta") +
        L"\r\nPressão Windows: " + (hasPressure ? std::to_wstring(info.pressure) + L" / 1024" : L"não informada") +
        L" | eventos: " + std::to_wstring(p.capture.samples.size()));
}
LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    Panel* p = State(window);
    if (message == WM_NCCREATE) {
        p = static_cast<Panel*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
    }
    if (!p) return DefWindowProcW(window, message, wParam, lParam);
    switch (message) {
    case WM_SIZE: ResetBitmap(window, *p); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint{}; HDC dc = BeginPaint(window, &paint);
        if (p->memory) BitBlt(dc, 0, 0, p->width, p->height, p->memory, 0, 0, SRCCOPY);
        EndPaint(window, &paint); return 0;
    }
    case WM_POINTERDOWN: case WM_POINTERUPDATE: case WM_POINTERUP:
    case WM_POINTERENTER: case WM_POINTERLEAVE: case WM_POINTERCAPTURECHANGED:
        Observe(window, *p, message, wParam); return 0;
    case WM_NCDESTROY:
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        if (p->ownedByWindow) delete p;
        break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

HWND CreateTipTestPanel(HWND parent, int x, int y, int width, int height, HWND status) {
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW cls{}; cls.hInstance = instance; cls.lpfnWndProc = Proc; cls.lpszClassName = kClass;
    cls.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    if (!RegisterClassW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return nullptr;
    auto state = std::make_unique<Panel>();
    state->status = status;
    const HWND panel = CreateWindowExW(WS_EX_CLIENTEDGE, kClass, L"Área de captura da ponta (somente caneta)",
        WS_CHILD | WS_VISIBLE, x, y, width, height, parent, nullptr, instance, state.get());
    if (panel) { state->ownedByWindow = true; state.release(); }
    return panel;
}
void ClearTipTestPanel(HWND panel) {
    if (auto* p = State(panel)) {
        if (p->capture.unsaved && MessageBoxW(GetParent(panel), L"Descartar os eventos ainda não salvos?", L"Limpar teste",
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        p->capture.Clear(); p->start = GetTickCount64(); ResetBitmap(panel, *p);
        p->Status(L"Aproxime e toque a caneta na área branca. Mouse e toque são ignorados.");
    }
}
bool TipTestPanelHasUnsavedData(HWND panel) {
    const auto* p = State(panel); return p && p->capture.unsaved;
}
bool SaveTipTestPanel(HWND panel) {
    auto* p = State(panel);
    if (!p || p->capture.samples.empty()) {
        MessageBoxW(GetParent(panel), L"Ainda não há eventos de caneta para salvar.", L"Captura da ponta", MB_OK); return false;
    }
    wchar_t path[32768]{};
    SYSTEMTIME now{}; GetLocalTime(&now);
    swprintf_s(path, L"ponta-%04u%02u%02u-%02u%02u%02u.csv", now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
    OPENFILENAMEW dialog{sizeof(dialog)};
    wchar_t module[32768]{};
    std::wstring initialDirectory;
    const DWORD moduleLength = GetModuleFileNameW(nullptr, module, ARRAYSIZE(module));
    if (moduleLength && moduleLength < ARRAYSIZE(module)) {
        auto directory = std::filesystem::path(module).parent_path();
        for (int level = 0; level <= 3; ++level, directory = directory.parent_path()) {
            std::error_code error;
            if (std::filesystem::is_directory(directory / L"captures", error)) {
                initialDirectory = (directory / L"captures").wstring(); break;
            }
        }
    }
    dialog.hwndOwner = GetParent(panel); dialog.lpstrFile = path; dialog.nMaxFile = ARRAYSIZE(path);
    if (!initialDirectory.empty()) dialog.lpstrInitialDir = initialDirectory.c_str();
    dialog.lpstrFilter = L"Captura CSV\0*.csv\0\0"; dialog.lpstrDefExt = L"csv";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&dialog)) return false;
    std::ofstream output(std::filesystem::path(path), std::ios::binary);
    p->capture.WriteCsv(output); output.close();
    if (!output) {
        MessageBoxW(GetParent(panel), L"Não foi possível salvar. Os eventos continuam na memória.", L"Falha ao salvar", MB_OK | MB_ICONERROR); return false;
    }
    p->capture.unsaved = false;
    p->Status(L"Captura salva. Novos eventos continuarão sendo registrados.");
    return true;
}
