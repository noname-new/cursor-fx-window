// src/overlay.cpp
#include "overlay.h"
#include "config.h"
#include "motion.h"
#include "cursor_hide.h"
#include "hover_detect.h"

#include <gdiplus.h>
#include <cmath>
#include <algorithm>
#include <cstring>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

extern Config g_cfg;

// ------------------------------------------------------------------
// Trạng thái toàn cục
// ------------------------------------------------------------------
static HWND      g_hwnd       = nullptr;
static ULONG_PTR g_gdiToken   = 0;
static int       g_screenX    = 0;
static int       g_screenY    = 0;
static int       g_screenW    = 0;
static int       g_screenH    = 0;

static HDC       g_memDC      = nullptr;
static HBITMAP   g_dib        = nullptr;
static void*     g_bits       = nullptr;
static HGDIOBJ   g_oldBmp     = nullptr;

static MotionState g_motion;
static double      g_lastTime = 0.0;
static bool        g_needsRedraw = true;
static UINT        g_dpi      = 96;

// ------------------------------------------------------------------
// Tiện ích
// ------------------------------------------------------------------
static double NowSec() {
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)f.QuadPart;
}

static Color Rgba(COLORREF c, float alpha) {
    BYTE a = (BYTE)std::clamp(alpha * 255.0f, 0.0f, 255.0f);
    return Color(a, GetRValue(c), GetGValue(c), GetBValue(c));
}

// Tạo GraphicsPath hình chữ nhật bo góc.
// Nếu r >= min(w,h)/2 -> tự động thành hình tròn (4 cung 90°).
static void MakeRoundRect(GraphicsPath& path,
                          float x, float y, float w, float h, float r) {
    if (w <= 0.0f || h <= 0.0f) return;
    r = std::min(r, std::min(w, h) * 0.5f);
    if (r < 0.5f) {
        path.AddRectangle(RectF(x, y, w, h));
        return;
    }
    float d = r * 2.0f;
    path.AddArc(x,         y,         d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y,         d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d,   0.0f, 90.0f);
    path.AddArc(x,         y + h - d, d, d,  90.0f, 90.0f);
    path.CloseFigure();
}

// ------------------------------------------------------------------
// DIB cache: tạo một lần, tái sử dụng mỗi frame
// ------------------------------------------------------------------
static void RecreateDib() {
    if (g_memDC) {
        SelectObject(g_memDC, g_oldBmp);
        DeleteDC(g_memDC);
        g_memDC = nullptr;
    }
    if (g_dib) {
        DeleteObject(g_dib);
        g_dib = nullptr;
    }

    HDC screen = GetDC(nullptr);
    g_memDC = CreateCompatibleDC(screen);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = g_screenW;
    bmi.bmiHeader.biHeight      = -g_screenH;   // top-down
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    g_dib = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &g_bits, nullptr, 0);
    g_oldBmp = SelectObject(g_memDC, g_dib);

    ReleaseDC(nullptr, screen);
    g_needsRedraw = true;
}

