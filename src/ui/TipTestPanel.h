#pragma once
#include <windows.h>

// Observes only Windows pen events delivered to this canvas; never injects input.
HWND CreateTipTestPanel(HWND parent, int x, int y, int width, int height, HWND status);
void ClearTipTestPanel(HWND panel);
bool SaveTipTestPanel(HWND panel);
bool TipTestPanelHasUnsavedData(HWND panel);
