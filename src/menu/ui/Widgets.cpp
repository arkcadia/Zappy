#include "Widgets.hpp"
#include "../app/Renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

static Color lerpColor(Color a, Color b, float t) {
    return {(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
            (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t)};
}

static void drawRounded(Rectangle r, float radius, Color c) {
    float maxDim = std::max(r.width, r.height);
    float ratio = (maxDim > 0.0f) ? radius / maxDim * 2.0f : 0.0f;
    ratio = std::min(ratio, 1.0f);
    DrawRectangleRounded(r, ratio, 8, c);
}

static void drawRoundedLines(Rectangle r, float radius, float thick, Color c) {
    (void)thick;
    float maxDim = std::max(r.width, r.height);
    float ratio = (maxDim > 0.0f) ? radius / maxDim * 2.0f : 0.0f;
    ratio = std::min(ratio, 1.0f);
    DrawRectangleRoundedLines(r, ratio, 8, c);
}

namespace zappy::ui {

bool Button::update() {
    Vector2 mp = GetMousePosition();
    hovered = CheckCollisionPointRec(mp, bounds);
    pressed = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    return pressed;
}

void Button::draw() const {
    Color bg = style.bgNormal;
    if (hovered)
        bg = style.bgHover;

    drawRounded(bounds, style.radius, bg);

    if (style.borderThick > 0.0f && style.borderColor.a > 0) {
        drawRoundedLines(bounds, style.radius, style.borderThick, style.borderColor);
    }

    if (hovered) {
        Color shimmer = {255, 255, 255, 20};
        Rectangle shimRect = {bounds.x, bounds.y, bounds.width * 0.6f, bounds.height};
        drawRounded(shimRect, style.radius, shimmer);
    }

    auto &R = Renderer::instance();
    Vector2 textSize =
        MeasureTextEx(R.fontMono(), label.c_str(), style.fontSize, style.letterSpacing);
    float tx = bounds.x + (bounds.width - textSize.x) * 0.5f;
    float ty = bounds.y + (bounds.height - textSize.y) * 0.5f;
    DrawTextEx(R.fontMono(), label.c_str(), {tx, ty}, style.fontSize, style.letterSpacing,
               style.textColor);
}

ButtonStyle Button::ghostStyle() {
    ButtonStyle s;
    s.bgNormal = {255, 255, 255, 13};
    s.bgHover = {255, 255, 255, 25};
    s.bgActive = {255, 255, 255, 35};
    s.textColor = {136, 136, 168, 255};
    s.borderColor = {255, 255, 255, 18};
    s.borderThick = 1.0f;
    return s;
}

ButtonStyle Button::iconStyle() {
    ButtonStyle s;
    s.bgNormal = {255, 255, 255, 15};
    s.bgHover = {124, 58, 237, 38};
    s.bgActive = {124, 58, 237, 60};
    s.textColor = {136, 136, 168, 255};
    s.borderColor = {255, 255, 255, 30};
    s.borderThick = 1.0f;
    s.radius = 6.0f;
    return s;
}

void TextInput::update() {
    if (!focused)
        return;

    int key = GetCharPressed();
    while (key > 0) {
        if (numeric) {
            if ((key >= '0' && key <= '9') || key == '-') {
                if ((int)value.size() < maxLength)
                    value += (char)key;
            }
        } else {
            if (key >= 32 && (int)value.size() < maxLength) {
                value += (char)key;
            }
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !value.empty()) {
        value.pop_back();
    }
}

void TextInput::draw() const {
    auto &R = Renderer::instance();
    auto &th = R.theme();

    Color bgCol = focused ? Color{124, 58, 237, 15} : Color{255, 255, 255, 10};
    Color border = focused ? Color{124, 58, 237, 128} : Color{255, 255, 255, 20};

    drawRounded(bounds, 6.0f, bgCol);
    drawRoundedLines(bounds, 6.0f, 1.0f, border);

    std::string display = value.empty() ? placeholder : value;
    Color textCol = value.empty() ? th.textMuted : th.textPrimary;

    float padding = 10.0f;
    BeginScissorMode((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height);
    DrawTextEx(R.fontMono(), display.c_str(),
               {bounds.x + padding, bounds.y + (bounds.height - fontSize) * 0.5f}, fontSize, 0.5f,
               textCol);
    EndScissorMode();

    if (focused) {
        float tw = MeasureTextEx(R.fontMono(), value.c_str(), fontSize, 0.5f).x;
        float cursorX = bounds.x + padding + tw + 1.0f;
        float cy = bounds.y + (bounds.height - fontSize) * 0.5f;
        if ((int)(GetTime() * 2) % 2 == 0) {
            DrawRectangle((int)cursorX, (int)cy, 1, (int)fontSize, th.textPrimary);
        }
    }
}

bool NavItem::update() {
    Vector2 mp = GetMousePosition();
    hovered = CheckCollisionPointRec(mp, bounds);
    bool pressed = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    return pressed;
}

void NavItem::draw() const {
    auto &R = Renderer::instance();
    auto &th = R.theme();

    Color bg = {0, 0, 0, 0};
    if (active)
        bg = {124, 58, 237, 30};
    else if (hovered)
        bg = {255, 255, 255, 8};

    DrawRectangleRec(bounds, bg);

    if (active) {
        DrawRectangle((int)bounds.x, (int)bounds.y, 3, (int)bounds.height, {168, 85, 247, 255});
        DrawRectangle((int)bounds.x, (int)bounds.y, 3, (int)bounds.height, {168, 85, 247, 80});
    }

    Color textCol = (active || hovered) ? th.textPrimary : th.textSub;

    float iconX = bounds.x + 20.0f;
    float textX = iconX + 26.0f;
    float y = bounds.y + (bounds.height - 13.0f) * 0.5f;

    DrawTextEx(R.fontBody(), icon.c_str(), {iconX, y + 1.0f}, 14.0f, 0.0f, textCol);
    DrawTextEx(R.fontMono(), label.c_str(), {textX, y}, 12.0f, 1.0f, textCol);
}

void ProgressBar::draw() const {
    drawRounded(bounds, radius, bgColor);

    if (value > 0.0f) {
        Rectangle fill = {bounds.x, bounds.y, bounds.width * std::min(value, 1.0f), bounds.height};
        Color mid = lerpColor(fillStart, fillEnd, std::min(value, 1.0f));
        drawRounded(fill, radius, mid);

        Color glow = {fillStart.r, fillStart.g, fillStart.b, 60};
        drawRounded(fill, radius, glow);
    }
}

void Panel::draw() const {
    drawRounded(bounds, radius, bgColor);
    drawRoundedLines(bounds, radius, 1.0f, border);
}

void ScrollView::update() {
    Vector2 mp = GetMousePosition();
    if (CheckCollisionPointRec(mp, viewport)) {
        float wheel = GetMouseWheelMove();
        scrollOffset -= wheel * 30.0f;
        float maxScroll = std::max(0.0f, contentHeight - viewport.height);
        scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll);
    }
}

void ScrollView::beginClip() const {
    BeginScissorMode((int)viewport.x, (int)viewport.y, (int)viewport.width, (int)viewport.height);
}

void ScrollView::endClip() const {
    EndScissorMode();

    if (contentHeight > viewport.height) {
        float trackH = viewport.height;
        float thumbH = std::max(20.0f, trackH * (viewport.height / contentHeight));
        float maxScroll = contentHeight - viewport.height;
        float thumbY = viewport.y + (scrollOffset / maxScroll) * (trackH - thumbH);

        Rectangle track = {viewport.x + viewport.width - 4.0f, viewport.y, 3.0f, trackH};
        DrawRectangleRec(track, {255, 255, 255, 10});
        Rectangle thumb = {track.x, thumbY, track.width, thumbH};
        drawRounded(thumb, 2.0f, {124, 58, 237, 76});
    }
}

float ScrollView::getOffset() const { return scrollOffset; }

void drawSectionTitle(const std::string &icon, const std::string &title, float x, float y,
                      float width) {
    auto &R = Renderer::instance();
    auto &th = R.theme();

    DrawTextEx(R.fontBody(), icon.c_str(), {x, y + 1.0f}, 11.0f, 0.0f, th.accent2);

    float iconW = MeasureTextEx(R.fontBody(), icon.c_str(), 11.0f, 0.0f).x + 8.0f;
    DrawTextEx(R.fontMono(), title.c_str(), {x + iconW, y}, 10.0f, 2.0f, th.textMuted);

    float titleW = iconW + MeasureTextEx(R.fontMono(), title.c_str(), 10.0f, 2.0f).x + 12.0f;
    float lineY = y + 5.0f;
    DrawLineEx({x + titleW, lineY}, {x + width, lineY}, 1.0f, th.borderCol);
}

void drawBinaryStatus(const std::string &binary, bool found, float x, float y, float width) {
    Color bg = found ? Color{34, 197, 94, 15} : Color{239, 68, 68, 15};
    Color border = found ? Color{34, 197, 94, 38} : Color{239, 68, 68, 38};
    Color text = found ? Color{74, 222, 128, 255} : Color{248, 113, 113, 255};

    Rectangle rec = {x, y, width, 28.0f};
    drawRounded(rec, 6.0f, bg);
    drawRoundedLines(rec, 6.0f, 1.0f, border);

    auto &R = Renderer::instance();
    DrawTextEx(R.fontMono(), binary.c_str(), {x + 10.0f, y + 8.0f}, 11.0f, 0.5f, text);
}

void drawCmdPreview(const std::vector<std::string> &lines, float x, float y, float width,
                    float height) {
    Rectangle rec = {x, y, width, height};
    drawRounded(rec, 6.0f, {0, 0, 0, 102});
    drawRoundedLines(rec, 6.0f, 1.0f, {255, 255, 255, 18});

    auto &R = Renderer::instance();
    float lineY = y + 10.0f;
    for (const auto &line : lines) {
        DrawTextEx(R.fontMono(), "$", {x + 12.0f, lineY}, 11.0f, 0.5f, {168, 85, 247, 255});

        std::string cmd = line;
        size_t first_space = cmd.find(' ');
        if (first_space == std::string::npos) {
            DrawTextEx(R.fontMono(), cmd.c_str(), {x + 26.0f, lineY}, 11.0f, 0.5f,
                       {226, 232, 240, 255});
        } else {
            std::string bin = cmd.substr(0, first_space);
            std::string args = cmd.substr(first_space + 1);
            DrawTextEx(R.fontMono(), bin.c_str(), {x + 26.0f, lineY}, 11.0f, 0.5f,
                       {226, 232, 240, 255});
            float binW = MeasureTextEx(R.fontMono(), (bin + " ").c_str(), 11.0f, 0.5f).x;

            float ax = x + 26.0f + binW;
            size_t i = 0;
            while (i < args.size()) {
                while (i < args.size() && args[i] == ' ') {
                    ax += 4.0f;
                    ++i;
                }
                if (i >= args.size())
                    break;

                if (args[i] == '-') {
                    size_t end = i + 1;
                    while (end < args.size() && args[end] != ' ')
                        ++end;
                    std::string flag = args.substr(i, end - i);
                    DrawTextEx(R.fontMono(), flag.c_str(), {ax, lineY}, 11.0f, 0.5f,
                               {192, 132, 252, 255});
                    ax += MeasureTextEx(R.fontMono(), flag.c_str(), 11.0f, 0.5f).x;
                    i = end;
                } else {
                    size_t end = i;
                    while (end < args.size() && args[end] != ' ')
                        ++end;
                    while (end < args.size() && args[end] == ' ')
                        ++end;
                    while (end < args.size() && args[end] != ' ')
                        ++end;
                    std::string val = args.substr(i, end - i);
                    DrawTextEx(R.fontMono(), (" " + val).c_str(), {ax, lineY}, 11.0f, 0.5f,
                               {134, 239, 172, 255});
                    ax += MeasureTextEx(R.fontMono(), (" " + val).c_str(), 11.0f, 0.5f).x;
                    i = end;
                }
            }
        }
        lineY += 20.0f;
    }
}

void drawLabel(const std::string &text, const std::string &flag, float x, float y) {
    auto &R = Renderer::instance();
    auto &th = R.theme();
    DrawTextEx(R.fontMono(), text.c_str(), {x, y}, 11.0f, 0.5f, th.textMuted);
    if (!flag.empty()) {
        float tw = MeasureTextEx(R.fontMono(), text.c_str(), 11.0f, 0.5f).x;
        DrawTextEx(R.fontMono(), (" " + flag).c_str(), {x + tw, y}, 10.0f, 0.5f,
                   {168, 85, 247, 178});
    }
}

} // namespace zappy::ui
