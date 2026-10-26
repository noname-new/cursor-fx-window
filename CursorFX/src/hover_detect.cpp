#include "hover_detect.h"
#include "config.h"
#include <uiautomation.h>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uiautomationcore.lib")

extern Config g_cfg;

static IUIAutomation* g_uia = nullptr;
static bool  g_uiaInited  = false;
static bool  g_uiaFailed  = false;
static DWORD g_lastUIATime = 0;
static HoverInfo g_lastHover{};
static POINT g_lastPt{};

// ---------------------------------------------------------------
// Ước lượng corner radius khi không có thông tin border-radius
// - Nếu gần vuông (w ≈ h)  -> coi như tròn: r = min(w,h)/2
// - Nếu chữ nhật dài       -> bo góc nhỏ: min(w,h) * 0.15, tối đa 8px
// ---------------------------------------------------------------
static float GuessCornerRadius(float w, float h) {
    if (w <= 0 || h <= 0) return 0;
    float minSide = std::min(w, h);
    float ratio = (std::max(w, h) > 0) ? (minSide / std::max(w, h)) : 1.0f;
    if (ratio > 0.85f) {
        // Gần vuông -> tròn
        return minSide * 0.5f;
    }
    // Chữ nhật -> bo góc nhỏ
    return std::min(minSide * 0.15f, 8.0f);
}

// ---------------------------------------------------------------
// Clamp kích thước fit: không bao giờ vượt maxFitSize
// ---------------------------------------------------------------
static void ClampFit(HoverInfo& info) {
    float maxS = g_cfg.maxFitSize;
    if (info.width  > maxS) info.width  = maxS;
    if (info.height > maxS) info.height = maxS;
}

// ---------------------------------------------------------------
// Heuristic nhanh (native controls + browser cursor fallback)
// ---------------------------------------------------------------
static HoverInfo Heuristic(POINT pt) {
    HoverInfo info{};
    HWND hwnd = WindowFromPoint(pt);
    if (!hwnd || !IsWindow(hwnd)) return info;

    wchar_t cls[256] = {0};
    GetClassNameW(hwnd, cls, 255);

    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    bool tabStop = (style & WS_TABSTOP) != 0;
    if (_wcsicmp(cls, L"Button") == 0 || _wcsicmp(cls, L"ToolbarWindow32") == 0)
        tabStop = true;

    // Text
    if (_wcsicmp(cls, L"Edit") == 0 ||
        _wcsicmp(cls, L"RichEdit20W") == 0 ||
        _wcsicmp(cls, L"RichEdit50W") == 0) {
        info.active = true;
        info.isText = true;
        info.width = 2; info.height = 22;
        info.centerX = (float)pt.x; info.centerY = (float)pt.y;
        info.cornerRadius = 1.0f;
        return info;
    }

    // Native button-like
    if (tabStop) {
        RECT r{};
        if (GetWindowRect(hwnd, &r)) {
            float w = (float)(r.right - r.left);
            float h = (float)(r.bottom - r.top);
            if (w > 0 && h > 0) {
                info.active = true;
                info.centerX = (r.left + r.right) * 0.5f;
                info.centerY = (r.top + r.bottom) * 0.5f;
                info.width  = w;
                info.height = h;
                info.cornerRadius = GuessCornerRadius(w, h);
                ClampFit(info);
            }
        }
    }
    return info;
}

// ---------------------------------------------------------------
// UIA chính xác (throttled)
// ---------------------------------------------------------------
static HoverInfo UIA(POINT pt) {
    if (!g_uiaInited) {
        g_uiaInited = true;
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        HRESULT hr = CoCreateInstance(__uuidof(CUIAutomation), nullptr,
            CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation), (void**)&g_uia);
        if (FAILED(hr) || !g_uia) { g_uiaFailed = true; return {}; }
    }
    if (g_uiaFailed || !g_uia) return {};

    HoverInfo info{};
    IUIAutomationElement* elem = nullptr;
    HRESULT hr = g_uia->ElementFromPoint(pt, &elem);
    if (FAILED(hr) || !elem) return {};

    RECT rect{};
    elem->get_CurrentBoundingRectangle(&rect);
    CONTROLTYPEID ct = 0;
    elem->get_CurrentControlType(&ct);

    bool isText = (ct == UIA_EditControlTypeId || ct == UIA_DocumentControlTypeId);
    bool isButtonLike = (ct == UIA_ButtonControlTypeId ||
                         ct == UIA_HyperlinkControlTypeId ||
                         ct == UIA_CheckBoxControlTypeId ||
                         ct == UIA_RadioButtonControlTypeId ||
                         ct == UIA_TabItemControlTypeId ||
                         ct == UIA_MenuItemControlTypeId);

    if (isText) {
        info.active = true;
        info.isText = true;
        info.width = 2; info.height = 22;
        info.centerX = (float)pt.x; info.centerY = (float)pt.y;
        info.cornerRadius = 1.0f;
    } else if (isButtonLike && rect.right > rect.left) {
        float w = (float)(rect.right - rect.left);
        float h = (float)(rect.bottom - rect.top);
        if (w > 0 && h > 0 && w <= g_cfg.maxFitSize && h <= g_cfg.maxFitSize) {
            info.active = true;
            info.centerX = (rect.left + rect.right) * 0.5f;
            info.centerY = (rect.top + rect.bottom) * 0.5f;
            info.width  = w;
            info.height = h;
            info.cornerRadius = GuessCornerRadius(w, h);
        }
    }

    elem->Release();
    return info;
}

// ---------------------------------------------------------------
// Public API
// ---------------------------------------------------------------
HoverInfo HoverDetect_Get(POINT screenPt, bool useUIA, int throttleMs) {
    if (useUIA && !g_uiaFailed) {
        DWORD now = GetTickCount();
        bool shouldQuery = (now - g_lastUIATime >= (DWORD)throttleMs)
                        || (g_lastHover.active == false);

        if (shouldQuery) {
            g_lastUIATime = now;
            g_lastPt = screenPt;
            HoverInfo u = UIA(screenPt);
            if (u.active) { g_lastHover = u; return u; }
            // fallback heuristic nếu UIA không trả về gì
        } else {
            // Cache nếu chuột chưa di chuyển xa
            if (abs(screenPt.x - g_lastPt.x) < 4 &&
                abs(screenPt.y - g_lastPt.y) < 4)
                return g_lastHover;
        }
    }

    HoverInfo h = Heuristic(screenPt);
    g_lastHover = h;
    return h;
}