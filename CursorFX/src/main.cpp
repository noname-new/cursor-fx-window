// src/main.cpp
// Không cần #define UNICODE/_UNICODE/OEMRESOURCE ở đây nữa,
// vì build.bat đã truyền -DUNICODE -D_UNICODE -DOEMRESOURCE.
#include <windows.h>
#include "config.h"
#include "overlay.h"
#include "cursor_hide.h"
#include "settings.h"

// ------------------------------------------------------------------
// MinGW-w64 cũ thiếu khai báo SetProcessDpiAwarenessContext.
// Ta tải động từ user32.dll và tự định nghĩa hằng số cần thiết.
// Giá trị (HANDLE)-4 là DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
// theo tài liệu chính thức của Microsoft.
// ------------------------------------------------------------------
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif

typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);

static void EnablePerMonitorDpiAwareness() {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32) return;
    auto pfn = (PFN_SetProcessDpiAwarenessContext)
        GetProcAddress(user32, "SetProcessDpiAwarenessContext");
    if (pfn) {
        pfn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }
}

// ------------------------------------------------------------------
// Biến toàn cục
// ------------------------------------------------------------------
static wchar_t g_iniPath[MAX_PATH]   = {0};
static wchar_t g_backupDir[MAX_PATH] = {0};

static void InitBackupDir() {
    wchar_t temp[MAX_PATH] = {0};
    GetTempPathW(MAX_PATH, temp);
    swprintf_s(g_backupDir, L"%sCursorFX_backup", temp);
}

// ------------------------------------------------------------------
// Watchdog: nếu overlay biến mất bất thường thì khôi phục cursor
// ------------------------------------------------------------------
static DWORD WINAPI Watchdog(LPVOID) {
    while (true) {
        Sleep(2000);
        if (CursorHide_IsApplied()) {
            HWND h = OverlayGetHwnd();
            if (!h || !IsWindow(h)) {
                CursorHide_RestoreAll();
                break;
            }
        }
    }
    return 0;
}

// ------------------------------------------------------------------
// Entry point
// ------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
    // 1. Bật DPI awareness trước khi tạo bất kỳ cửa sổ nào
    EnablePerMonitorDpiAwareness();

    // 2. Nạp cấu hình
    ConfigGetIniPath(g_iniPath, MAX_PATH);
    ConfigLoad(g_iniPath);

    // 3. Chuẩn bị thư mục backup cursor
    InitBackupDir();
    CursorHide_Init(g_backupDir);

    // 4. Khởi tạo overlay (cửa sổ layered + GDI+)
    if (!OverlayInit(hInst)) {
        MessageBoxW(nullptr, L"Không thể khởi tạo overlay.", L"Cursor FX",
                    MB_ICONERROR | MB_OK);
        return 1;
    }

    // 5. Watchdog thread
    CreateThread(nullptr, 0, Watchdog, nullptr, 0, nullptr);

    // 6. Hotkeys
    HWND overlay = OverlayGetHwnd();
    RegisterHotKey(overlay, 1, MOD_CONTROL | MOD_ALT, 'E'); // Mở Settings
    RegisterHotKey(overlay, 2, MOD_CONTROL | MOD_ALT, 'Q'); // Thoát

    // 7. Nếu người dùng đã bật hideCursor từ lần trước, áp dụng ngay
    if (g_cfg.hideCursor) {
        CursorHide_Apply();
    }

    // 8. Message loop
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_HOTKEY) {
            if (msg.wParam == 1) {
                SettingsOpen(nullptr);
            } else if (msg.wParam == 2) {
                PostQuitMessage(0);
            }
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 9. Dọn dẹp
    CursorHide_RestoreAll();
    OverlayShutdown();
    return (int)msg.wParam;
}
