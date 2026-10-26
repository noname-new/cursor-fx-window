// src/cursor_hide.cpp
#include "cursor_hide.h"
#include <shlwapi.h>
#include <cstdio>

#pragma comment(lib, "shlwapi.lib")

// ------------------------------------------------------------------
// Danh sách 13 cursor hệ thống hợp lệ theo Win32 SDK.
// (OCR_HELP, OCR_PIN, OCR_PERSON không tồn tại trong SDK chính thức,
//  nên đã được loại bỏ.)
// ------------------------------------------------------------------
static const DWORD kIds[] = {
    OCR_NORMAL,       // 32512 - mũi tên chuẩn
    OCR_IBEAM,        // 32513 - con trỏ văn bản
    OCR_WAIT,         // 32514 - đồng hồ cát
    OCR_CROSS,        // 32515 - dấu chữ thập
    OCR_UP,           // 32516 - mũi tên lên
    OCR_SIZENWSE,     // 32642 - resize góc NW-SE
    OCR_SIZENESW,     // 32643 - resize góc NE-SW
    OCR_SIZEWE,       // 32644 - resize ngang
    OCR_SIZENS,       // 32645 - resize dọc
    OCR_SIZEALL,      // 32646 - di chuyển
    OCR_NO,           // 32648 - cấm
    OCR_HAND,         // 32649 - bàn tay (link)
    OCR_APPSTARTING   // 32650 - mũi tên + đồng hồ
};

static const int kIdCount = (int)(sizeof(kIds) / sizeof(kIds[0])); // = 13

// ------------------------------------------------------------------
// Trạng thái nội bộ
// ------------------------------------------------------------------
static bool     g_applied   = false;
static HCURSOR  g_saved[13] = { nullptr };
static DWORD    g_savedIds[13] = { 0 };
static int      g_savedCount = 0;
static wchar_t  g_backupDir[MAX_PATH] = { 0 };

// ------------------------------------------------------------------
// Tạo cursor 1x1 hoàn toàn trong suốt
// ------------------------------------------------------------------
static HCURSOR MakeTransparent() {
    BYTE andMask[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    BYTE xorMask[4] = { 0x00, 0x00, 0x00, 0x00 };
    return CreateCursor(GetModuleHandle(nullptr), 0, 0, 1, 1, andMask, xorMask);
}

// ------------------------------------------------------------------
// Khởi tạo: tạo thư mục backup, kiểm tra marker từ lần chạy trước
// ------------------------------------------------------------------
void CursorHide_Init(const wchar_t* backupDir) {
    wcscpy_s(g_backupDir, backupDir);
    CreateDirectoryW(g_backupDir, nullptr);

    // Nếu lần chạy trước bị crash khi đang ẩn cursor,
    // marker vẫn còn -> khôi phục ngay để tránh con trỏ trong suốt.
    wchar_t marker[MAX_PATH];
    swprintf_s(marker, L"%s\\applied.marker", g_backupDir);
    if (GetFileAttributesW(marker) != INVALID_FILE_ATTRIBUTES) {
        CursorHide_RestoreAll();
    }
}

// ------------------------------------------------------------------
// Áp dụng: lưu 13 cursor gốc vào RAM + tạo marker file,
// sau đó thay tất cả bằng cursor trong suốt.
// ------------------------------------------------------------------
void CursorHide_Apply() {
    if (g_applied) return;

    g_savedCount = 0;

    // Lưu handle gốc vào RAM
    for (int i = 0; i < kIdCount; i++) {
        DWORD id = kIds[i];
        HCURSOR h = CopyCursor(LoadCursor(nullptr, MAKEINTRESOURCE(id)));
        if (!h) continue;
        g_saved[g_savedCount]    = h;
        g_savedIds[g_savedCount] = id;
        g_savedCount++;
    }

    // Tạo marker file để biết lần sau có cần khôi phục không
    wchar_t marker[MAX_PATH];
    swprintf_s(marker, L"%s\\applied.marker", g_backupDir);
    HANDLE f = CreateFileW(marker, GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f != INVALID_HANDLE_VALUE) CloseHandle(f);

    // Thay tất cả bằng cursor trong suốt
    HCURSOR t = MakeTransparent();
    if (!t) return;
    for (int i = 0; i < kIdCount; i++) {
        HCURSOR c = CopyCursor(t);
        if (c) SetSystemCursor(c, kIds[i]);
    }
    DestroyCursor(t);

    g_applied = true;
}

// ------------------------------------------------------------------
// Khôi phục toàn bộ cursor gốc
// ------------------------------------------------------------------
void CursorHide_RestoreAll() {
    // Nếu chưa từng áp dụng nhưng vẫn còn marker (do crash lần trước),
    // vẫn phải khôi phục bằng SystemParametersInfo bên dưới.
    bool hadMarker = false;
    wchar_t marker[MAX_PATH];
    swprintf_s(marker, L"%s\\applied.marker", g_backupDir);
    if (GetFileAttributesW(marker) != INVALID_FILE_ATTRIBUTES) {
        hadMarker = true;
    }

    if (!g_applied && !hadMarker) return;

    // 1. Khôi phục từ RAM (nếu có)
    for (int i = 0; i < g_savedCount; i++) {
        if (g_saved[i]) {
            SetSystemCursor(g_saved[i], g_savedIds[i]);
            g_saved[i] = nullptr;
        }
    }
    g_savedCount = 0;

    // 2. Xóa marker
    DeleteFileW(marker);

    // 3. Fallback: nếu RAM không còn handle (do crash lần trước),
    //    dùng SystemParametersInfo để nạp lại cursor mặc định hệ thống.
    if (hadMarker && !g_applied) {
        SystemParametersInfoW(SPI_SETCURSORS, 0, nullptr, SPIF_SENDCHANGE);
    }

    g_applied = false;
}

bool CursorHide_IsApplied() {
    return g_applied;
}