#pragma once
#include <windows.h>

struct HoverInfo {
    bool  active      = false;
    bool  isText      = false;
    float centerX     = 0, centerY = 0;
    float width       = 0, height = 0;
    float cornerRadius = 0;   // <-- MỚI: bán kính bo góc thực tế
};

HoverInfo HoverDetect_Get(POINT screenPt, bool useUIA, int throttleMs);