#include "TitleBar.hpp"
#include "../app/Renderer.hpp"
#include <cmath>
#include <raylib.h>

static void fillR(Rectangle r, float rad, Color c) {
    float mx = std::max(r.width, r.height);
    DrawRectangleRounded(r, mx > 0 ? std::min(rad / mx * 2.0f, 1.0f) : 0.0f, 8, c);
}
static void strokeR(Rectangle r, float rad, float t, Color c) {
    (void)t;
    float mx = std::max(r.width, r.height);
    DrawRectangleRoundedLines(r, mx > 0 ? std::min(rad / mx * 2.0f, 1.0f) : 0.0f, 8, c);
}

namespace zappy {

void TitleBar::update(AppState &state) {
    float sw = (float)GetScreenWidth();

    struct Btn {
        Rectangle r;
    };
    Btn close_btn = {{sw - 24.0f, 7.0f, 28.0f, 22.0f}};

    if (CheckCollisionPointRec(GetMousePosition(), close_btn.r) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        state.running = false;
    }
}

void TitleBar::draw() const {
    float sw = (float)GetScreenWidth();
    auto &R = Renderer::instance();
    auto &th = R.theme();

    DrawRectangle(0, 0, (int)sw, (int)HEIGHT, th.bgSidebar);
    DrawLineEx({0.0f, HEIGHT}, {sw, HEIGHT}, 1.0f, th.borderCol);

    struct BtnDef {
        Rectangle r;
        const char *icon;
        bool isClose;
    };
    BtnDef btns[] = {
        {{sw - 88.0f, 7.0f, 28.0f, 22.0f}, "-", false},
        {{sw - 56.0f, 7.0f, 28.0f, 22.0f}, "[]", false},
        {{sw - 24.0f, 7.0f, 28.0f, 22.0f}, "x", true},
    };

    Vector2 mp = GetMousePosition();
    for (auto &b : btns) {
        bool hov = CheckCollisionPointRec(mp, b.r);
        Color bg, border, tc;
        if (b.isClose && hov) {
            bg = {239, 68, 68, 76};
            border = {239, 68, 68, 127};
            tc = {248, 113, 113, 255};
        } else {
            bg = hov ? Color{255, 255, 255, 25} : Color{255, 255, 255, 13};
            border = th.borderCol;
            tc = hov ? th.textPrimary : th.textSub;
        }
        fillR(b.r, 4.0f, bg);
        strokeR(b.r, 4.0f, 1.0f, border);

        float tw = MeasureTextEx(R.fontBody(), b.icon, 11.0f, 0.0f).x;
        DrawTextEx(R.fontBody(), b.icon, {b.r.x + (b.r.width - tw) * 0.5f, b.r.y + 5.0f}, 11.0f,
                   0.0f, tc);
    }
}

} // namespace zappy
