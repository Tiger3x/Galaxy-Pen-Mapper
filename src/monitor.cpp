#include <windows.h>

#include "ui/MainWindow.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    MainWindow window;
    if (!window.create(instance, showCommand)) {
        MessageBoxW(nullptr, L"Não foi possível iniciar o Galaxy Pen Diagnostic Studio.", L"Galaxy Pen Diagnostic Studio", MB_OK | MB_ICONERROR);
        return 1;
    }
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
