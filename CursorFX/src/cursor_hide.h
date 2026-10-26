// src/cursor_hide.h
#pragma once
#include <windows.h>

void CursorHide_Init(const wchar_t* backupDir);
void CursorHide_Apply();
void CursorHide_RestoreAll();
bool CursorHide_IsApplied();