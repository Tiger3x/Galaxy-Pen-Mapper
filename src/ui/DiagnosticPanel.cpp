#include "DiagnosticPanel.h"

#include <algorithm>

namespace {

void text(HDC dc, int x, int y, const std::wstring& value, COLORREF color = RGB(220, 228, 240)) {
    SetTextColor(dc, color);
    TextOutW(dc, x, y, value.c_str(), static_cast<int>(value.size()));
}

void metric(HDC dc, int x, int y, const wchar_t* label, const std::wstring& value) {
    text(dc, x, y, label, RGB(136, 156, 184));
    text(dc, x, y + 20, value, RGB(242, 246, 252));
}

} // namespace

RECT DiagnosticPanel::captureArea(HWND window) const {
    RECT client{};
    GetClientRect(window, &client);
    return {32, 270, std::max(132L, client.right - 32), std::max(370L, client.bottom - 32)};
}

bool DiagnosticPanel::contains(HWND window, POINT screenPoint) const {
    POINT point = screenPoint;
    ScreenToClient(window, &point);
    const RECT area = captureArea(window);
    return PtInRect(&area, point) != FALSE;
}

void DiagnosticPanel::draw(HDC dc, HWND window, const PenState& state, unsigned long long events,
                            bool recording, bool penInside, const std::wstring& status,
                            const std::wstring& latestRaw) const {
    RECT client{};
    GetClientRect(window, &client);
    HBRUSH page = CreateSolidBrush(RGB(18, 24, 35));
    FillRect(dc, &client, page);
    DeleteObject(page);

    SetBkMode(dc, TRANSPARENT);
    HFONT titleFont = CreateFontW(24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH, L"Segoe UI");
    HGDIOBJ previousFont = SelectObject(dc, titleFont);
    text(dc, 32, 24, L"Galaxy Pen Diagnostic Studio", RGB(246, 249, 255));
    SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
    text(dc, 32, 58, L"P002.5  •  Caneta, Raw HID, foco, mouse e teclado", RGB(152, 170, 198));

    const RECT area = captureArea(window);
    const COLORREF card = RGB(29, 38, 53);
    HBRUSH cardBrush = CreateSolidBrush(card);
    RECT summary{32, 152, client.right - 32, 246};
    FillRect(dc, &summary, cardBrush);
    DeleteObject(cardBrush);

    metric(dc, 52, 169, L"STATUS", recording ? L"GRAVANDO" : L"EM ESPERA");
    metric(dc, 210, 169, L"EVENTOS", std::to_wstring(events));
    metric(dc, 350, 169, L"PRESSÃO", std::to_wstring(state.pressure));
    metric(dc, 500, 169, L"TILT X / Y", std::to_wstring(state.tiltX) + L" / " + std::to_wstring(state.tiltY));
    metric(dc, 690, 169, L"CANETA", penInside ? L"NA ÁREA" : L"FORA DA ÁREA");
    text(dc, 52, 222, status, RGB(180, 197, 220));

    HBRUSH areaBrush = CreateSolidBrush(recording ? RGB(21, 51, 47) : RGB(31, 42, 58));
    FillRect(dc, &area, areaBrush);
    DeleteObject(areaBrush);
    HPEN border = CreatePen(PS_SOLID, 2, recording ? RGB(52, 201, 147) : RGB(103, 128, 165));
    HGDIOBJ oldPen = SelectObject(dc, border);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, area.left, area.top, area.right, area.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(border);

    text(dc, area.left + 20, area.top + 18, L"ÁREA DE TESTE DA CANETA", RGB(226, 234, 247));
    text(dc, area.left + 20, area.top + 46, L"Ponteiro aqui; Raw HID durante a sessão, mesmo sem foco.", RGB(151, 171, 198));
    metric(dc, area.left + 20, area.top + 90, L"POINTER FLAGS", std::to_wstring(state.pointerFlags));
    metric(dc, area.left + 220, area.top + 90, L"PEN FLAGS", std::to_wstring(state.penFlags));
    metric(dc, area.left + 400, area.top + 90, L"POSIÇÃO", std::to_wstring(state.position.x) + L", " + std::to_wstring(state.position.y));

    std::wstring preview = latestRaw.empty() ? L"Nenhum relatório Raw HID recebido." : latestRaw;
    if (preview.size() > 100) preview = preview.substr(0, 100) + L"...";
    text(dc, area.left + 20, area.bottom - 54, L"ÚLTIMO RAW HID", RGB(136, 156, 184));
    text(dc, area.left + 20, area.bottom - 30, preview, RGB(213, 224, 240));

    SelectObject(dc, previousFont);
    DeleteObject(titleFont);
}
