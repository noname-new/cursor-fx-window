#include "config.h"
#include <cstdio>
#include <cwchar>
#include <cstdlib>

Config g_cfg;

static void EnsureIniExists(const wchar_t* path) {
    if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) return;
    WritePrivateProfileStringW(L"CursorFX", L"Version", L"1", path);
    WritePrivateProfileStringW(L"CursorFX", L"EnableDot", L"1", path);
    WritePrivateProfileStringW(L"CursorFX", L"EnableRing", L"1", path);
    WritePrivateProfileStringW(L"CursorFX", L"HideCursor", L"0", path); // TẮT mặc định
    WritePrivateProfileStringW(L"CursorFX", L"DotSize", L"8", path);
    WritePrivateProfileStringW(L"CursorFX", L"RingSize", L"40", path);
    WritePrivateProfileStringW(L"CursorFX", L"RingBorder", L"1.5", path);
    WritePrivateProfileStringW(L"CursorFX", L"RingAlpha", L"0.9", path);
    WritePrivateProfileStringW(L"CursorFX", L"FitPadding", L"6", path);
    WritePrivateProfileStringW(L"CursorFX", L"MaxFitSize", L"200", path);
    WritePrivateProfileStringW(L"CursorFX", L"FollowSpeed", L"16", path);
    WritePrivateProfileStringW(L"CursorFX", L"FitSpeed", L"26", path);
    WritePrivateProfileStringW(L"CursorFX", L"ShapeSpeed", L"18", path);
    WritePrivateProfileStringW(L"CursorFX", L"ClickScale", L"0.78", path);
    WritePrivateProfileStringW(L"CursorFX", L"SpringK", L"520", path);
    WritePrivateProfileStringW(L"CursorFX", L"SpringDamp", L"21", path);
    WritePrivateProfileStringW(L"CursorFX", L"TextBarW", L"2", path);
    WritePrivateProfileStringW(L"CursorFX", L"TextBarH", L"22", path);
    WritePrivateProfileStringW(L"CursorFX", L"TextRingSize", L"24", path);
    WritePrivateProfileStringW(L"CursorFX", L"TextRingAlpha", L"0.4", path);
    WritePrivateProfileStringW(L"CursorFX", L"DotColor", L"#D4AF37", path);
    WritePrivateProfileStringW(L"CursorFX", L"RingColor", L"#D4AF37", path);
    WritePrivateProfileStringW(L"CursorFX", L"RingFill", L"#D4AF37", path);
    WritePrivateProfileStringW(L"CursorFX", L"RingFillAlpha", L"0.12", path);
    WritePrivateProfileStringW(L"CursorFX", L"BlendMode", L"0", path);
    WritePrivateProfileStringW(L"CursorFX", L"UseUIA", L"1", path);
    WritePrivateProfileStringW(L"CursorFX", L"UIAThrottleMs", L"80", path);
}

static COLORREF ReadColor(const wchar_t* key, COLORREF def, const wchar_t* path) {
    wchar_t buf[64];
    wchar_t defbuf[16];
    swprintf_s(defbuf, L"#%02X%02X%02X", GetRValue(def), GetGValue(def), GetBValue(def));
    GetPrivateProfileStringW(L"CursorFX", key, defbuf, buf, 64, path);
    unsigned r=0,g=0,b=0;
    if (swscanf_s(buf, L"#%02X%02X%02X", &r, &g, &b) == 3)
        return RGB(r,g,b);
    return def;
}

void ConfigGetIniPath(wchar_t* out, size_t maxLen) {
    GetModuleFileNameW(nullptr, out, (DWORD)maxLen);
    wchar_t* slash = wcsrchr(out, L'\\');
    if (slash) wcscpy_s(slash + 1, maxLen - (slash + 1 - out), L"cursorfx.ini");
}

