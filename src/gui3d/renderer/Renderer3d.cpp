#include "../graphical/Graphical3d.hpp"
#include "config.hpp"
#include "world.hpp"
#include <cmath>
#include <raylib.h>

static ::Color toRl(Config::Color c) { return {c.r, c.g, c.b, c.a}; }

static const ::Color RES_COLORS[7] = {
    toRl(Config::RES_FOOD),
    toRl(Config::RES_LINEMATE),
    toRl(Config::RES_DERAUMERE),
    toRl(Config::RES_SIBUR),
    toRl(Config::RES_MENDIANE),
    toRl(Config::RES_PHIRAS),
    toRl(Config::RES_THYSTAME),
};

void Graphical3D::render()
{
    auto bg = Config::BG_COLOR;
    BeginDrawing();
    ClearBackground({bg.r, bg.g, bg.b, bg.a});
    BeginMode3D(m_camera);
    if (m_mapReceived && m_world) {
        renderWorld();
        renderPlayers();
    }
    EndMode3D();
    if (m_mapReceived && m_world) {
        renderPlayerLabels();
        renderHUD();
        renderTeamPanel();
    } else
        renderWaiting();
    renderLog();
    EndDrawing();
}