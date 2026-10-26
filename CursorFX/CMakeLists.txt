cmake_minimum_required(VERSION 3.20)
project(CursorFX CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE -DOEMRESOURCE)
add_executable(CursorFX WIN32
    src/main.cpp
    src/config.cpp
    src/motion.cpp
    src/overlay.cpp
    src/hover_detect.cpp
    src/cursor_hide.cpp
    src/settings.cpp
)
target_link_libraries(CursorFX PRIVATE
    gdiplus comctl32 comdlg32 ole32 oleaut32 uuid shlwapi uiautomationcore
)
