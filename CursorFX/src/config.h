#pragma once
#include <windows.h>
#include <string>

struct Config {
    // Appearance
    bool  enableDot = true;
    bool  enableRing = true;
    bool  hideCursor = false; // Mặc định TẮT để an toàn

    float dotSize = 8.0f;
    float ringSize = 40.0f;
    float ringBorder = 1.5f;
    float ringAlpha = 0.90f;

    float fitPadding = 6.0f;
    float maxFitSize = 200.0f;

    float followSpeed = 16.0f;
    float fitSpeed = 26.0f;
    float shapeSpeed = 18.0f;

    float clickScale = 0.78f;
    float springK = 520.0f;
    float springDamp = 21.0f;

    float textBarW = 2.0f;
    float textBarH = 22.0f;
    float textRingSize = 24.0f;
    float textRingAlpha = 0.40f;

    // Colors (hex #RRGGBB)
    COLORREF dotColor = RGB(212, 175, 55);
    COLORREF ringColor = RGB(212, 175, 55);
    COLORREF ringFillColor = RGB(212, 175, 55);
    float ringFillAlpha = 0.12f;

    // Blend mode: 0 = normal, 1 = difference (yêu cầu Direct2D, hiện chưa hỗ trợ)
    int blendMode = 0;

    // System
    bool useUIA = true;          // Bật UIA backend
    int uiaThrottleMs = 80;      // Tần suất gọi UIA
};

extern Config g_cfg;

void ConfigLoad(const wchar_t* iniPath);
void ConfigSave(const wchar_t* iniPath);
void ConfigResetToDefaults();
void ConfigApplyToOverlay();
void ConfigGetIniPath(wchar_t* out, size_t maxLen);
