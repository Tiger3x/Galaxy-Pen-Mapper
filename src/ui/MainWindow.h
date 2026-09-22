#pragma once

#include <windows.h>

#include "../capture/SessionWriter.h"
#include "../input/PenCapture.h"
#include "../input/RawHidCapture.h"
#include "DiagnosticPanel.h"

#include <string>

class MainWindow {
public:
    bool create(HINSTANCE instance, int showCommand);

private:
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handle(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    void createControls(HWND window);
    void startCapture(HWND window);
    void stopCapture();
    void clearPanel();
    void scheduleRepaint();
    std::wstring controlText(HWND control) const;
    bool canRecord(HWND window) const;

    HWND sessionBox = nullptr;
    HWND descriptionBox = nullptr;
    bool recording = false;
    bool penInside = false;
    bool repaintPending = true;
    unsigned long long eventCount = 0;
    std::wstring status = L"Pronto para uma captura de diagnóstico.";
    std::wstring latestRaw;
    PenCapture penCapture;
    RawHidCapture rawHidCapture;
    SessionWriter sessionWriter;
    DiagnosticPanel panel;
};
