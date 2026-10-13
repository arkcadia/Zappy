#include "LaunchModal.hpp"
#include "../app/Renderer.hpp"
#include "../config/ConfigManager.hpp"
#include "../launcher/LauncherManager.hpp"
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

void LaunchModal::update(AppState &state) {
    auto &lm = LaunchManager::instance();
    lm.update(GetFrameTime());

    if (lm.isComplete()) {
        if (!state.progressRunning) {
            state.progressRunning = true;
            state.progress = 0.0f;
        }
    }

    if (state.progressRunning && !state.paused) {
        state.progress += GetFrameTime() * 0.04f;
        if (state.progress > 1.0f) {
            state.progress = 1.0f;
            state.progressRunning = false;
        }
    }

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float mw = 540.0f;
    float mh = 300.0f;
    float mx = (sw - mw) * 0.5f;
    float my = (sh - mh) * 0.5f;

    if (lm.isComplete()) {
        Rectangle closeBtn = {mx + mw - 120.0f, my + mh - 52.0f, 100.0f, 36.0f};
        if (CheckCollisionPointRec(GetMousePosition(), closeBtn) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            state.closeOverlay();
            lm.reset();
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(GetMousePosition(), {mx, my, mw, mh})) {
        if (lm.isComplete()) {
            state.closeOverlay();
            lm.reset();
        }
    }
}

void LaunchModal::draw(const AppState &state) const {
    (void)state;
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    DrawRectangle(0, 0, (int)sw, (int)sh, {0, 0, 0, 204});

    float mw = 540.0f;
    float mh = 300.0f;
    float mx = (sw - mw) * 0.5f;
    float my = (sh - mh) * 0.5f;

    fillR({mx, my, mw, mh}, 12.0f, {10, 10, 20, 255});
    strokeR({mx, my, mw, mh}, 12.0f, 1.0f, {124, 58, 237, 102});

    for (int i = 3; i >= 1; --i) {
        DrawRectangleRoundedLines({mx - i * 4.0f, my - i * 4.0f, mw + i * 8.0f, mh + i * 8.0f},
                                  12.0f / std::max(mw + i * 8.0f, mh + i * 8.0f) * 2.0f, 1.0f,
                                  {124, 58, 237, (unsigned char)(15 / i)});
    }

    auto &R = Renderer::instance();
    auto &th = R.theme();

    DrawTextEx(R.fontMono(), "// LAUNCH SEQUENCE", {mx + 20.0f, my + 20.0f}, 13.0f, 1.5f,
               th.accent2);
    DrawLineEx({mx + 20.0f, my + 44.0f}, {mx + mw - 20.0f, my + 44.0f}, 1.0f, th.borderCol);

    auto &log = LaunchManager::instance().log();
    float ly = my + 56.0f;

    for (auto &entry : log) {
        if (!entry.visible)
            continue;

        Color textCol = th.textSub;
        std::string prefix = "  ";

        switch (entry.type) {
            using T = zappy::LaunchStep::Type;
        case T::Ok:
            textCol = {74, 222, 128, 255};
            prefix = "✓ ";
            break;
        case T::Wait:
            textCol = {251, 191, 36, 255};
            prefix = "○ ";
            break;
        case T::Done:
            textCol = {167, 139, 250, 255};
            prefix = "★ ";
            break;
        case T::Error:
            textCol = {248, 113, 113, 255};
            prefix = "! ";
            break;
        case T::Info:
            textCol = th.textSub;
            prefix = "  ";
            break;
        }

        DrawTextEx(R.fontMono(), (prefix + entry.message).c_str(), {mx + 20.0f, ly}, 12.0f, 0.5f,
                   textCol);
        ly += 22.0f;
    }

    if (LaunchManager::instance().isComplete()) {
        Rectangle closeBtn = {mx + mw - 120.0f, my + mh - 52.0f, 100.0f, 36.0f};
        bool closeHov = CheckCollisionPointRec(GetMousePosition(), closeBtn);
        fillR(closeBtn, 6.0f, closeHov ? Color{109, 40, 217, 255} : th.accent);
        DrawTextEx(R.fontMono(), "CLOSE", {closeBtn.x + 28.0f, closeBtn.y + 11.0f}, 12.0f, 1.0f,
                   WHITE);
    }
}

} // namespace zappy
