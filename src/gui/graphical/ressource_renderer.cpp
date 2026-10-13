#include "ressource_renderer.hpp"
#include "config.hpp"
#include <algorithm>
#include <cmath>
#include <string>

static ::Color toRl(Config::Color c) { return {c.r, c.g, c.b, c.a}; }

ResourceRenderer::ResourceRenderer() {
    m_visuals[0] = {toRl(Config::RES_FOOD), "FOOD", "F"};
    m_visuals[1] = {toRl(Config::RES_LINEMATE), "LINEMATE", "L"};
    m_visuals[2] = {toRl(Config::RES_DERAUMERE), "DERAUMERE", "D"};
    m_visuals[3] = {toRl(Config::RES_SIBUR), "SIBUR", "S"};
    m_visuals[4] = {toRl(Config::RES_MENDIANE), "MENDIANE", "M"};
    m_visuals[5] = {toRl(Config::RES_PHIRAS), "PHIRAS", "P"};
    m_visuals[6] = {toRl(Config::RES_THYSTAME), "THYSTAME", "T"};
}

void ResourceRenderer::drawResourceDot(int x, int y, int size, ResourceType type, uint16_t amount,
                                       float pulse) {
    const ResourceVisual &v = m_visuals[static_cast<uint8_t>(type)];

    float baseRadius = static_cast<float>(std::max(2, size / 2));
    float radius = baseRadius * (0.88f + 0.12f * pulse);
    float cx = static_cast<float>(x) + baseRadius;
    float cy = static_cast<float>(y) + baseRadius;

    uint8_t fillA = static_cast<uint8_t>(28 + 22 * pulse);
    uint8_t ringA = static_cast<uint8_t>(160 + 95 * pulse);
    uint8_t glowA = static_cast<uint8_t>(18 + 30 * pulse);

    DrawCircle(static_cast<int>(cx), static_cast<int>(cy), radius * 1.6f,
               {v.color.r, v.color.g, v.color.b, glowA});
    DrawCircle(static_cast<int>(cx), static_cast<int>(cy), radius,
               {v.color.r, v.color.g, v.color.b, fillA});
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), radius,
                    {v.color.r, v.color.g, v.color.b, ringA});

    if (amount > 1 && size >= 10) {
        std::string cnt = std::to_string(amount);
        int fs = std::max(6, size - 4);
        int tw = MeasureText(cnt.c_str(), fs);
        DrawText(cnt.c_str(), static_cast<int>(cx) - tw / 2, static_cast<int>(cy) - fs / 2, fs,
                 {v.color.r, v.color.g, v.color.b, ringA});
    }
}

void ResourceRenderer::drawResourcesOnTile(int px, int py, int tileSize, const Tile &tile,
                                           float time, float phase) {
    if (!tile.resources.hasAny())
        return;

    float pulse = 0.5f + 0.5f * std::sin(time * 1.8f + phase);

    int dotSize = std::max(6, std::min(tileSize / 4, 14));
    int padding = 3;
    int cols = std::max(1, (tileSize - padding) / (dotSize + padding));

    int col = 0;
    int row = 0;

    for (uint8_t i = 0; i < static_cast<uint8_t>(ResourceType::Count); i++) {
        uint16_t amt = tile.resources.get(static_cast<ResourceType>(i));
        if (amt == 0)
            continue;

        int dx = px + padding + col * (dotSize + padding);
        int dy = py + padding + row * (dotSize + padding);

        if (dy + dotSize > py + tileSize)
            break;

        drawResourceDot(dx, dy, dotSize, static_cast<ResourceType>(i), amt, pulse);

        col++;
        if (col >= cols) {
            col = 0;
            row++;
        }
    }
}
