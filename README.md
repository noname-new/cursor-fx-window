# Cursor FX for Windows (Experimental)

> ⚠️ **THIS IS AN EXPERIMENTAL BUILD.** Not a finished product. Not recommended for daily use. May break your system cursor, crash, or hang. Use at your own risk.

[![Status](https://img.shields.io/badge/status-experimental-red.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows%2010%20%7C%2011-blue.svg)](https://www.microsoft.com/windows)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-orange.svg)](https://isocpp.org/)
[![Build](https://img.shields.io/badge/build-MinGW--w64%20%7C%20MSVC-green.svg)](https://www.mingw-w64.org/)
[![License](https://img.shields.io/badge/license-MIT-lightgrey.svg)](LICENSE)

---

## 🚨 Important Warning

This is a **personal experimental project**, written to learn and prototype custom cursor ideas on Windows desktop. It is **not** a stable product. Specifically:

- **Your system cursor may get stuck in a transparent state** if the app crashes or is killed abruptly. Although there is a recovery mechanism (marker file + watchdog), nothing is 100% guaranteed — especially on power loss or if Windows restores the cursor mid-session.
- **May conflict with other cursor-modifying software** (CursorFX, Stardock, gaming mouse drivers, etc.).
- **Performance is not optimized** for every configuration. On weak machines it may stutter or eat CPU.
- **The UIA API may be slow** on some applications (especially browsers with many tabs).
- **No auto-update, no code signing.** Windows SmartScreen will warn on first run.
- **Not thoroughly tested** on every Windows 10/11 build, every multi-monitor setup, or every DPI configuration.

If you just want a pretty cursor for daily use, use more stable solutions like **CursorFX**, **Custom Cursor**, or simply install a pre-made `.cur`/`.ani` cursor pack.

If you're curious and want to experiment, read on.

---

## 📖 Introduction

**Cursor FX for Windows** is a lightweight overlay that replaces the default mouse cursor with a custom dot + ring effect. Inspired by [cursor-fx-userscript](https://github.com/Xinyang-Gao/cursor-fx-userscript), but running **system-wide** on the Windows desktop.

The app does not hook, does not install drivers, and does not touch other processes — it simply creates a transparent, click-through layered window covering the entire virtual screen and draws the cursor with GDI+.

![Demo](docs/demo.gif)

> *Demo: delayed ring follow, shape morphing on buttons, spring scale on click.*

---

## ✨ Features (tested)

### Visual effects
- **Dot** follows the mouse instantly, **ring** lags behind with speed increasing with distance.
- **Fade with distance**: the farther the ring is from the dot, the more transparent it becomes.
- **Spring physics** on click: dot scales up, ring scales down, with a slight overshoot — as soft as a CSS spring.
- **Shape morphing**: the ring automatically becomes a rounded rectangle when hovering buttons/links, a vertical bar when hovering text inputs.
- **Soft golden fill** appears inside the ring on click or hover.

### Technical
- **Frame-rate independent** — uses `QueryPerformanceCounter` + exponential smoothing, no stutter when FPS changes.
- **No per-frame DIB allocation** — buffer is reused, only redrawn when needed.
- **Multi-monitor support** — covers the entire virtual screen, handles `WM_DISPLAYCHANGE`.
- **DPI-aware** — calls `SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)` via dynamic loading (compatible with older MinGW).
- **Two-tier hover detection**:
  - Fast tier: `WindowFromPoint` + `GetClassName` + `WS_TABSTOP` — runs every frame.
  - Accurate tier: **UI Automation** (`IUIAutomation::ElementFromPoint`) — throttled to 80ms, falls back to heuristic if UIA fails.
- **INI-based configuration** — no rebuild needed to change colors/sizes.

---

## 🛠️ System requirements

- **Windows 10 1809+** or **Windows 11**.
- **MinGW-w64** (tested with g++ 14.2.0) or **MSVC 2019+**.
- No external runtime (no .NET, no WebView2, no VC++ redistributable if built statically).

---

## 🚀 Build from source

### Option 1: Build directly with `g++` (recommended for MinGW-w64)

```bat
git clone https://github.com/noname-new/cursor-fx-window.git
cd cursor-fx-window
build.bat
```

`build.bat` runs:

```bat
g++ -std=c++17 -O2 -municode -mwindows ^
    -DUNICODE -D_UNICODE -DOEMRESOURCE ^
    src\main.cpp src\config.cpp src\motion.cpp ^
    src\overlay.cpp src\hover_detect.cpp ^
    src\cursor_hide.cpp src\settings.cpp ^
    -o CursorFX.exe ^
    -lgdiplus -lcomctl32 -lcomdlg32 -lole32 -loleaut32 -luuid -lshlwapi -luiautomationcore
```

### Option 2: Build with CMake

```bat
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

### Option 3: Build with MSVC (Visual Studio)

Open the folder in Visual Studio 2022, it will detect `CMakeLists.txt` automatically. Or use the "Open Folder" workflow.

---

## ▶️ Running

```bat
CursorFX.exe
```

The overlay appears immediately, but **the system cursor is NOT hidden by default** (safety). To hide it:

1. Press `Ctrl+Alt+S` to open Settings.
2. Check **"Hide system cursor"**.
3. Click **Apply**.

> ⚠️ Remember to click **"Restore cursor"** in Settings before closing the app, or rely on the watchdog. If the app crashes, the marker file `%TEMP%\CursorFX_backup\applied.marker` will trigger recovery on next launch.

### Hotkeys

| Hotkey | Action |
|--------|--------|
| `Ctrl+Alt+S` | Open Settings |
| `Ctrl+Alt+Q` | Quit (restores cursor) |

---

## ⚙️ Configuration

Settings are saved to `cursorfx.ini` next to the executable. You can edit it directly:

```ini
[CursorFX]
Version=1
EnableDot=1
EnableRing=1
HideCursor=0
DotSize=8
RingSize=40
RingBorder=1.5
RingAlpha=0.9
FitPadding=6
MaxFitSize=200
FollowSpeed=16
FitSpeed=26
ShapeSpeed=18
ClickScale=0.78
SpringK=520
SpringDamp=21
DotColor=#D4AF37
RingColor=#D4AF37
RingFill=#D4AF37
RingFillAlpha=0.12
UseUIA=1
UIAThrottleMs=80
```

### Notes on config

- **`HideCursor=0` by default** — for safety. Set to `1` only if you understand the risk.
- **`UseUIA=1`** — enables UI Automation for accurate hover detection in browsers. Set to `0` to disable if you notice lag.
- **`UIAThrottleMs=80`** — how often (ms) UIA is queried. Increase to 150–200 if UIA is slow on your machine.
- **`BlendMode`** — currently **not implemented** (GDI+ doesn't support `difference` blend). Kept in config for future Direct2D migration.

---

## 🧱 Architecture

```
src/
├── main.cpp           # WinMain, message loop, hotkeys, watchdog
├── config.h/.cpp      # Config struct, INI read/write
├── motion.h/.cpp      # Spring physics, exponential smoothing, state machine
├── overlay.h/.cpp     # Layered window, GDI+ rendering, DIB cache
├── hover_detect.h/.cpp# Heuristic + UIA hover detection
├── cursor_hide.h/.cpp # Safe cursor hide/restore with marker file
└── settings.h/.cpp    # Win32 settings dialog
```

---

## ⚠️ Known limitations

- **`blendMode: difference`** is not implemented. Requires Direct2D. Planned for future.
- **`border-radius` in browsers** cannot be read via UIA. The ring approximates corner radius by aspect ratio (square → circle, rectangle → small radius). For pixel-perfect border-radius, would need DevTools Protocol injection — out of scope.
- **Multi-monitor with different DPI** — the overlay covers the whole virtual screen with a single DIB. Ring size may not scale perfectly per-monitor. Planned for future.
- **No code signing** — SmartScreen warning on first run.
- **Cursor restore on power loss** — impossible without a driver. Marker file helps on normal crash, not on hard power-off.

---

## 🤝 Contributing

This is a personal experiment. Issues and PRs are welcome at
[github.com/noname-new/cursor-fx-window/issues](https://github.com/noname-new/cursor-fx-window/issues),
but **no guarantee of response time**. If you find a bug that breaks your cursor, please include:

- Windows version (`winver`)
- Steps to reproduce
- Whether the cursor was hidden at the time
- Contents of `%TEMP%\CursorFX_backup\`

---

## 📄 License

MIT License. See [LICENSE](LICENSE).

---

## 🙏 Credits

- Inspired by [cursor-fx-userscript](https://github.com/Xinyang-Gao/cursor-fx-userscript) by Xinyang Gao.
- Built with Win32, GDI+, UI Automation, and a lot of trial-and-error.

---

## ⭐ Final reminder

**This is an experimental build.** If it breaks your cursor, don't blame me — read the recovery section, reboot, or run `SystemParametersInfo(SPI_SETCURSORS)` via a small script to restore. Use at your own risk.

