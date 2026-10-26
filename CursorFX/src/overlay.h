#pragma once
#include <windows.h>

bool OverlayInit(HINSTANCE hInst);
void OverlayShutdown();
void OverlayRequestRedraw();
HWND OverlayGetHwnd();
void OverlaySetDpi(UINT dpi);
UINT OverlayGetDpi();
