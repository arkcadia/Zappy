#include "Homescreen.hpp"
#include "../app/Renderer.hpp"
#include "Sidebar.hpp"
#include <cmath>
#include <raylib.h>
#include <vector>

static void fillRounded(Rectangle r, float rad, Color c) {
    float mx = std::max(r.width, r.height);
    DrawRectangleRounded(r, mx > 0 ? std::min(rad / mx * 2.0f, 1.0f) : 0.0f, 8, c);
}
static void strokeRounded(Rectangle r, float rad, float t, Color c) {
    (void)t;
    float mx = std::max(r.width, r.height);
    DrawRectangleRoundedLines(r, mx > 0 ? std::min(rad / mx * 2.0f, 1.0f) : 0.0f, 8, c);
}

namespace zappy {

HomeScreen::HomeScreen() {
    m_launchBtn.label = "LAUNCH ALL";
    m_launchBtn.style = ui::ButtonStyle{};

    m_settingsBtn.label = "S";
    m_settingsBtn.style = ui::Button::iconStyle();
}

void HomeScreen::update(AppState &state) {
    m_animTime = (float)GetTime();

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float titleH = 36.0f;
    float bottomH = 72.0f;
    float heroH = sh - titleH - bottomH;
    float contentX = Sidebar::WIDTH;

    m_launchBtn.bounds = {contentX + 32.0f, titleH + heroH - 90.0f, 154.0f, 44.0f};
    m_settingsBtn.bounds = {contentX + 196.0f, titleH + heroH - 90.0f, 44.0f, 44.0f};

    if (m_launchBtn.update()) {
        state.openOverlay(Overlay::Settings);
    }
    if (m_settingsBtn.update()) {
        state.openOverlay(Overlay::Settings);
    }

    if (m_newsVisible) {
        float nx = sw - 28.0f - 260.0f;
        float ny = titleH + heroH - 50.0f - 192.0f;

        Rectangle closeBtn = {nx + 260.0f - 24.0f, ny + 8.0f, 16.0f, 16.0f};
        if (CheckCollisionPointRec(GetMousePosition(), closeBtn) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            m_newsVisible = false;
        }
    }
}

void HomeScreen::draw(const AppState &state) {
    (void)state;
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float titleH = 36.0f;
    float bottomH = 72.0f;
    float heroH = sh - titleH - bottomH;
    float contentX = Sidebar::WIDTH;
    float contentW = sw - contentX;

    drawHeroBackground(contentX, titleH, contentW, heroH);
    drawHeroText(contentX, titleH + heroH);

    if (m_newsVisible) {
        float nx = sw - 28.0f - 260.0f;
        float ny = titleH + heroH - 50.0f - 192.0f;
        drawNewsCard(nx, ny);
    }

    m_launchBtn.draw();
    m_settingsBtn.draw();

    DrawTextEx(Renderer::instance().fontMono(), "S",
               {m_settingsBtn.bounds.x + 14.0f, m_settingsBtn.bounds.y + 13.0f}, 16.0f, 0.0f,
               Renderer::instance().theme().textSub);
}

void HomeScreen::drawHeroBackground(float x, float y, float w, float h) const {
    auto &th = Renderer::instance().theme();

    DrawRectangle((int)x, (int)y, (int)w, (int)h, {14, 10, 24, 255});

    for (int gx = 0; gx < (int)(w / 50); ++gx) {
        float lx = x + gx * 50.0f;
        DrawLineEx({lx, y}, {lx, y + h}, 1.0f, {124, 58, 237, 10});
    }
    for (int gy = 0; gy < (int)(h / 50); ++gy) {
        float ly = y + gy * 50.0f;
        DrawLineEx({x, ly}, {x + w, ly}, 1.0f, {124, 58, 237, 10});
    }

    struct Particle {
        float px, py;
        float alpha;
        float size;
    };
    static const Particle particles[] = {
        {0.20f, 0.30f, 0.6f, 2.0f}, {0.70f, 0.20f, 0.4f, 1.5f}, {0.85f, 0.65f, 0.5f, 2.0f},
        {0.35f, 0.75f, 0.3f, 1.5f}, {0.60f, 0.45f, 0.8f, 3.0f}, {0.10f, 0.60f, 0.4f, 1.5f},
        {0.90f, 0.30f, 0.5f, 2.0f}, {0.45f, 0.10f, 0.6f, 2.0f}, {0.78f, 0.80f, 0.3f, 1.5f},
    };

    float drift = std::sin(m_animTime * 0.5f) * 4.0f;
    for (auto &p : particles) {
        float px = x + p.px * w;
        float py = y + p.py * h + drift;
        float pulse = 0.5f + 0.5f * std::sin(m_animTime + p.px * 10.0f);
        Color col = {167, 139, 250, (unsigned char)(p.alpha * pulse * 200)};
        DrawCircle((int)px, (int)py, p.size, col);
    }

    DrawRectangleGradientH((int)x, (int)y, 80, (int)h, th.bgSidebar, {14, 10, 24, 0});

    DrawRectangleGradientV((int)x, (int)(y + h - 120), (int)w, 120, {14, 10, 24, 0}, th.bgApp);
}

void HomeScreen::drawHeroText(float x, float baseY) const {
    auto &R = Renderer::instance();

    float tx = x + 32.0f;
    float ty = baseY - 130.0f;

    DrawTextEx(R.fontMono(), "ZAPPY", {tx, ty + 16.0f}, 48.0f, 2.0f, WHITE);
    DrawTextEx(R.fontMono(), "WORLD", {tx, ty + 60.0f}, 48.0f, 2.0f, WHITE);
}

void HomeScreen::drawNewsCard(float x, float y) const {
    auto &R = Renderer::instance();
    auto &th = R.theme();

    Rectangle card = {x, y, 260.0f, 192.0f};
    fillRounded(card, 10.0f, {10, 10, 20, 216});
    strokeRounded(card, 10.0f, 1.0f, {124, 58, 237, 64});

    DrawTextEx(R.fontMono(), "LATEST NEWS", {x + 14.0f, y + 12.0f}, 10.0f, 1.5f, th.textSub);

    Vector2 mp = GetMousePosition();
    Rectangle closeHit = {x + 260.0f - 24.0f, y + 8.0f, 16.0f, 16.0f};
    Color closeCol = CheckCollisionPointRec(mp, closeHit) ? th.textPrimary : th.textMuted;
    DrawTextEx(R.fontBody(), "x", {x + 246.0f, y + 11.0f}, 12.0f, 0.0f, closeCol);

    Rectangle thumb = {x, y + 32.0f, 260.0f, 100.0f};
    fillRounded(thumb, 0.0f, {30, 16, 64, 255});

    for (int i = 3; i >= 1; --i) {
        DrawEllipse((int)(x + 156.0f), (int)(y + 72.0f), i * 50, i * 40,
                    {124, 58, 237, (unsigned char)(40 / i)});
    }
    for (int i = 2; i >= 1; --i) {
        DrawEllipse((int)(x + 78.0f), (int)(y + 92.0f), i * 30, i * 25,
                    {88, 28, 135, (unsigned char)(30 / i)});
    }

    Vector2 center = {x + 130.0f, y + 82.0f};
    bool thumbHovered = CheckCollisionPointRec(mp, thumb);
    Color circBg = thumbHovered ? th.accent : Color{255, 255, 255, 38};
    Color circBorder = thumbHovered ? th.accent2 : Color{255, 255, 255, 76};
    DrawCircle((int)center.x, (int)center.y, 18.0f, circBg);
    DrawCircleLines((int)center.x, (int)center.y, 19.0f, circBorder);
    DrawTextEx(R.fontBody(), ">", {center.x - 5.0f, center.y - 7.0f}, 14.0f, 0.0f, WHITE);

    DrawTextEx(R.fontBody(), "Server v1.0 - ready to launch", {x + 14.0f, y + 144.0f}, 12.0f, 0.2f,
               th.textPrimary);

    Color linkCol = CheckCollisionPointRec(mp, {x + 14.0f, y + 162.0f, 130.0f, 14.0f})
                        ? Color{192, 132, 252, 255}
                        : th.accent2;
    DrawTextEx(R.fontMono(), "Configure & Launch >>", {x + 14.0f, y + 164.0f}, 11.0f, 0.8f,
               linkCol);
}

} // namespace zappy