// ------------------------------------------------------------------
// Vẽ một frame
// ------------------------------------------------------------------
static void RenderFrame() {
    double now = NowSec();
    double dt = now - g_lastTime;
    if (dt <= 0.0) dt = 0.001;
    if (dt > 0.05) dt = 0.05;
    g_lastTime = now;

    // Vị trí chuột (toạ độ overlay = toạ độ virtual screen - offset)
    POINT cursor;
    GetCursorPos(&cursor);
    float mx = (float)(cursor.x - g_screenX);
    float my = (float)(cursor.y - g_screenY);

    // Trạng thái nhấn
    bool pressed = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0) ||
                   ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);

    // Hover (UIA hoặc heuristic)
    HoverInfo hover = HoverDetect_Get(cursor, g_cfg.useUIA, g_cfg.uiaThrottleMs);

    // Chuẩn bị input cho motion
    MotionInput in{};
    in.mouseX = mx;
    in.mouseY = my;
    in.pressed = pressed;
    in.hoverActive = hover.active && !hover.isText;
    in.hoverIsText = hover.isText;
    in.hoverX = hover.centerX - g_screenX;
    in.hoverY = hover.centerY - g_screenY;
    in.hoverW = hover.width;
    in.hoverH = hover.height;
    in.hoverCornerRadius = hover.cornerRadius;
    in.dt = (float)dt;

    MotionOutput out = g_motion.Update(in);

    // Xoá buffer
    std::memset(g_bits, 0, (size_t)g_screenW * g_screenH * 4);

    // Vẽ bằng GDI+
    {
        Graphics g(g_memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetCompositingMode(CompositingModeSourceOver);
        g.SetCompositingQuality(CompositingQualityHighQuality);

        // ---------- Vòng ----------
        if (g_cfg.enableRing) {
            float baseAlpha = in.hoverIsText ? g_cfg.textRingAlpha : g_cfg.ringAlpha;
            float finalAlpha = baseAlpha * out.ringOpacity;
            Color border = Rgba(g_cfg.ringColor, finalAlpha);

            float sw = out.ringW * out.ringScale;
            float sh = out.ringH * out.ringScale;
            float sr = out.ringR * out.ringScale;
            float left = out.ringX - sw * 0.5f;
            float top  = out.ringY - sh * 0.5f;

            GraphicsPath path;
            MakeRoundRect(path, left, top, sw, sh, sr);

            // Fill (khoảng vàng nhẹ khi nhấn / hover)
            float fillA = 0.0f;
            if (pressed) {
                fillA = g_cfg.ringFillAlpha;
            } else if (in.hoverActive) {
                fillA = g_cfg.ringFillAlpha * 0.6f;
            }
            if (fillA > 0.001f) {
                Color fillC = Rgba(g_cfg.ringFillColor, fillA * out.ringOpacity);
                SolidBrush fillBrush(fillC);
                g.FillPath(&fillBrush, &path);
            }

            // Viền
            float borderW = g_cfg.ringBorder * (pressed ? 1.3f : 1.0f);
            Pen pen(border, borderW);
            pen.SetLineJoin(LineJoinRound);
            g.DrawPath(&pen, &path);
        }

        // ---------- Tâm ----------
        if (g_cfg.enableDot) {
            if (in.hoverIsText) {
                // Con trỏ văn bản: thanh dọc
                SolidBrush brush(Rgba(g_cfg.dotColor, 1.0f));
                g.FillRectangle(&brush,
                    out.dotX - g_cfg.textBarW * 0.5f,
                    out.dotY - g_cfg.textBarH * 0.5f,
                    g_cfg.textBarW, g_cfg.textBarH);
            } else {
                // Chấm tròn
                float r = (g_cfg.dotSize * 0.5f) * out.dotScale;
                SolidBrush brush(Rgba(g_cfg.dotColor, 1.0f));
                g.FillEllipse(&brush, out.dotX - r, out.dotY - r, r * 2.0f, r * 2.0f);
            }
        }
    }

    // Cập nhật layered window
    POINT src{0, 0};
    POINT dst{g_screenX, g_screenY};
    SIZE  size{g_screenW, g_screenH};
    BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(g_hwnd, nullptr, &dst, &size, g_memDC, &src, 0, &blend, ULW_ALPHA);
}

// ------------------------------------------------------------------
// WndProc
// ------------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        RenderFrame();
        return 0;

    case WM_DISPLAYCHANGE: {
        g_screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
        g_screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
        g_screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        g_screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        if (g_screenW <= 0 || g_screenH <= 0) {
            g_screenX = g_screenY = 0;
            g_screenW = GetSystemMetrics(SM_CXSCREEN);
            g_screenH = GetSystemMetrics(SM_CYSCREEN);
        }
        RecreateDib();
        SetWindowPos(hwnd, HWND_TOPMOST,
                     g_screenX, g_screenY, g_screenW, g_screenH,
                     SWP_NOACTIVATE);
        g_motion.Init(0, 0);
        g_needsRedraw = true;
        return 0;
    }

    case WM_DPICHANGED: {
        g_dpi = HIWORD(wParam);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        OverlayShutdown();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------
bool OverlayInit(HINSTANCE hInst) {
    GdiplusStartupInput gsi;
    if (GdiplusStartup(&g_gdiToken, &gsi, nullptr) != Ok) return false;

    // Lấy toàn bộ virtual screen (đa màn hình)
    g_screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    g_screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    g_screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    g_screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (g_screenW <= 0 || g_screenH <= 0) {
        g_screenX = g_screenY = 0;
        g_screenW = GetSystemMetrics(SM_CXSCREEN);
        g_screenH = GetSystemMetrics(SM_CYSCREEN);
    }

    WNDCLASSW wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"CursorFXNativeOverlay";
    RegisterClassW(&wc);

    g_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"Cursor FX",
        WS_POPUP,
        g_screenX, g_screenY, g_screenW, g_screenH,
        nullptr, nullptr, hInst, nullptr);
    if (!g_hwnd) return false;

    RecreateDib();

    POINT pt;
    GetCursorPos(&pt);
    g_motion.Init((float)(pt.x - g_screenX), (float)(pt.y - g_screenY));
    g_lastTime = NowSec();

    ShowWindow(g_hwnd, SW_SHOWNA);
    SetTimer(g_hwnd, 1, 1, nullptr);  // ~1000 Hz timer, render theo dt
    return true;
}

void OverlayShutdown() {
    if (g_memDC) {
        SelectObject(g_memDC, g_oldBmp);
        DeleteDC(g_memDC);
        g_memDC = nullptr;
    }
    if (g_dib) {
        DeleteObject(g_dib);
        g_dib = nullptr;
    }
    if (g_gdiToken) {
        GdiplusShutdown(g_gdiToken);
        g_gdiToken = 0;
    }
    CursorHide_RestoreAll();
}

void OverlayRequestRedraw() {
    g_needsRedraw = true;
}

HWND OverlayGetHwnd() { return g_hwnd; }
void OverlaySetDpi(UINT dpi) { g_dpi = dpi; }
UINT OverlayGetDpi() { return g_dpi; }