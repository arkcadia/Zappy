#include "../graphical/Graphical3d.hpp"
#include "config.hpp"
#include "world.hpp"
#include <algorithm>
#include <cmath>
#include <raylib.h>

static ::Color toRl(Config::Color c) { return {c.r, c.g, c.b, c.a}; }
static constexpr float TILE_SPACING = 0.625f;
static constexpr float TILE_HEIGHT  = 0.2f;

static const ::Color RES_COLORS[7] = {
    toRl(Config::RES_FOOD),
    toRl(Config::RES_LINEMATE),
    toRl(Config::RES_DERAUMERE),
    toRl(Config::RES_SIBUR),
    toRl(Config::RES_MENDIANE),
    toRl(Config::RES_PHIRAS),
    toRl(Config::RES_THYSTAME),
};

void Graphical3D::drawTile(uint16_t x, uint16_t y, const Vector3& center)
{
    if (m_tileModelLoaded) {
        DrawModel(m_tileModel, center, 0.3f, WHITE);
    } else {
        ::Color tileCol = ((x + y) % 2 == 0)
            ? ::Color{30, 80, 40, 255}
            : ::Color{25, 65, 33, 255};
        Vector3 pos = {center.x, center.y - TILE_HEIGHT * 0.5f, center.z};
        DrawCube(pos, TILE_SPACING, TILE_HEIGHT, TILE_SPACING, tileCol);
        DrawCubeWires(pos, TILE_SPACING, TILE_HEIGHT, TILE_SPACING, {0, 0, 0, 15});
    }
}

void Graphical3D::drawResourceCube(float x, float y, float z, float scale, ::Color color)
{
    Vector3 pos = {x, y, z};
    DrawCube(pos, scale, scale, scale, color);
    DrawCubeWires(pos, scale, scale, scale, {255, 255, 255, 60});
}

void Graphical3D::drawTileResources(const Tile& tile, const Vector3& center, float anim)
{
    if (!tile.resources.hasAny())
        return;

    int col = 0, row = 0;
    const int maxCols = 3;
    const float dotSize = 0.07f;
    const float dotGap = 0.11f;
    const float dotY = 0.3f + dotSize * 0.5f + 0.02f * anim;
    const float gridW = (maxCols - 1) * dotGap;
    const float startX = center.x - gridW * 0.5f;
    const float startZ = center.z - dotGap;

    for (uint8_t r = 0; r < static_cast<uint8_t>(ResourceType::Count); r++) {
        uint16_t amt = tile.resources.get(static_cast<ResourceType>(r));
        if (amt == 0)
            continue;
        float dx = startX + col * dotGap;
        float dz = startZ + row * dotGap;
        float scale = dotSize * (1.0f + 0.12f * std::min((int)amt, 5));
        ::Color rc = RES_COLORS[r];

        drawResourceCube(dx, dotY, dz, scale, rc);
        col++;
        if (col >= maxCols) { 
            col = 0; 
            row++; 
        }
    }
}

void Graphical3D::renderWorld()
{
    for (uint16_t y = 0; y < m_gridHeight; y++) {
        for (uint16_t x = 0; x < m_gridWidth; x++) {
            Vector3 center = tileCenter(x, y);

            drawTile(x, y, center);

            float phase = static_cast<float>(x * 7 + y * 13);
            float anim = 0.5f + 0.5f * sinf(m_time * 1.8f + phase * 0.4f);

            const Tile &tile = m_world->getTile(x, y);
            drawTileResources(tile, center, anim);
        }
    }
}