#pragma once
#include <windows.h>

struct MotionInput {
    float mouseX, mouseY;
    bool  pressed;
    bool  hoverActive;
    bool  hoverIsText;
    float hoverX, hoverY;
    float hoverW, hoverH;
    float hoverCornerRadius;   // <-- MỚI
    float dt;
};

struct MotionOutput {
    float ringX, ringY;
    float ringW, ringH, ringR;     // ringR giờ là cornerRadius
    float ringScale;
    float dotX, dotY;
    float dotScale;
    float ringOpacity;
};

class MotionState {
public:
    void Init(float startX, float startY);
    MotionOutput Update(const MotionInput& in);
private:
    bool inited = false;
    float rx=0, ry=0, rw=0, rh=0, rr=0;
    float dotX=0, dotY=0;
    float ringS=1, ringV=0;
    float dotS=1, dotV=0;
};