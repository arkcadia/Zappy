#include "BottomBar.hpp"
#include "../app/Renderer.hpp"
#include "../launcher/LauncherManager.hpp"
#include <cmath>
#include <raylib.h>
#include <string>

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

void BottomBar::update(AppState &state) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float barY = sh - HEIGHT;

    Rectangle pauseBtn = {sw - 56.0f, barY + (HEIGHT - 32.0f) * 0.5f, 32.0f, 32.0f};
    if (CheckCollisionPointRec(GetMousePosition(), pauseBtn) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        state.paused = !state.paused;
    }
}

void BottomBar::draw(const AppState &state) const {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float barY = sh - HEIGHT;

    DrawRectangle(0, (int)barY, (int)sw, (int)HEIGHT, {8, 8, 14, 242});
    DrawLineEx({0, barY}, {sw, barY}, 1.0f, Renderer::instance().theme().borderCol);

    auto &R = Renderer::instance();
    auto &th = R.theme();

    float cy = barY + HEIGHT * 0.5f;

    float px = 24.0f;
    fillR({px, cy - 12.0f, 80.0f, 24.0f}, 4.0f, {255, 255, 255, 8});
    strokeR({px, cy - 12.0f, 80.0f, 24.0f}, 4.0f, 1.0f, th.borderCol);
    DrawTextEx(R.fontMono(), "SERVER v1.0", {px + 6.0f, cy - 7.0f}, 10.0f, 1.0f, th.textSub);

    px += 96.0f;
    std::string statusText = "Ready";
    if (LaunchManager::instance().isRunning()) {
        if (LaunchManager::instance().isComplete()) {
            statusText = state.paused
                             ? "Paused"
                             : (state.progress >= 1.0f ? "Processes running" : "Running...");
        } else {
            statusText = "Launching...";
        }
    }
    DrawTextEx(R.fontBody(), statusText.c_str(), {px, cy - 6.0f}, 12.0f, 0.3f, th.textSub);

    float progressX = px + 120.0f;
    float progressW = sw - progressX - 100.0f;

    Rectangle progressBg = {progressX, cy - 2.0f, progressW, 5.0f};
    fillR(progressBg, 3.0f, {255, 255, 255, 15});

    if (state.progress > 0.0f) {
        float fillW = progressW * std::min(state.progress, 1.0f);
        Rectangle fillRect = {progressX, cy - 2.0f, fillW, 5.0f};

        Color fillA = th.accent;
        Color fillB = th.accent2;

        float t = std::min(state.progress, 1.0f);
        Color mid = {(unsigned char)(fillA.r + (fillB.r - fillA.r) * t),
                     (unsigned char)(fillA.g + (fillB.g - fillA.g) * t),
                     (unsigned char)(fillA.b + (fillB.b - fillA.b) * t), 255};
        fillR(fillRect, 3.0f, mid);
        fillR(fillRect, 3.0f, {mid.r, mid.g, mid.b, 50});
    }

    std::string progressLabel =
        state.progress > 0.0f ? "Processes active" : "Waiting for launch...";
    std::string progressRight = state.progress > 0.0f ? "Server * AI * GUI" : "";
    DrawTextEx(R.fontMono(), progressLabel.c_str(), {progressX, cy + 7.0f}, 10.0f, 0.5f,
               th.textMuted);
    if (!progressRight.empty()) {
        float rw = MeasureTextEx(R.fontMono(), progressRight.c_str(), 10.0f, 0.5f).x;
        DrawTextEx(R.fontMono(), progressRight.c_str(), {progressX + progressW - rw, cy + 7.0f},
                   10.0f, 0.5f, th.textMuted);
    }

    std::string pctStr =
        state.progress > 0.0f ? std::to_string((int)(state.progress * 100)) + "%" : "--";
    DrawTextEx(R.fontMono(), pctStr.c_str(), {sw - 88.0f, cy - 8.0f}, 16.0f, 0.5f, th.textSub);

    Rectangle pauseBtn = {sw - 56.0f, cy - 16.0f, 32.0f, 32.0f};
    bool pauseHov = CheckCollisionPointRec(GetMousePosition(), pauseBtn);
    fillR(pauseBtn, 6.0f, pauseHov ? Color{124, 58, 237, 38} : Color{255, 255, 255, 13});
    strokeR(pauseBtn, 6.0f, 1.0f, pauseHov ? Color{124, 58, 237, 102} : th.borderCol);

    const char *pauseIcon = state.paused ? ">" : "||";
    float iconW = MeasureTextEx(R.fontBody(), pauseIcon, 13.0f, 0.0f).x;
    DrawTextEx(R.fontBody(), pauseIcon, {pauseBtn.x + (32.0f - iconW) * 0.5f, pauseBtn.y + 8.0f},
               13.0f, 0.0f, pauseHov ? th.accent2 : th.textSub);
}

} // namespace zappy
