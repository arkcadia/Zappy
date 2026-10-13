#include "../graphical/Graphical3d.hpp"
#include "config.hpp"
#include "world.hpp"
#include <cmath>
#include <raylib.h>
#include <algorithm>
#include <string>

static ::Color toRl(Config::Color c) { return {c.r, c.g, c.b, c.a}; }

void Graphical3D::renderHUD()
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, Config::HUD_H, toRl(Config::HUD_BG));
    DrawLine(0, Config::HUD_H, sw, Config::HUD_H, toRl(Config::HUD_BORDER));

    drawText("ZAPPY", 14, 10, 20, toRl(Config::ACCENT_CYAN));
    drawText("3D", 70, 12, 14, toRl(Config::ACCENT_ORANGE));

    float pulse = 0.5f + 0.5f * sinf(m_time * 3.0f);
    uint8_t dotA = static_cast<uint8_t>(160 + 95 * pulse);
    DrawCircle(195, Config::HUD_H / 2, 5.0f, {0, 255, 160, 40});
    DrawCircle(195, Config::HUD_H / 2, 4.0f, {0, 255, 160, dotA});
    drawText("CONNECTED", 206, 18, 11, toRl(Config::ACCENT_GREEN));

    std::string mapStr = std::to_string(m_gridWidth) + "x" + std::to_string(m_gridHeight);
    drawText("MAP", 320, 10, 8, toRl(Config::TEXT_GHOST));
    drawText(mapStr.c_str(), 320, 22, 12, toRl(Config::TEXT_PRIMARY));

    std::string foodStr = "FOOD " + std::to_string(m_totalFood);
    std::string minStr  = "MIN "  + std::to_string(m_totalMinerals);
    drawText(foodStr.c_str(), 410, 10, 10, toRl(Config::RES_FOOD));
    drawText(minStr.c_str(),  410, 26, 10, toRl(Config::RES_LINEMATE));

    std::string plStr = "PLAYERS " + std::to_string(m_world ? m_world->getPlayerCount() : 0);
    drawText(plStr.c_str(), 530, 18, 11, toRl(Config::ACCENT_CYAN));

    std::string fps = std::to_string(GetFPS()) + " FPS";
    drawText(fps.c_str(), sw - 70, 18, 11, toRl(Config::TEXT_DIM));

    DrawRectangle(0, sh - Config::STRIP_H, sw, Config::STRIP_H, toRl(Config::HUD_BG));
    DrawLine(0, sh - Config::STRIP_H, sw, Config::STRIP_H, toRl(Config::HUD_BORDER));
    drawText("[FLECHES] DEPLACER   [LMB] TOURNER   [Z/X] ZOOM   [W/S] MONTER/DESCENDRE   [R] RESET   [ESC] QUITTER",
             12, sh - Config::STRIP_H + 8, 10, toRl(Config::TEXT_GHOST));
}

void Graphical3D::renderWaiting()
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float pulse = 0.5f + 0.5f * sinf(m_time * 2.0f);
    uint8_t a = static_cast<uint8_t>(180 + 75 * pulse);

    drawText("ZAPPY 3D", sw / 2 - 60, sh / 2 - 40, 32, {40, 180, 255, a});
    drawText("EN ATTENTE DU SERVEUR...", sw / 2 - 110, sh / 2 + 10, 14, toRl(Config::TEXT_DIM));
}

void Graphical3D::renderLog()
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int y = sh - Config::STRIP_H - 16;

    for (int i = static_cast<int>(m_log.size()) - 1; i >= 0; i--) {
        const auto &entry = m_log[i];
        uint8_t a = static_cast<uint8_t>(std::clamp(entry.alpha / 6.0f, 0.0f, 1.0f) * 200.0f);

        drawText(entry.text.c_str(), sw - 320, y, 11, {255, 120, 20, a});
        y -= 15;
        if (y < Config::HUD_H + 4) break;
    }
}

void Graphical3D::renderTeamPanel()
{
    if (!m_world) 
        return;

    const auto &teams = m_world->getTeams();
    const auto &players = m_world->getPlayers();

    static const ::Color TEAM_COLORS[] = {
        {255, 80,  80,  255},
        {80,  160, 255, 255},
        {80,  255, 120, 255},
        {255, 200, 60,  255},
        {200, 80,  255, 255},
        {255, 140, 40,  255},
        {60,  220, 255, 255},
        {255, 100, 180, 255},
    };

    int sw = GetScreenWidth();
    int x = sw - 200;
    int y = Config::HUD_H + 10;

    DrawRectangle(x - 8, Config::HUD_H, 208, GetScreenHeight() - Config::HUD_H - Config::STRIP_H, {6, 7, 18, 200});
    DrawLine(x - 8, Config::HUD_H, x - 8, GetScreenHeight() - Config::STRIP_H, toRl(Config::HUD_BORDER));

    for (size_t ti = 0; ti < teams.size(); ti++) {
        ::Color tc = TEAM_COLORS[ti % 8];
        drawText(teams[ti].c_str(), x, y, 12, tc);
        y += 18;
        DrawLine(x, y, sw - 4, y, {tc.r, tc.g, tc.b, 80});
        y += 6;
        for (const auto &[id, player] : players) {
            if (player.teamName != teams[ti] || !player.alive)
                continue;
            std::string line = "  #" + std::to_string(player.id)
                             + "  L" + std::to_string(player.level);
            drawText(line.c_str(), x, y, 10, toRl(Config::TEXT_PRIMARY));
            y += 14;
        }
        y += 8;
    }
}