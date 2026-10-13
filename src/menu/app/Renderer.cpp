#include "Renderer.hpp"
#include <cmath>
#include <raylib.h>
#include <rlgl.h>

namespace zappy {

Renderer &Renderer::instance() {
    static Renderer inst;
    return inst;
}

void Renderer::init(int w, int h, const char *title) {
    m_width = w;
    m_height = h;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(w, h, title);
    SetTargetFPS(60);

    m_fontDisplay = GetFontDefault();
    m_fontMono = GetFontDefault();
    m_fontBody = GetFontDefault();
}

void Renderer::shutdown() {
    UnloadFont(m_fontDisplay);
    UnloadFont(m_fontMono);
    UnloadFont(m_fontBody);
    CloseWindow();
}

Font &Renderer::fontDisplay() { return m_fontDisplay; }
Font &Renderer::fontMono() { return m_fontMono; }
Font &Renderer::fontBody() { return m_fontBody; }
const Theme &Renderer::theme() const { return m_theme; }
int Renderer::width() const { return GetScreenWidth(); }
int Renderer::height() const { return GetScreenHeight(); }

void Renderer::drawTextDisplay(const std::string &text, float x, float y, float size, Color color) {
    DrawTextEx(m_fontDisplay, text.c_str(), {x, y}, size, size * 0.05f, color);
}
void Renderer::drawTextMono(const std::string &text, float x, float y, float size, Color color) {
    DrawTextEx(m_fontMono, text.c_str(), {x, y}, size, size * 0.02f, color);
}
void Renderer::drawTextBody(const std::string &text, float x, float y, float size, Color color) {
    DrawTextEx(m_fontBody, text.c_str(), {x, y}, size, size * 0.02f, color);
}

void Renderer::drawRoundedRect(Rectangle rec, float radius, Color color) {
    DrawRectangleRounded(rec, radius / std::max(rec.width, rec.height) * 2.0f, 8, color);
}

void Renderer::drawRoundedRectLines(Rectangle rec, float radius, float thick, Color color) {
    (void)thick;
    DrawRectangleRoundedLines(rec, radius / std::max(rec.width, rec.height) * 2.0f, 8, color);
}

void Renderer::drawGlow(float cx, float cy, float r, Color color, int steps) {
    for (int i = steps; i >= 1; --i) {
        float t = (float)i / steps;
        float alpha = (1.0f - t) * 0.15f;
        Color c = {color.r, color.g, color.b, (unsigned char)(alpha * 255)};
        DrawCircle((int)cx, (int)cy, r * t * 2.0f, c);
    }
}

} // namespace zappy
