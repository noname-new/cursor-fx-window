@echo off
setlocal

set SRC=src\main.cpp src\config.cpp src\motion.cpp src\overlay.cpp src\hover_detect.cpp src\cursor_hide.cpp src\settings.cpp
set OUT=CursorFX.exe

echo [1/2] Compiling...
g++ -std=c++17 -O2 -municode -mwindows ^
    -DUNICODE -D_UNICODE -DOEMRESOURCE ^
    %SRC% ^
    -o %OUT% ^
    -lgdiplus -lcomctl32 -lcomdlg32 -lole32 -loleaut32 -luuid -lshlwapi -luiautomationcore

if %ERRORLEVEL% NEQ 0 (
    echo Build FAILED.
    pause
    exit /b 1
)

echo [2/2] Done: %OUT%
echo Chay %OUT% de bat dau. Ctrl+Alt+S de mo Settings, Ctrl+Alt+Q de thoat.
pause
