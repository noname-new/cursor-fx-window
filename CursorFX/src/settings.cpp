#include "settings.h"
#include "config.h"
#include "cursor_hide.h"
#include <commctrl.h>
#include <commdlg.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

extern Config g_cfg;
static HWND g_hSettings = nullptr;
static wchar_t g_iniPath[MAX_PATH];

// Control IDs
enum {
    IDC_TAB = 100, IDC_DOT_ENABLE, IDC_RING_ENABLE, IDC_HIDE_CURSOR,
    IDC_DOT_SIZE, IDC_RING_SIZE, IDC_RING_BORDER, IDC_RING_ALPHA,
    IDC_FOLLOW_SPEED, IDC_FIT_SPEED, IDC_CLICK_SCALE,
    IDC_DOT_COLOR, IDC_RING_COLOR, IDC_RING_FILL, IDC_RING_FILL_ALPHA,
    IDC_USE_UIA, IDC_BTN_APPLY, IDC_BTN_SAVE, IDC_BTN_RESET, IDC_BTN_RESTORE_CURSOR
};

static void UpdateControls(HWND hwnd) {
    CheckDlgButton(hwnd, IDC_DOT_ENABLE, g_cfg.enableDot ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_RING_ENABLE, g_cfg.enableRing ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_HIDE_CURSOR, g_cfg.hideCursor ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_USE_UIA, g_cfg.useUIA ? BST_CHECKED : BST_UNCHECKED);

    SendDlgItemMessage(hwnd, IDC_DOT_SIZE, TBM_SETPOS, TRUE, (LPARAM)g_cfg.dotSize);
    SendDlgItemMessage(hwnd, IDC_RING_SIZE, TBM_SETPOS, TRUE, (LPARAM)g_cfg.ringSize);
    SendDlgItemMessage(hwnd, IDC_RING_BORDER, TBM_SETPOS, TRUE, (LPARAM)(g_cfg.ringBorder * 10));
    SendDlgItemMessage(hwnd, IDC_RING_ALPHA, TBM_SETPOS, TRUE, (LPARAM)(g_cfg.ringAlpha * 100));
    SendDlgItemMessage(hwnd, IDC_FOLLOW_SPEED, TBM_SETPOS, TRUE, (LPARAM)g_cfg.followSpeed);
    SendDlgItemMessage(hwnd, IDC_FIT_SPEED, TBM_SETPOS, TRUE, (LPARAM)g_cfg.fitSpeed);
    SendDlgItemMessage(hwnd, IDC_CLICK_SCALE, TBM_SETPOS, TRUE, (LPARAM)(g_cfg.clickScale * 100));
    SendDlgItemMessage(hwnd, IDC_RING_FILL_ALPHA, TBM_SETPOS, TRUE, (LPARAM)(g_cfg.ringFillAlpha * 100));
}

static void ReadControls(HWND hwnd) {
    g_cfg.enableDot = IsDlgButtonChecked(hwnd, IDC_DOT_ENABLE) == BST_CHECKED;
    g_cfg.enableRing = IsDlgButtonChecked(hwnd, IDC_RING_ENABLE) == BST_CHECKED;
    g_cfg.hideCursor = IsDlgButtonChecked(hwnd, IDC_HIDE_CURSOR) == BST_CHECKED;
    g_cfg.useUIA = IsDlgButtonChecked(hwnd, IDC_USE_UIA) == BST_CHECKED;

    g_cfg.dotSize = (float)SendDlgItemMessage(hwnd, IDC_DOT_SIZE, TBM_GETPOS, 0, 0);
    g_cfg.ringSize = (float)SendDlgItemMessage(hwnd, IDC_RING_SIZE, TBM_GETPOS, 0, 0);
    g_cfg.ringBorder = (float)SendDlgItemMessage(hwnd, IDC_RING_BORDER, TBM_GETPOS, 0, 0) / 10.0f;
    g_cfg.ringAlpha = (float)SendDlgItemMessage(hwnd, IDC_RING_ALPHA, TBM_GETPOS, 0, 0) / 100.0f;
    g_cfg.followSpeed = (float)SendDlgItemMessage(hwnd, IDC_FOLLOW_SPEED, TBM_GETPOS, 0, 0);
    g_cfg.fitSpeed = (float)SendDlgItemMessage(hwnd, IDC_FIT_SPEED, TBM_GETPOS, 0, 0);
    g_cfg.clickScale = (float)SendDlgItemMessage(hwnd, IDC_CLICK_SCALE, TBM_GETPOS, 0, 0) / 100.0f;
    g_cfg.ringFillAlpha = (float)SendDlgItemMessage(hwnd, IDC_RING_FILL_ALPHA, TBM_GETPOS, 0, 0) / 100.0f;
}

static void OnApply(HWND hwnd) {
    ReadControls(hwnd);
    ConfigApplyToOverlay();
    if (g_cfg.hideCursor) CursorHide_Apply();
    else CursorHide_RestoreAll();
}

static void OnSave(HWND hwnd) {
    OnApply(hwnd);
    ConfigSave(g_iniPath);
}

static void OnReset(HWND hwnd) {
    ConfigResetToDefaults();
    UpdateControls(hwnd);
    OnApply(hwnd);
}

