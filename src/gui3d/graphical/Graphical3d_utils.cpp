#include "Graphical3d.hpp"
#include "world.hpp"
#include <raylib.h>

static constexpr float TILE_SPACING = 0.625f;

void Graphical3D::drawText(const char *text, int x, int y, int size, Color color) const
{
    if (m_fontLoaded)
        DrawTextEx(m_font, text, {(float)x, (float)y}, (float)size, 1.0f, color);
    else
        DrawText(text, x, y, size, color);
}

Vector3 Graphical3D::tileCenter(uint16_t x, uint16_t y) const
{
    return {
        static_cast<float>(x) * TILE_SPACING,
        0.0f,
        static_cast<float>(y) * TILE_SPACING
    };
}