void ConfigLoad(const wchar_t* iniPath) {
    EnsureIniExists(iniPath);

    g_cfg.enableDot = GetPrivateProfileIntW(L"CursorFX", L"EnableDot", 1, iniPath) != 0;
    g_cfg.enableRing = GetPrivateProfileIntW(L"CursorFX", L"EnableRing", 1, iniPath) != 0;
    g_cfg.hideCursor = GetPrivateProfileIntW(L"CursorFX", L"HideCursor", 0, iniPath) != 0; // default 0

    auto rf = [&](const wchar_t* k, float def) -> float {
        wchar_t buf[64]; wchar_t db[64];
        swprintf_s(db, L"%f", def);
        GetPrivateProfileStringW(L"CursorFX", k, db, buf, 64, iniPath);
        return wcstof(buf, nullptr);
    };

    g_cfg.dotSize = rf(L"DotSize", 8.0f);
    g_cfg.ringSize = rf(L"RingSize", 40.0f);
    g_cfg.ringBorder = rf(L"RingBorder", 1.5f);
    g_cfg.ringAlpha = rf(L"RingAlpha", 0.9f);
    g_cfg.fitPadding = rf(L"FitPadding", 6.0f);
    g_cfg.maxFitSize = rf(L"MaxFitSize", 200.0f);
    g_cfg.followSpeed = rf(L"FollowSpeed", 16.0f);
    g_cfg.fitSpeed = rf(L"FitSpeed", 26.0f);
    g_cfg.shapeSpeed = rf(L"ShapeSpeed", 18.0f);
    g_cfg.clickScale = rf(L"ClickScale", 0.78f);
    g_cfg.springK = rf(L"SpringK", 520.0f);
    g_cfg.springDamp = rf(L"SpringDamp", 21.0f);
    g_cfg.textBarW = rf(L"TextBarW", 2.0f);
    g_cfg.textBarH = rf(L"TextBarH", 22.0f);
    g_cfg.textRingSize = rf(L"TextRingSize", 24.0f);
    g_cfg.textRingAlpha = rf(L"TextRingAlpha", 0.4f);

    g_cfg.dotColor = ReadColor(L"DotColor", RGB(212,175,55), iniPath);
    g_cfg.ringColor = ReadColor(L"RingColor", RGB(212,175,55), iniPath);
    g_cfg.ringFillColor = ReadColor(L"RingFill", RGB(212,175,55), iniPath);
    g_cfg.ringFillAlpha = rf(L"RingFillAlpha", 0.12f);
    g_cfg.blendMode = GetPrivateProfileIntW(L"CursorFX", L"BlendMode", 0, iniPath);
    g_cfg.useUIA = GetPrivateProfileIntW(L"CursorFX", L"UseUIA", 1, iniPath) != 0;
    g_cfg.uiaThrottleMs = GetPrivateProfileIntW(L"CursorFX", L"UIAThrottleMs", 80, iniPath);
}

void ConfigSave(const wchar_t* iniPath) {
    auto wi = [&](const wchar_t* k, int v) {
        wchar_t b[32]; swprintf_s(b, L"%d", v);
        WritePrivateProfileStringW(L"CursorFX", k, b, iniPath);
    };
    auto wf = [&](const wchar_t* k, float v) {
        wchar_t b[64]; swprintf_s(b, L"%f", v);
        WritePrivateProfileStringW(L"CursorFX", k, b, iniPath);
    };
    auto wc = [&](const wchar_t* k, COLORREF c) {
        wchar_t b[16]; swprintf_s(b, L"#%02X%02X%02X", GetRValue(c), GetGValue(c), GetBValue(c));
        WritePrivateProfileStringW(L"CursorFX", k, b, iniPath);
    };

    wi(L"Version", 1);
    wi(L"EnableDot", g_cfg.enableDot ? 1 : 0);
    wi(L"EnableRing", g_cfg.enableRing ? 1 : 0);
    wi(L"HideCursor", g_cfg.hideCursor ? 1 : 0);
    wf(L"DotSize", g_cfg.dotSize);
    wf(L"RingSize", g_cfg.ringSize);
    wf(L"RingBorder", g_cfg.ringBorder);
    wf(L"RingAlpha", g_cfg.ringAlpha);
    wf(L"FitPadding", g_cfg.fitPadding);
    wf(L"MaxFitSize", g_cfg.maxFitSize);
    wf(L"FollowSpeed", g_cfg.followSpeed);
    wf(L"FitSpeed", g_cfg.fitSpeed);
    wf(L"ShapeSpeed", g_cfg.shapeSpeed);
    wf(L"ClickScale", g_cfg.clickScale);
    wf(L"SpringK", g_cfg.springK);
    wf(L"SpringDamp", g_cfg.springDamp);
    wf(L"TextBarW", g_cfg.textBarW);
    wf(L"TextBarH", g_cfg.textBarH);
    wf(L"TextRingSize", g_cfg.textRingSize);
    wf(L"TextRingAlpha", g_cfg.textRingAlpha);
    wc(L"DotColor", g_cfg.dotColor);
    wc(L"RingColor", g_cfg.ringColor);
    wc(L"RingFill", g_cfg.ringFillColor);
    wf(L"RingFillAlpha", g_cfg.ringFillAlpha);
    wi(L"BlendMode", g_cfg.blendMode);
    wi(L"UseUIA", g_cfg.useUIA ? 1 : 0);
    wi(L"UIAThrottleMs", g_cfg.uiaThrottleMs);
}

void ConfigResetToDefaults() {
    g_cfg = Config{};
    g_cfg.hideCursor = false; // giữ an toàn
}

// Được implement trong overlay.cpp
extern void OverlayRequestRedraw();
void ConfigApplyToOverlay() {
    OverlayRequestRedraw();
}