static void OnRestoreCursor() {
    CursorHide_RestoreAll();
    g_cfg.hideCursor = false;
    ConfigSave(g_iniPath);
}

static LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        int y = 10;
        auto makeCheck = [&](const wchar_t* text, int id) {
            CreateWindowW(L"BUTTON", text, WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
                10, y, 250, 24, hwnd, (HMENU)(INT_PTR)id, nullptr, nullptr);
            y += 28;
        };
        makeCheck(L"Hiển thị chấm (Dot)", IDC_DOT_ENABLE);
        makeCheck(L"Hiển thị vòng (Ring)", IDC_RING_ENABLE);
        makeCheck(L"Ẩn con trỏ hệ thống (nguy hiểm)", IDC_HIDE_CURSOR);
        makeCheck(L"Dùng UI Automation (chính xác trong browser)", IDC_USE_UIA);

        y += 10;
        auto makeSlider = [&](const wchar_t* text, int id, int min, int max) {
            CreateWindowW(L"STATIC", text, WS_CHILD|WS_VISIBLE, 10, y, 200, 20, hwnd, nullptr, nullptr, nullptr);
            CreateWindowW(L"msctls_trackbar32", L"", WS_CHILD|WS_VISIBLE|TBS_HORZ,
                210, y, 200, 24, hwnd, (HMENU)(INT_PTR)id, nullptr, nullptr);
            SendDlgItemMessage(hwnd, id, TBM_SETRANGE, TRUE, MAKELPARAM(min, max));
            y += 30;
        };
        makeSlider(L"Kích thước chấm", IDC_DOT_SIZE, 2, 24);
        makeSlider(L"Kích thước vòng", IDC_RING_SIZE, 16, 96);
        makeSlider(L"Độ dày viền (x10)", IDC_RING_BORDER, 5, 60);
        makeSlider(L"Độ mờ viền (%)", IDC_RING_ALPHA, 10, 100);
        makeSlider(L"Tốc độ đuổi", IDC_FOLLOW_SPEED, 2, 40);
        makeSlider(L"Tốc độ bám", IDC_FIT_SPEED, 4, 80);
        makeSlider(L"Tỉ lệ nhấn (%)", IDC_CLICK_SCALE, 40, 100);
        makeSlider(L"Độ mờ nền vòng (%)", IDC_RING_FILL_ALPHA, 0, 100);

        y += 10;
        CreateWindowW(L"BUTTON", L"Màu chấm", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            10, y, 120, 28, hwnd, (HMENU)IDC_DOT_COLOR, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Màu viền", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            140, y, 120, 28, hwnd, (HMENU)IDC_RING_COLOR, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Màu nền vòng", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            270, y, 140, 28, hwnd, (HMENU)IDC_RING_FILL, nullptr, nullptr);
        y += 40;

        CreateWindowW(L"BUTTON", L"Áp dụng", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            10, y, 100, 30, hwnd, (HMENU)IDC_BTN_APPLY, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Lưu", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            120, y, 100, 30, hwnd, (HMENU)IDC_BTN_SAVE, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Mặc định", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            230, y, 100, 30, hwnd, (HMENU)IDC_BTN_RESET, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Khôi phục con trỏ", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            340, y, 150, 30, hwnd, (HMENU)IDC_BTN_RESTORE_CURSOR, nullptr, nullptr);
        y += 40;

        UpdateControls(hwnd);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_BTN_APPLY) OnApply(hwnd);
        else if (id == IDC_BTN_SAVE) OnSave(hwnd);
        else if (id == IDC_BTN_RESET) OnReset(hwnd);
        else if (id == IDC_BTN_RESTORE_CURSOR) OnRestoreCursor();
        else if (id == IDC_DOT_COLOR || id == IDC_RING_COLOR || id == IDC_RING_FILL) {
            CHOOSECOLORW cc{};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hwnd;
            static COLORREF custom[16];
            cc.lpCustColors = custom;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (id == IDC_DOT_COLOR) cc.rgbResult = g_cfg.dotColor;
            else if (id == IDC_RING_COLOR) cc.rgbResult = g_cfg.ringColor;
            else cc.rgbResult = g_cfg.ringFillColor;
            if (ChooseColorW(&cc)) {
                if (id == IDC_DOT_COLOR) g_cfg.dotColor = cc.rgbResult;
                else if (id == IDC_RING_COLOR) g_cfg.ringColor = cc.rgbResult;
                else g_cfg.ringFillColor = cc.rgbResult;
            }
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        g_hSettings = nullptr;
        return 0;
    case WM_DESTROY:
        g_hSettings = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void SettingsOpen(HWND parent) {
    if (g_hSettings) { SetForegroundWindow(g_hSettings); return; }

    ConfigGetIniPath(g_iniPath, MAX_PATH);

    WNDCLASSW wc{};
    wc.lpfnWndProc = SettingsProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"CursorFXSettings";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    g_hSettings = CreateWindowExW(
        WS_EX_TOOLWINDOW, L"CursorFXSettings", L"Cursor FX - Cài đặt",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 620,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);
    if (g_hSettings) {
        ShowWindow(g_hSettings, SW_SHOW);
        UpdateWindow(g_hSettings);
    }
}

void SettingsClose() {
    if (g_hSettings) { DestroyWindow(g_hSettings); g_hSettings = nullptr; }
}
