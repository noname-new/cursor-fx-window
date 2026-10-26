#include "motion.h"
#include "config.h"
#include <cmath>
#include <algorithm>

extern Config g_cfg;

static inline float Clampf(float v, float a, float b) {
    return v < a ? a : (v > b ? b : v);
}

void MotionState::Init(float sx, float sy) {
    rx = dotX = sx;
    ry = dotY = sy;
    rw = rh = g_cfg.ringSize;
    rr = g_cfg.ringSize * 0.5f;
    ringS = dotS = 1.0f;
    ringV = dotV = 0.0f;
    inited = true;
}

MotionOutput MotionState::Update(const MotionInput& in) {
    if (!inited) Init(in.mouseX, in.mouseY);

    float dt = Clampf(in.dt, 0.001f, 0.05f);

    // Mục tiêu
    float tx = in.mouseX, ty = in.mouseY;
    float speed = g_cfg.followSpeed;
    float baseSize = in.hoverIsText ? g_cfg.textRingSize : g_cfg.ringSize;

    // Mặc định: vòng tròn
    float tw = baseSize;
    float th = baseSize;
    float tr = baseSize * 0.5f;

    if (in.hoverActive && in.hoverW > 0 && in.hoverH > 0) {
        // Fit theo hình dạng element
        tx = in.hoverX;
        ty = in.hoverY;
        tw = in.hoverW + g_cfg.fitPadding * 2.0f;
        th = in.hoverH + g_cfg.fitPadding * 2.0f;
        // Corner radius: lấy theo element + padding, clamp không vượt nửa cạnh
        tr = std::min(in.hoverCornerRadius + g_cfg.fitPadding,
                      std::min(tw, th) * 0.5f);
        speed = g_cfg.fitSpeed;
    } else if (in.hoverActive) {
        // Fallback: không có rect -> phóng to nhẹ, vẫn tròn
        float s = g_cfg.ringSize * 1.3f;
        tw = th = s;
        tr = s * 0.5f;
        speed = g_cfg.fitSpeed;
    }

    // Exponential smoothing vị trí
    float k = 1.0f - expf(-speed * dt);
    rx += (tx - rx) * k;
    ry += (ty - ry) * k;
    if (fabsf(tx - rx) < 0.05f) rx = tx;
    if (fabsf(ty - ry) < 0.05f) ry = ty;

    // Smoothing kích thước + corner radius
    float ks = 1.0f - expf(-g_cfg.shapeSpeed * dt);
    rw += (tw - rw) * ks;
    rh += (th - rh) * ks;
    rr += (tr - rr) * ks;
    if (fabsf(tw - rw) < 0.1f) rw = tw;
    if (fabsf(th - rh) < 0.1f) rh = th;
    if (fabsf(tr - rr) < 0.1f) rr = tr;

    // Spring scale
    float pressedTarget = in.pressed ? g_cfg.clickScale : 1.0f;
    float K = g_cfg.springK, D = g_cfg.springDamp;

    ringV = (ringV + (pressedTarget - ringS) * K * dt) * expf(-D * dt);
    ringS = Clampf(ringS + ringV * dt, 0.2f, 4.0f);

    dotX = in.mouseX;
    dotY = in.mouseY;
    dotV = (dotV + (pressedTarget - dotS) * K * dt) * expf(-D * dt);
    dotS = Clampf(dotS + dotV * dt, 0.2f, 4.0f);

    // Opacity giảm khi xa
    float dist = sqrtf((tx - rx)*(tx - rx) + (ty - ry)*(ty - ry));
    float opacity = Clampf(1.0f - dist / 420.0f, 0.15f, 1.0f);

    MotionOutput out;
    out.ringX = rx; out.ringY = ry;
    out.ringW = rw; out.ringH = rh; out.ringR = rr;   // rr = cornerRadius
    out.ringScale = ringS;
    out.dotX = dotX; out.dotY = dotY;
    out.dotScale = dotS;
    out.ringOpacity = opacity;
    return out;
}