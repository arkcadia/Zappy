#include "Sidebar.hpp"
#include "../app/Renderer.hpp"
#include "../config/ConfigManager.hpp"
#include <cmath>
#include <raylib.h>

static void drawRoundedFill(Rectangle r, float radius, Color c) {
    float mx = std::max(r.width, r.height);
    DrawRectangleRounded(r, mx > 0 ? std::min(radius / mx * 2.0f, 1.0f) : 0.0f, 8, c);
}
static void drawRoundedBorder(Rectangle r, float radius, float t, Color c) {
    (void)t;
    float mx = std::max(r.width, r.height);
    DrawRectangleRoundedLines(r, mx > 0 ? std::min(radius / mx * 2.0f, 1.0f) : 0.0f, 8, c);
}

namespace zappy {

Sidebar::Sidebar() {
    m_navItems = {
        {"[H]", "HOME", Page::Home, {}},
        {"[>]", "LAUNCH", Page::Launch, {}},
        {"[#]", "LOGS", Page::Logs, {}},
        {"[*]", "CONFIG", Page::Config, {}},
    };
}

void Sidebar::update(AppState &state) {
    float titleH = 36.0f;
    float logoH = 86.0f;
    float itemH = 44.0f;

    for (int i = 0; i < (int)m_navItems.size(); ++i) {
        auto &entry = m_navItems[i];
        entry.widget.bounds = {0.0f, titleH + logoH + (float)i * itemH, WIDTH, itemH};
        entry.widget.icon = entry.icon;
        entry.widget.label = entry.label;
        entry.widget.active = (state.currentPage == entry.page);

        if (entry.widget.update()) {
            state.navigate(entry.page);
            if (entry.page == Page::Launch || entry.page == Page::Config) {
                state.openOverlay(Overlay::Settings);
            } else {
                state.closeOverlay();
            }
        }
    }
}

void Sidebar::draw(const AppState &state) const {
    (void)state;
    auto &R = Renderer::instance();
    auto &th = R.theme();
    float screenH = (float)GetScreenHeight();

    DrawLineEx({WIDTH, 0.0f}, {WIDTH, screenH}, 1.0f, th.borderCol);

    float titleH = 36.0f;
    drawLogo(0.0f, titleH);

    DrawLineEx({0.0f, titleH + 86.0f}, {WIDTH, titleH + 86.0f}, 1.0f, th.borderCol);

    for (auto &entry : m_navItems) {
        entry.widget.draw();
    }

    float socialsY = screenH - 72.0f - 58.0f;
    DrawLineEx({0.0f, socialsY}, {WIDTH, socialsY}, 1.0f, th.borderCol);
    drawSocials(socialsY + 1.0f);

    float userY = screenH - 58.0f;
    DrawLineEx({0.0f, userY}, {WIDTH, userY}, 1.0f, th.borderCol);
    drawUserArea(userY, 58.0f);
}

void Sidebar::drawLogo(float x, float y) const {
    auto &R = Renderer::instance();
    auto &th = R.theme();

    float padX = x + 20.0f;
    float padY = y + 22.0f;

    Rectangle iconRect = {padX, padY, 40.0f, 40.0f};
    Color gradStart = {124, 58, 237, 255};

    drawRoundedFill(iconRect, 10.0f, gradStart);

    for (int i = 3; i >= 0; --i) {
        float alpha = 0.08f * (4 - i);
        Color glow = {168, 85, 247, (unsigned char)(alpha * 255)};
        Rectangle gr = {iconRect.x - i * 3.0f, iconRect.y - i * 3.0f, iconRect.width + i * 6.0f,
                        iconRect.height + i * 6.0f};
        drawRoundedFill(gr, 10.0f + i * 2.0f, glow);
    }
    drawRoundedFill(iconRect, 10.0f, gradStart);

    DrawTextEx(R.fontBody(), "Z", {padX + 12.0f, padY + 9.0f}, 20.0f, 0.0f, WHITE);

    float tx = padX + 40.0f + 12.0f;
    DrawTextEx(R.fontMono(), "ZAPPY", {tx, padY + 2.0f}, 15.0f, 1.2f, th.textPrimary);
    DrawTextEx(R.fontMono(), "LAUNCHER", {tx, padY + 20.0f}, 9.0f, 2.0f, th.textMuted);
}

void Sidebar::drawUserArea(float y, float height) const {
    auto &R = Renderer::instance();
    auto &th = R.theme();
    auto &cfg = ConfigManager::instance().config();

    Vector2 mp = GetMousePosition();
    Rectangle area = {0.0f, y, WIDTH, height};
    if (CheckCollisionPointRec(mp, area)) {
        DrawRectangleRec(area, {255, 255, 255, 8});
    }

    float cx = 16.0f + 17.0f;
    float cy = y + height * 0.5f;

    drawRoundedFill({16.0f, cy - 17.0f, 34.0f, 34.0f}, 17.0f, {30, 27, 75, 255});
    drawRoundedBorder({16.0f, cy - 17.0f, 34.0f, 34.0f}, 17.0f, 2.0f, {124, 58, 237, 102});
    DrawTextEx(R.fontBody(), "U", {cx - 5.0f, cy - 8.0f}, 14.0f, 0.0f, th.textSub);

    float infoX = 58.0f;
    DrawTextEx(R.fontMono(), cfg.username.c_str(), {infoX, y + 14.0f}, 12.0f, 0.8f, th.textPrimary);

    DrawCircle((int)(infoX), (int)(y + 36.0f), 3.0f, th.green);
    DrawCircle((int)(infoX), (int)(y + 36.0f), 3.0f, {34, 197, 94, 80});
    DrawTextEx(R.fontMono(), "Online", {infoX + 9.0f, y + 30.0f}, 10.0f, 0.5f, th.textMuted);

    DrawTextEx(R.fontBody(), "v", {WIDTH - 20.0f, y + height * 0.5f - 5.0f}, 12.0f, 0.0f,
               th.textMuted);
}

void Sidebar::drawSocials(float y) const {
    auto &R = Renderer::instance();
    auto &th = R.theme();

    struct Social {
        const char *icon;
        const char *tip;
    };
    static const Social socials[] = {
        {"G", "GitHub"}, {"D", "Docs"}, {"B", "Bugs"}, {"C", "Discord"}};

    float x = 20.0f;
    float fy = y + 14.0f;

    for (auto &s : socials) {
        Vector2 mp = GetMousePosition();
        Rectangle btn = {x - 2.0f, fy - 2.0f, 20.0f, 20.0f};
        Color col = CheckCollisionPointRec(mp, btn) ? th.accent2 : th.textMuted;
        DrawTextEx(R.fontMono(), s.icon, {x, fy}, 14.0f, 0.0f, col);
        x += 24.0f;
    }
}

} // namespace zappy
