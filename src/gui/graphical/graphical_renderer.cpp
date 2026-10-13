#include "config.hpp"
#include "graphical.hpp"
#include "ressource_renderer.hpp"
#include "window.hpp"
#include "world.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <raylib.h>
#include <string>
#include <vector>

static ::Color toRl(Config::Color c) { return {c.r, c.g, c.b, c.a}; }
static ::Color withAlpha(Config::Color c, uint8_t a) { return {c.r, c.g, c.b, a}; }
static void drawRect(float x, float y, float w, float h, ::Color c) {
    DrawRectangleRec({x, y, w, h}, c);
}
static void drawSectionBar(int x, int y, int h, ::Color col) { DrawRectangle(x, y, 2, h, col); }
static float easeOut(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return 1.0f - (1.0f - t) * (1.0f - t);
}
static float smoothStep(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static Font s_font;
static bool s_fontSet = false;

static void ft(const char *text, int x, int y, int size, ::Color col) {
    if (s_fontSet)
        DrawTextEx(s_font, text, {(float)x, (float)y}, (float)size, 1.0f, col);
    else
        DrawText(text, x, y, size, col); // Fixed: Uses raylib default font fallback instead of calling itself
}

static int ftMeasure(const char *text, int size) {
    if (s_fontSet)
        return (int)MeasureTextEx(s_font, text, (float)size, 1.0f).x;
    return MeasureText(text, size); // Fixed: Uses raylib default layout estimation fallback
}

static ::Color teamColor(const std::string &teamName, int fallbackId) {
    uint32_t hash = 2166136261u;
    std::string key = teamName.empty() ? std::to_string(fallbackId) : teamName;

    for (unsigned char c : key) {
        hash ^= c;
        hash *= 16777619u;
    }
    return {
        static_cast<uint8_t>(80 + (hash & 0x7f)),
        static_cast<uint8_t>(110 + ((hash >> 8) & 0x7f)),
        static_cast<uint8_t>(130 + ((hash >> 16) & 0x7f)),
        255,
    };
}

static ::Color resourceColor(int index) {
    static const Config::Color colors[] = {
        Config::RES_FOOD,     Config::RES_LINEMATE, Config::RES_DERAUMERE,
        Config::RES_SIBUR,    Config::RES_MENDIANE, Config::RES_PHIRAS,
        Config::RES_THYSTAME,
    };

    if (index < 0 || index >= 7)
        return toRl(Config::TEXT_PRIMARY);
    return toRl(colors[index]);
}

static float wrapCoord(float value, int size) {
    if (size <= 0)
        return value;
    while (value < 0.0f)
        value += static_cast<float>(size);
    while (value >= static_cast<float>(size))
        value -= static_cast<float>(size);
    return value;
}

static Vector2 playerWorldPosition(const Player &player, const World &world, float now) {
    if (!player.hasMotion || player.moveDuration <= 0.0f)
        return {static_cast<float>(player.x), static_cast<float>(player.y)};

    float t = smoothStep((now - player.moveStartedAt) / player.moveDuration);
    float x = player.fromX + (static_cast<float>(player.x) - player.fromX) * t;
    float y = player.fromY + (static_cast<float>(player.y) - player.fromY) * t;
    return {
        wrapCoord(x, world.getWidth()),
        wrapCoord(y, world.getHeight()),
    };
}

static const char *orientationName(int orientation) {
    switch (orientation) {
    case 1: return "N";
    case 2: return "E";
    case 3: return "S";
    case 4: return "W";
    default: return "?";
    }
}

static void drawOrientationTriangle(float cx, float cy, float radius, int orientation,
                                    ::Color color) {
    Vector2 tip{cx, cy - radius};
    Vector2 left{cx - radius * 0.55f, cy + radius * 0.45f};
    Vector2 right{cx + radius * 0.55f, cy + radius * 0.45f};

    if (orientation == 2) {
        tip = {cx + radius, cy};
        left = {cx - radius * 0.45f, cy - radius * 0.55f};
        right = {cx - radius * 0.45f, cy + radius * 0.55f};
    } else if (orientation == 3) {
        tip = {cx, cy + radius};
        left = {cx + radius * 0.55f, cy - radius * 0.45f};
        right = {cx - radius * 0.55f, cy - radius * 0.45f};
    } else if (orientation == 4) {
        tip = {cx - radius, cy};
        left = {cx + radius * 0.45f, cy + radius * 0.55f};
        right = {cx + radius * 0.45f, cy - radius * 0.55f};
    }
    DrawTriangle(tip, left, right, color);
}

static void drawChip(int x, int y, int w, int h, const char *label, const char *value,
                     ::Color labelCol, ::Color valueCol, ::Color borderCol, ::Color bgCol) {
    DrawRectangle(x, y, w, h, bgCol);
    DrawRectangleLinesEx({(float)x, (float)y, (float)w, (float)h}, 1.0f, borderCol);
    ft(label, x + 8, y + 5, 8, labelCol);
    ft(value, x + 8, y + 16, 11, valueCol);
}

void Graphical::initLayout() {
    int sw = m_window->getWidth();
    int sh = m_window->getHeight();
    m_mapX = Config::PANEL_W;
    m_mapY = Config::HUD_H;
    m_mapW = sw - Config::PANEL_W * 2;
    m_mapH = sh - Config::HUD_H - Config::STRIP_H;
}

void Graphical::drawBackground(int x, int y, int w, int h) {
    if (m_bgLoaded) {
        Rectangle src = {0, 0, static_cast<float>(m_bgTexture.width),
                         static_cast<float>(m_bgTexture.height)};
        Rectangle dst = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(w),
                         static_cast<float>(h)};
        DrawTexturePro(m_bgTexture, src, dst, {0, 0}, 0.0f, WHITE);
    } else {
        DrawRectangle(x, y, w, h, toRl(Config::BG_COLOR));
    }
}

void Graphical::render() {
    if (m_fontLoaded) {
        s_font = m_font;
        s_fontSet = true;
    }
    auto bg = Config::BG_COLOR;
    m_window->clear(bg.r, bg.g, bg.b, bg.a);
    initLayout();

    if (m_mapReceived && m_world) {
        drawMapViewport();
        updateMapStats();
    } else {
        drawWaitingScreen();
    }

    drawLeftPanel();
    drawRightPanel();
    drawHUDBar();
    drawControlsStrip();
    m_window->display();
}

void Graphical::drawHUDBar() {
    int sw = m_window->getWidth();
    int h = Config::HUD_H;

    DrawRectangle(0, 0, sw, h, toRl(Config::HUD_BG));
    DrawLine(0, h, sw, h, toRl(Config::HUD_BORDER));

    ft("ZAPPY", 14, 10, 20, toRl(Config::ACCENT_CYAN));
    ft("MISSION CONTROL", 78, 18, 9, toRl(Config::TEXT_GHOST));
    DrawRectangle(170, 10, 1, 28, toRl(Config::HUD_BORDER));

    float pulse = 0.5f + 0.5f * std::sin(m_time * 3.0f);
    ::Color dotCol = m_mapReceived
                         ? withAlpha(Config::ACCENT_GREEN, static_cast<uint8_t>(160 + 95 * pulse))
                         : withAlpha(Config::ACCENT_ORANGE, static_cast<uint8_t>(140 + 80 * pulse));
    DrawCircle(188, h / 2, 5.0f,
               withAlpha(m_mapReceived ? Config::ACCENT_GREEN : Config::ACCENT_ORANGE, 40));
    DrawCircle(188, h / 2, 4.0f, dotCol);

    const char *statusVal = m_mapReceived ? "CONNECTED" : "WAITING";
    ::Color statusCol = m_mapReceived ? toRl(Config::ACCENT_GREEN) : toRl(Config::ACCENT_ORANGE);
    drawChip(198, 8, 88, 32, "STATUS", statusVal, toRl(Config::TEXT_GHOST), statusCol,
             toRl(Config::HUD_BORDER), {0, 0, 0, 80});

    std::string mapVal =
        m_mapReceived ? (std::to_string(m_gridWidth) + "x" + std::to_string(m_gridHeight)) : "—";
    drawChip(294, 8, 72, 32, "MAP", mapVal.c_str(), toRl(Config::TEXT_GHOST),
             toRl(Config::TEXT_PRIMARY), toRl(Config::HUD_BORDER), {0, 0, 0, 80});

    std::string zoomVal = std::to_string(static_cast<int>(m_camera.zoom * 100)) + "%";
    drawChip(374, 8, 68, 32, "ZOOM", zoomVal.c_str(), toRl(Config::TEXT_GHOST),
             toRl(Config::TEXT_PRIMARY), toRl(Config::HUD_BORDER), {0, 0, 0, 80});

    int cx = 452;
    if (m_hoveredTileX >= 0) {
        std::string hStr =
            "HOVER [" + std::to_string(m_hoveredTileX) + "," + std::to_string(m_hoveredTileY) + "]";
        drawChip(cx, 8, static_cast<int>(hStr.size()) * 7 + 18, 32, "CURSOR", hStr.c_str(),
                 toRl(Config::TEXT_GHOST), toRl(Config::ACCENT_CYAN), toRl(Config::HUD_BORDER),
                 {0, 0, 0, 80});
        cx += static_cast<int>(hStr.size()) * 7 + 26;
    }
    if (m_hasSelection) {
        std::string sStr =
            "SEL [" + std::to_string(m_selectedTileX) + "," + std::to_string(m_selectedTileY) + "]";
        drawChip(cx, 8, static_cast<int>(sStr.size()) * 7 + 18, 32, "SELECTED", sStr.c_str(),
                 toRl(Config::TEXT_GHOST), toRl(Config::ACCENT_GREEN), {0, 200, 160, 60},
                 {0, 0, 0, 80});
    }

    const char *hint = "[←→↑↓] NAVIGATE   [R] RESET   [SCROLL] ZOOM   [RMB] PAN";
    int hw = ftMeasure(hint, 10);
    ft(hint, sw - hw - 80, 12, 10, toRl(Config::TEXT_GHOST));

    std::string fps = std::to_string(m_window->getFPS()) + " FPS";
    int fw = ftMeasure(fps.c_str(), 11);
    ft(fps.c_str(), sw - fw - 12, 18, 11, toRl(Config::TEXT_DIM));
}

void Graphical::drawLeftPanel() {
    int sh = m_window->getHeight();
    int px = 0;
    int py = Config::HUD_H;
    int pw = Config::PANEL_W;
    int ph = sh - Config::HUD_H - Config::STRIP_H;

    DrawRectangle(px, py, pw, ph, toRl(Config::BG_PANEL));
    DrawRectangle(pw - 1, py, 1, ph, toRl(Config::HUD_BORDER));

    int sy = py + 14;
    drawSectionBar(px + 14, sy, 12, toRl(Config::ACCENT_CYAN));
    ft("RESOURCES", px + 22, sy, 9, toRl(Config::TEXT_GHOST));
    sy += 18;

    static const char *resNames[] = {"FOOD",     "LINEMATE", "DERAUMERE", "SIBUR",
                                     "MENDIANE", "PHIRAS",   "THYSTAME"};
    static const Config::Color resCols[] = {
        Config::RES_FOOD,     Config::RES_LINEMATE, Config::RES_DERAUMERE, Config::RES_SIBUR,
        Config::RES_MENDIANE, Config::RES_PHIRAS,   Config::RES_THYSTAME};

    int colW = (pw - 20) / 2;
    for (int i = 0; i < 7; i++) {
        int col = i % 2;
        int row = i / 2;
        int ex = px + 14 + col * colW;
        int ey = sy + row * 22;

        DrawCircle(ex + 5, ey + 6, 7.0f, withAlpha(resCols[i], 22));
        DrawCircle(ex + 5, ey + 6, 4.0f, toRl(resCols[i]));
        ft(resNames[i], ex + 14, ey + 1, 10, toRl(Config::TEXT_DIM));
    }
    sy += 4 * 22 + 8;

    DrawLine(px, sy, px + pw - 1, sy, toRl(Config::HUD_BORDER));
    sy += 1;

    int inspY = sy + 12;
    drawSectionBar(px + 14, inspY, 12, toRl(Config::ACCENT_RED));
    ft("TILE INSPECTOR", px + 22, inspY, 9, toRl(Config::TEXT_GHOST));

    std::string coordStr = "—";
    int dispX = -1, dispY = -1;
    if (m_hoveredTileX >= 0) {
        dispX = m_hoveredTileX;
        dispY = m_hoveredTileY;
    } else if (m_hasSelection) {
        dispX = m_selectedTileX;
        dispY = m_selectedTileY;
    }
    if (dispX >= 0)
        coordStr = "[" + std::to_string(dispX) + "," + std::to_string(dispY) + "]";

    int coordW = ftMeasure(coordStr.c_str(), 11);
    ft(coordStr.c_str(), px + pw - coordW - 16, inspY, 11,
       dispX >= 0 ? toRl(Config::ACCENT_RED) : toRl(Config::TEXT_GHOST));

    inspY += 18;
    DrawLine(px, inspY, px + pw - 1, inspY, toRl(Config::HUD_BORDER));
    inspY += 10;

    if (dispX < 0) {
        const char *msg = "Hover or select a tile";
        const char *msg2 = "to inspect its contents";
        int mw1 = ftMeasure(msg, 10);
        int mw2 = ftMeasure(msg2, 10);
        int midY = inspY + (ph - (inspY - py)) / 2 - 20;
        ft(msg, px + (pw - mw1) / 2, midY, 10, toRl(Config::TEXT_GHOST));
        ft(msg2, px + (pw - mw2) / 2, midY + 16, 10, toRl(Config::TEXT_GHOST));
        return;
    }

    if (!m_world)
        return;

    const Tile &tile = m_world->getTile(static_cast<uint16_t>(dispX), static_cast<uint16_t>(dispY));
    uint16_t maxAmt = 1;
    for (uint8_t r = 0; r < 7; r++) {
        uint16_t a = tile.resources.get(static_cast<ResourceType>(r));
        if (a > maxAmt)
            maxAmt = a;
    }

    int rowH = 24;
    for (int i = 0; i < 7; i++) {
        uint16_t amt = tile.resources.get(static_cast<ResourceType>(i));
        ::Color rc = toRl(resCols[i]);
        bool zero = (amt == 0);
        int ry = inspY + i * rowH;

        if (zero)
            DrawCircleLines(px + 19, ry + 8, 4.0f, withAlpha(resCols[i], 50));
        else {
            DrawCircle(px + 19, ry + 8, 6.0f, withAlpha(resCols[i], 25));
            DrawCircle(px + 19, ry + 8, 4.0f, rc);
        }

        ft(resNames[i], px + 30, ry + 3, 10,
           zero ? toRl(Config::TEXT_GHOST) : toRl(Config::TEXT_DIM));

        int barX = px + 105, barW = 80, barY = ry + 9, barH = 3;
        DrawRectangle(barX, barY, barW, barH, {255, 255, 255, 10});
        if (!zero)
            DrawRectangle(barX, barY, static_cast<int>(barW * amt / (float)maxAmt), barH,
                          withAlpha(resCols[i], 150));

        std::string amtStr = std::to_string(amt);
        int aw = ftMeasure(amtStr.c_str(), 12);
        ft(amtStr.c_str(), px + pw - aw - 16, ry + 2, 12,
           zero ? toRl(Config::TEXT_GHOST) : withAlpha(resCols[i], 220));
        DrawLine(px + 14, ry + rowH - 1, px + pw - 14, ry + rowH - 1, {255, 255, 255, 8});
    }

    int entityY = inspY + 7 * rowH + 12;
    if (entityY + 20 >= py + ph)
        return;

    DrawLine(px, entityY - 6, px + pw - 1, entityY - 6, toRl(Config::HUD_BORDER));
    drawSectionBar(px + 14, entityY, 12, toRl(Config::ACCENT_GREEN));
    ft("PLAYERS", px + 22, entityY, 9, toRl(Config::TEXT_GHOST));

    auto players = m_world->getPlayersAt(dispX, dispY);
    auto eggs = m_world->getEggsAt(dispX, dispY);
    int lineY = entityY + 18;

    if (players.empty()) {
        ft("No players on tile", px + 16, lineY, 10, toRl(Config::TEXT_GHOST));
        lineY += 16;
    } else {
        int maxPlayers = std::min<int>(static_cast<int>(players.size()), 4);
        for (int i = 0; i < maxPlayers; ++i) {
            const Player *player = players[i];
            ::Color color = teamColor(player->teamName, player->id);
            std::string name = "#" + std::to_string(player->id) + " L" +
                               std::to_string(player->level) + " " +
                               orientationName(player->orientation);
            std::string team = player->teamName.empty() ? "unknown" : player->teamName;

            DrawCircle(px + 20, lineY + 7, 5.0f, color);
            ft(name.c_str(), px + 32, lineY + 1, 10, toRl(Config::TEXT_PRIMARY));
            int tw = ftMeasure(team.c_str(), 9);
            ft(team.c_str(), px + pw - tw - 16, lineY + 2, 9, toRl(Config::TEXT_DIM));
            lineY += 15;

            if (player->hasInventory && lineY + 12 < py + ph) {
                std::string inv = "F " +
                    std::to_string(player->inventory.get(ResourceType::Food)) + "  L " +
                    std::to_string(player->inventory.get(ResourceType::Linemate)) + "  S " +
                    std::to_string(player->inventory.get(ResourceType::Sibur));
                ft(inv.c_str(), px + 32, lineY, 8, toRl(Config::TEXT_GHOST));
                lineY += 12;
            }
            if (!player->lastEvent.empty() && lineY + 12 < py + ph) {
                ft(player->lastEvent.c_str(), px + 32, lineY, 8, toRl(Config::ACCENT_ORANGE));
                lineY += 12;
            }
            if (!player->lastMessage.empty() && lineY + 12 < py + ph) {
                std::string msg = player->lastMessage.substr(0, 28);
                ft(msg.c_str(), px + 32, lineY, 8, toRl(Config::TEXT_DIM));
                lineY += 12;
            }
        }
        if (static_cast<int>(players.size()) > maxPlayers && lineY + 12 < py + ph) {
            std::string more = "+" + std::to_string(players.size() - maxPlayers) + " more";
            ft(more.c_str(), px + 32, lineY, 8, toRl(Config::TEXT_GHOST));
            lineY += 12;
        }
    }

    if (!eggs.empty() && lineY + 18 < py + ph) {
        std::string eggLine = "EGGS " + std::to_string(eggs.size());
        ft(eggLine.c_str(), px + 16, lineY + 4, 9, toRl(Config::ACCENT_AMBER));
        int maxEggs = std::min<int>(static_cast<int>(eggs.size()), 3);
        for (int i = 0; i < maxEggs; ++i) {
            std::string eggId = "#" + std::to_string(eggs[i]->id);
            ft(eggId.c_str(), px + 72 + i * 42, lineY + 4, 9, toRl(Config::TEXT_DIM));
        }
    }
}

void Graphical::drawRightPanel() {
    int sw = m_window->getWidth();
    int sh = m_window->getHeight();
    int px = sw - Config::PANEL_W;
    int py = Config::HUD_H;
    int pw = Config::PANEL_W;
    int ph = sh - Config::HUD_H - Config::STRIP_H;

    DrawRectangle(px, py, pw, ph, toRl(Config::BG_PANEL));
    DrawRectangle(px, py, 1, ph, toRl(Config::HUD_BORDER));

    int logSectionH = static_cast<int>(ph * 0.62f);
    int sy = py + 14;

    drawSectionBar(px + 14, sy, 12, toRl(Config::ACCENT_ORANGE));
    ft("EVENT LOG", px + 22, sy, 9, toRl(Config::TEXT_GHOST));
    sy += 18;
    DrawLine(px, sy, px + pw, sy, toRl(Config::HUD_BORDER));
    sy += 1;

    int maxVisible = (logSectionH - 32) / 18;
    int startIdx = std::max(0, (int)m_log.size() - maxVisible);
    int drawn = 0;

    for (int i = (int)m_log.size() - 1; i >= startIdx; i--) {
        const LogEntry &e = m_log[i];
        float alpha = std::clamp(e.alpha, 0.0f, 1.0f);
        auto a = static_cast<uint8_t>(alpha * 200);

        ::Color msgCol;
        const std::string &txt = e.text;
        if (txt.find("LOADED") != std::string::npos || txt.find("CONNECTED") != std::string::npos)
            msgCol = withAlpha(Config::ACCENT_GREEN, a);
        else if (txt.find("TICK") != std::string::npos || txt.find("FREQ") != std::string::npos)
            msgCol = withAlpha(Config::ACCENT_CYAN, a);
        else if (txt.find("ERROR") != std::string::npos ||
                 txt.find("GAME OVER") != std::string::npos)
            msgCol = withAlpha(Config::ACCENT_RED, a);
        else
            msgCol = withAlpha(Config::TEXT_DIM, a);

        if (drawn == 0 && !m_log.empty())
            DrawRectangle(px + 1, sy + drawn * 18, pw - 2, 17, {255, 80, 40, 8});

        std::string idx = std::to_string(i + 1);
        ft(idx.c_str(), px + 14, sy + drawn * 18 + 4, 8, withAlpha(Config::TEXT_GHOST, a / 2));
        ft(e.text.c_str(), px + 30, sy + drawn * 18 + 4, 10, msgCol);
        drawn++;
    }

    if (m_log.empty()) {
        const char *msg = "No events yet";
        int mw = ftMeasure(msg, 10);
        ft(msg, px + (pw - mw) / 2, sy + 20, 10, toRl(Config::TEXT_GHOST));
    }

    int sepY = py + logSectionH;
    DrawLine(px, sepY, px + pw, sepY, toRl(Config::HUD_BORDER));

    int statY = sepY + 14;
    drawSectionBar(px + 14, statY, 12, toRl(Config::ACCENT_CYAN));
    ft("MAP STATISTICS", px + 22, statY, 9, toRl(Config::TEXT_GHOST));
    statY += 22;

    int cardW = (pw - 36) / 2, cardH = 46, cardGap = 8;
    struct StatCard {
        const char *label;
        int value;
        Config::Color valCol;
    } cards[4] = {
        {"TOTAL TILES", m_gridWidth * m_gridHeight, Config::ACCENT_CYAN},
        {"FILLED", m_filledTiles, Config::ACCENT_GREEN},
        {"FOOD", m_totalFood, Config::ACCENT_AMBER},
        {"MINERALS", m_totalMinerals, Config::TEXT_PRIMARY},
    };

    for (int i = 0; i < 4; i++) {
        int col = i % 2, row = i / 2;
        int ccx = px + 14 + col * (cardW + cardGap);
        int ccy = statY + row * (cardH + cardGap);

        DrawRectangle(ccx, ccy, cardW, cardH, {10, 5, 20, 80});
        DrawRectangleLinesEx({(float)ccx, (float)ccy, (float)cardW, (float)cardH}, 1.0f,
                             toRl(Config::HUD_BORDER));
        ft(cards[i].label, ccx + 8, ccy + 8, 8, toRl(Config::TEXT_GHOST));

        std::string val = m_mapReceived ? std::to_string(cards[i].value) : "—";
        ft(val.c_str(), ccx + 9, ccy + 22, 18, withAlpha(cards[i].valCol, 40));
        ft(val.c_str(), ccx + 8, ccy + 22, 18, toRl(cards[i].valCol));
    }

    int teamY = statY + 2 * (cardH + cardGap) + 10;
    if (m_mapReceived && m_world && teamY + 22 < py + ph) {
        drawSectionBar(px + 14, teamY, 12, toRl(Config::ACCENT_GREEN));
        ft("TEAMS", px + 22, teamY, 9, toRl(Config::TEXT_GHOST));
        teamY += 18;

        if (m_world->getTeams().empty()) {
            ft("No team data", px + 16, teamY, 10, toRl(Config::TEXT_GHOST));
        } else {
            int rows = 0;
            for (const std::string &team : m_world->getTeams()) {
                int count = 0;
                for (const auto &[id, player] : m_world->getPlayers()) {
                    (void)id;
                    if (player.teamName == team)
                        ++count;
                }
                ::Color color = teamColor(team, rows);
                DrawCircle(px + 20, teamY + rows * 16 + 6, 5.0f, color);
                ft(team.c_str(), px + 32, teamY + rows * 16 + 1, 10, toRl(Config::TEXT_DIM));
                std::string countText = std::to_string(count);
                int cw = ftMeasure(countText.c_str(), 10);
                ft(countText.c_str(), px + pw - cw - 18, teamY + rows * 16 + 1, 10,
                   toRl(Config::TEXT_PRIMARY));
                if (++rows >= 6 || teamY + rows * 16 + 14 >= py + ph)
                    break;
            }
        }
    }
}

void Graphical::drawControlsStrip() {
    int sw = m_window->getWidth();
    int sh = m_window->getHeight();
    int sy = sh - Config::STRIP_H;

    DrawRectangle(0, sy, sw, Config::STRIP_H, toRl(Config::BG_PANEL));
    DrawLine(0, sy, sw, sy, toRl(Config::HUD_BORDER));

    const char *hint = "[↑↓←→ / WASD]  Navigate    [Scroll]  Zoom    "
                       "[RMB]  Pan    [R]  Reset camera    [Click]  Select tile";
    int hw = ftMeasure(hint, 9);
    ft(hint, (sw - hw) / 2, sy + 9, 9, toRl(Config::TEXT_GHOST));
}

void Graphical::drawWaitingScreen() {
    int mx = m_mapX, my = m_mapY, mw = m_mapW, mh = m_mapH;

    drawBackground(mx, my, mw, mh);

    DrawRectangle(mx, my, mw, mh, {0, 0, 0, 120});

    float pulse = 0.5f + 0.5f * std::sin(m_time * 2.0f);
    uint8_t glowA = static_cast<uint8_t>(80 + 80 * pulse);

    const char *title = "TRANTOR SURFACE SCANNER";
    int tfs = 26;
    int tw = ftMeasure(title, tfs);
    ft(title, mx + (mw - tw) / 2, my + mh / 2 - 56, tfs, toRl(Config::ACCENT_CYAN));

    const char *sub = "AWAITING SERVER SIGNAL...";
    int sfs = 12;
    int stw = ftMeasure(sub, sfs);
    ft(sub, mx + (mw - stw) / 2, my + mh / 2 - 10, sfs, withAlpha(Config::ACCENT_RED, glowA));

    int barW = 240, barH = 2;
    int barX = mx + (mw - barW) / 2;
    int barY = my + mh / 2 + 18;
    float fill = std::fmod(m_time * 0.4f, 1.0f);
    DrawRectangle(barX, barY, barW, barH, toRl(Config::GRID_LINE));
    DrawRectangle(barX, barY, static_cast<int>(barW * fill), barH,
                  withAlpha(Config::ACCENT_CYAN, 200));

    const char *hint = "Connect:  ./zappy_gui -p <port> -h <host>";
    int hfs = 10;
    int htw = ftMeasure(hint, hfs);
    ft(hint, mx + (mw - htw) / 2, my + mh / 2 + 44, hfs, toRl(Config::TEXT_GHOST));
}

void Graphical::drawMapViewport() {
    BeginScissorMode(m_mapX, m_mapY, m_mapW, m_mapH);

    drawBackground(m_mapX, m_mapY, m_mapW, m_mapH);

    DrawRectangle(m_mapX, m_mapY, m_mapW, m_mapH, {0, 0, 0, 100});

    drawGrid();
    drawToroidalEdges();
    drawResources();
    drawInteractionAnimations();

    EndScissorMode();
}

void Graphical::drawGrid() {
    if (!m_world)
        return;

    float ts = static_cast<float>(m_tileSize) * m_camera.zoom;
    int sw = m_window->getWidth();
    int sh = m_window->getHeight();

    for (int y = 0; y < m_gridHeight; y++) {
        for (int x = 0; x < m_gridWidth; x++) {
            float px = m_camera.offsetX + x * ts;
            float py = m_camera.offsetY + y * ts;

            if (px + ts < 0 || px > sw || py + ts < 0 || py > sh)
                continue;

            ::Color fill =
                ((x + y) % 2 == 0) ? toRl(Config::TILE_FILL) : toRl(Config::TILE_FILL_ALT);
            drawRect(px, py, ts, ts, fill);

            bool hovered = (x == m_hoveredTileX && y == m_hoveredTileY);
            bool selected = (m_hasSelection && x == m_selectedTileX && y == m_selectedTileY);
            bool incanting = false;

            for (const Incantation &incantation : m_world->getIncantations()) {
                if (incantation.active && incantation.x == x && incantation.y == y) {
                    incanting = true;
                    break;
                }
            }

            ::Color border = incanting ? toRl(Config::ACCENT_GREEN)
                             : selected  ? toRl(Config::ACCENT_RED)
                             : hovered ? toRl(Config::GRID_HOVER)
                                       : toRl(Config::GRID_LINE);
            float borderThick = (incanting || selected || hovered) ? 2.5f : 1.5f;
            DrawRectangleLinesEx({px, py, ts, ts}, borderThick, border);

            if (incanting) {
                drawRect(px, py, ts, ts, withAlpha(Config::ACCENT_GREEN, 18));
            }
            if (selected) {
                drawRect(px, py, ts, ts, withAlpha(Config::ACCENT_RED, 16));
                float pulse = 0.5f + 0.5f * std::sin(m_time * 3.0f);
                uint8_t cA = static_cast<uint8_t>(140 + 115 * pulse);
                ::Color cc = withAlpha(Config::ACCENT_RED, cA);
                float cs = std::min(ts * 0.28f, 10.0f);
                DrawLineEx({px, py}, {px + cs, py}, 2.0f, cc);
                DrawLineEx({px, py}, {px, py + cs}, 2.0f, cc);
                DrawLineEx({px + ts, py}, {px + ts - cs, py}, 2.0f, cc);
                DrawLineEx({px + ts, py}, {px + ts, py + cs}, 2.0f, cc);
                DrawLineEx({px, py + ts}, {px + cs, py + ts}, 2.0f, cc);
                DrawLineEx({px, py + ts}, {px, py + ts - cs}, 2.0f, cc);
                DrawLineEx({px + ts, py + ts}, {px + ts - cs, py + ts}, 2.0f, cc);
                DrawLineEx({px + ts, py + ts}, {px + ts, py + ts - cs}, 2.0f, cc);
            } else if (hovered) {
                drawRect(px, py, ts, ts, withAlpha(Config::ACCENT_RED, 14));
            }
        }
    }
}

void Graphical::drawResources() {
    if (!m_world)
        return;

    float ts = static_cast<float>(m_tileSize) * m_camera.zoom;
    int sw = m_window->getWidth();
    int sh = m_window->getHeight();

    // ==========================================
    // PASS 1: DRAW BACKGROUND RESOURCES ON TILES
    // ==========================================
    for (uint16_t y = 0; y < m_gridHeight; y++) {
        for (uint16_t x = 0; x < m_gridWidth; x++) {
            float px = m_camera.offsetX + x * ts;
            float py = m_camera.offsetY + y * ts;

            // Fast culling
            if (px + ts < 0 || px > sw || py + ts < 0 || py > sh)
                continue;

            const Tile &tile = m_world->getTile(x, y);
            float phase = static_cast<float>(x * 3 + y * 7) * 0.42f;

            // This safely finishes completely before we handle living entities
            m_resourceRenderer->drawResourcesOnTile(static_cast<int>(px), static_cast<int>(py),
                                                    static_cast<int>(ts), tile, m_time, phase);
        }
    }

    // ==========================================
    // PASS 2: DRAW PLAYERS OVER TILES (UNLINKED TO RESOURCES)
    // ==========================================;

    for (const auto &[id, egg] : m_world->getEggs()) {
        (void)id;
        float px = m_camera.offsetX + egg.x * ts;
        float py = m_camera.offsetY + egg.y * ts;

        if (px + ts < 0 || px > sw || py + ts < 0 || py > sh)
            continue;

        float radius = std::clamp(ts * 0.12f, 3.0f, 9.0f);
        float cx = px + ts * 0.78f;
        float cy = py + ts * 0.78f;
        ::Color color = teamColor(egg.teamName, egg.parentId);
        DrawCircle(static_cast<int>(cx), static_cast<int>(cy), radius, {color.r, color.g, color.b, 45});
        DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), radius, color);
    }

    std::vector<const Player *> players;
    for (const auto &[id, player] : m_world->getPlayers()) {
        (void)id;
        players.push_back(&player);
    }
    std::sort(players.begin(), players.end(), [](const Player *a, const Player *b) {
        return a->id < b->id;
    });

    for (const Player *player : players) {
        if (!player->alive)
            continue;

        Vector2 worldPos = playerWorldPosition(*player, *m_world, m_time);
        float px = m_camera.offsetX + worldPos.x * ts;
        float py = m_camera.offsetY + worldPos.y * ts;

        // Screen viewport validation check
        if (px + ts < 0 || px > sw || py + ts < 0 || py > sh)
            continue;

        float radius = std::clamp(ts * 0.24f, 5.0f, 18.0f);
        float phase = static_cast<float>((player->id % 9 + 9) % 9) * 0.7f;
        float jitterRadius = (ts > 22.0f) ? ts * 0.10f : 0.0f;
        float cx = px + ts * 0.5f + std::cos(phase) * jitterRadius;
        float cy = py + ts * 0.5f + std::sin(phase) * jitterRadius;
        ::Color color = teamColor(player->teamName, player->id);

        DrawCircle(static_cast<int>(cx), static_cast<int>(cy), radius * 1.45f,
                   {color.r, color.g, color.b, 28});
        DrawCircle(static_cast<int>(cx), static_cast<int>(cy), radius, color);
        DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), radius,
                        player->inIncantation ? toRl(Config::ACCENT_GREEN) : ::WHITE);
        drawOrientationTriangle(cx, cy, radius * 0.58f, player->orientation,
                                {255, 255, 255, 210});

        if (ts >= 30.0f) {
            std::string level = std::to_string(player->level);
            int fs = std::max(8, static_cast<int>(radius * 0.75f));
            int tw = MeasureText(level.c_str(), fs);
            DrawText(level.c_str(), static_cast<int>(cx) - tw / 2,
                     static_cast<int>(cy + radius * 0.28f), fs, {0, 0, 0, 210});
        }
    }
}

void Graphical::drawInteractionAnimations() {
    if (!m_mapReceived || !m_world)
        return;

    float ts = static_cast<float>(m_tileSize) * m_camera.zoom;
    int sw = m_window->getWidth();
    int sh = m_window->getHeight();

    for (const InteractionAnimation &animation : m_interactionAnimations) {
        float duration = std::max(animation.duration, 0.001f);
        float t = std::clamp((m_time - animation.startedAt) / duration, 0.0f, 1.0f);
        float e = easeOut(t);
        float cx = m_camera.offsetX + animation.x * ts + ts * 0.5f;
        float cy = m_camera.offsetY + animation.y * ts + ts * 0.5f;

        if (cx + ts < 0 || cx - ts > sw || cy + ts < 0 || cy - ts > sh)
            continue;

        uint8_t alpha = static_cast<uint8_t>(std::clamp(1.0f - t, 0.0f, 1.0f) * 220.0f);
        ::Color team = teamColor(animation.teamName, animation.playerId);
        ::Color color = team;
        if (animation.resourceIndex >= 0)
            color = resourceColor(animation.resourceIndex);
        if (!animation.success)
            color = toRl(Config::ACCENT_RED);

        switch (animation.type) {
        case InteractionAnimationType::Spawn:
            DrawRing({cx, cy}, ts * (0.16f + e * 0.20f), ts * (0.19f + e * 0.22f),
                     0, 360, 48, {color.r, color.g, color.b, alpha});
            DrawCircle(static_cast<int>(cx), static_cast<int>(cy), ts * 0.08f * (1.0f - t),
                       {255, 255, 255, static_cast<uint8_t>(alpha * 0.7f)});
            break;
        case InteractionAnimationType::Move:
            DrawCircle(static_cast<int>(cx), static_cast<int>(cy), ts * (0.08f + e * 0.10f),
                       {color.r, color.g, color.b, static_cast<uint8_t>(alpha * 0.35f)});
            break;
        case InteractionAnimationType::Broadcast:
            for (int i = 0; i < 3; ++i) {
                float local = std::clamp(t - i * 0.18f, 0.0f, 1.0f);
                float radius = ts * (0.22f + easeOut(local) * 0.75f);
                uint8_t ringAlpha = static_cast<uint8_t>((1.0f - local) * 150.0f);
                DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), radius,
                                {color.r, color.g, color.b, ringAlpha});
            }
            break;
        case InteractionAnimationType::Pickup:
        case InteractionAnimationType::Drop: {
            float dir = animation.type == InteractionAnimationType::Pickup ? -1.0f : 1.0f;
            float y = cy + dir * e * ts * 0.35f;
            DrawCircle(static_cast<int>(cx), static_cast<int>(y), ts * 0.08f,
                       {color.r, color.g, color.b, alpha});
            std::string label = animation.label.empty()
                ? (animation.type == InteractionAnimationType::Pickup ? "+" : "-")
                : animation.label;
            ft(label.c_str(), static_cast<int>(cx + ts * 0.10f), static_cast<int>(y - ts * 0.15f),
               std::max(8, static_cast<int>(ts * 0.22f)), {color.r, color.g, color.b, alpha});
            break;
        }
        case InteractionAnimationType::Eject:
            DrawRing({cx, cy}, ts * (0.20f + e * 0.45f), ts * (0.23f + e * 0.48f),
                     0, 360, 48, {255, 70, 40, alpha});
            DrawLineEx({cx - ts * 0.25f, cy}, {cx + ts * 0.25f, cy}, 2.0f,
                       {255, 255, 255, alpha});
            DrawLineEx({cx, cy - ts * 0.25f}, {cx, cy + ts * 0.25f}, 2.0f,
                       {255, 255, 255, alpha});
            break;
        case InteractionAnimationType::Fork:
        case InteractionAnimationType::EggLaid:
            DrawRing({cx, cy + ts * 0.16f}, ts * (0.10f + e * 0.12f), ts * (0.13f + e * 0.15f),
                     0, 360, 36, {255, 184, 0, alpha});
            DrawCircle(static_cast<int>(cx), static_cast<int>(cy + ts * 0.16f), ts * 0.07f,
                       {255, 210, 80, static_cast<uint8_t>(alpha * 0.7f)});
            break;
        case InteractionAnimationType::EggHatch:
            DrawRing({cx, cy}, ts * (0.08f + e * 0.35f), ts * (0.11f + e * 0.38f),
                     0, 360, 40, {0, 255, 160, alpha});
            for (int i = 0; i < 6; ++i) {
                float a = m_time * 0.8f + i * 1.047f;
                DrawLineEx({cx, cy},
                           {cx + std::cos(a) * ts * 0.34f * e,
                            cy + std::sin(a) * ts * 0.34f * e},
                           2.0f, {0, 255, 160, alpha});
            }
            break;
        case InteractionAnimationType::EggDeath:
        case InteractionAnimationType::Death:
            DrawRing({cx, cy}, ts * (0.12f + e * 0.32f), ts * (0.15f + e * 0.35f),
                     0, 360, 40, {255, 60, 40, alpha});
            DrawLineEx({cx - ts * 0.16f, cy - ts * 0.16f}, {cx + ts * 0.16f, cy + ts * 0.16f},
                       2.0f, {255, 255, 255, alpha});
            DrawLineEx({cx + ts * 0.16f, cy - ts * 0.16f}, {cx - ts * 0.16f, cy + ts * 0.16f},
                       2.0f, {255, 255, 255, alpha});
            break;
        case InteractionAnimationType::IncantationStart:
        case InteractionAnimationType::IncantationEnd: {
            ::Color ritual = animation.type == InteractionAnimationType::IncantationStart
                ? toRl(Config::ACCENT_GREEN)
                : (animation.success ? toRl(Config::ACCENT_AMBER) : toRl(Config::ACCENT_RED));
            DrawRing({cx, cy}, ts * (0.22f + e * 0.38f), ts * (0.25f + e * 0.41f),
                     0, 360, 64, {ritual.r, ritual.g, ritual.b, alpha});
            for (int i = 0; i < 4; ++i) {
                float a = m_time * 2.2f + i * 1.5708f;
                Vector2 p1{cx + std::cos(a) * ts * 0.28f, cy + std::sin(a) * ts * 0.28f};
                Vector2 p2{cx + std::cos(a + 0.35f) * ts * 0.42f,
                           cy + std::sin(a + 0.35f) * ts * 0.42f};
                DrawLineEx(p1, p2, 2.0f, {ritual.r, ritual.g, ritual.b, alpha});
            }
            break;
        }
        case InteractionAnimationType::LevelUp:
            DrawRing({cx, cy}, ts * (0.14f + e * 0.35f), ts * (0.17f + e * 0.38f),
                     0, 360, 48, {255, 210, 60, alpha});
            for (int i = 0; i < 8; ++i) {
                float a = i * 0.7854f;
                DrawLineEx({cx + std::cos(a) * ts * 0.16f, cy + std::sin(a) * ts * 0.16f},
                           {cx + std::cos(a) * ts * (0.26f + e * 0.18f),
                            cy + std::sin(a) * ts * (0.26f + e * 0.18f)},
                           2.0f, {255, 210, 60, alpha});
            }
            break;
        }

        if (!animation.label.empty() && animation.type != InteractionAnimationType::Pickup &&
            animation.type != InteractionAnimationType::Drop) {
            int fs = std::max(8, static_cast<int>(ts * 0.18f));
            int tw = ftMeasure(animation.label.c_str(), fs);
            ft(animation.label.c_str(), static_cast<int>(cx - tw / 2),
               static_cast<int>(cy - ts * (0.52f + t * 0.18f)), fs,
               {color.r, color.g, color.b, alpha});
        }
    }
}

void Graphical::drawToroidalEdges() {
    float ts = static_cast<float>(m_tileSize) * m_camera.zoom;
    float gx = m_camera.offsetX;
    float gy = m_camera.offsetY;
    float gw = ts * m_gridWidth;
    float gh = ts * m_gridHeight;

    float waveA = 0.5f + 0.5f * std::sin(m_time * 1.2f);
    uint8_t edgeA = static_cast<uint8_t>(55 + 45 * waveA);
    uint8_t arrowA = static_cast<uint8_t>(120 + 100 * waveA);

    int thick = std::max(2, static_cast<int>(ts * 0.08f));
    int glowThick = thick * 3;

    ::Color glowColR = {255, 40, 10, static_cast<uint8_t>(edgeA / 3)};
    ::Color glowColB = {30, 80, 255, static_cast<uint8_t>(edgeA / 3)};
    ::Color edgeColR = {255, 50, 20, edgeA};
    ::Color edgeColB = {40, 100, 255, edgeA};
    ::Color arrowColR = {255, 60, 20, arrowA};
    ::Color arrowColB = {40, 120, 255, arrowA};

    DrawRectangle((int)gx, (int)gy, (int)gw, glowThick, glowColR);
    DrawRectangle((int)gx, (int)(gy + gh - glowThick), (int)gw, glowThick, glowColB);
    DrawRectangle((int)gx, (int)gy, glowThick, (int)gh, glowColB);
    DrawRectangle((int)(gx + gw - glowThick), (int)gy, glowThick, (int)gh, glowColR);

    DrawRectangle((int)gx, (int)gy, (int)gw, thick, edgeColR);
    DrawRectangle((int)gx, (int)(gy + gh - thick), (int)gw, thick, edgeColB);
    DrawRectangle((int)gx, (int)gy, thick, (int)gh, edgeColB);
    DrawRectangle((int)(gx + gw - thick), (int)gy, thick, (int)gh, edgeColR);

    float arrowSz = std::clamp(ts * 0.22f, 5.0f, 14.0f);
    float inset = arrowSz * 2.0f;
    int hSteps = std::max(1, (int)(m_gridWidth / 5));
    int vSteps = std::max(1, (int)(m_gridHeight / 4));

    for (int i = 1; i < m_gridWidth; i += hSteps) {
        float cx = gx + i * ts + ts * 0.5f;
        DrawTriangle({cx, gy + inset}, {cx + arrowSz * .5f, gy + inset + arrowSz},
                     {cx - arrowSz * .5f, gy + inset + arrowSz}, arrowColR);
        DrawTriangle({cx, gy + gh - inset}, {cx - arrowSz * .5f, gy + gh - inset - arrowSz},
                     {cx + arrowSz * .5f, gy + gh - inset - arrowSz}, arrowColB);
    }
    for (int i = 1; i < m_gridHeight; i += vSteps) {
        float cy = gy + i * ts + ts * 0.5f;
        DrawTriangle({gx + inset, cy}, {gx + inset + arrowSz, cy + arrowSz * .5f},
                     {gx + inset + arrowSz, cy - arrowSz * .5f}, arrowColB);
        DrawTriangle({gx + gw - inset, cy}, {gx + gw - inset - arrowSz, cy - arrowSz * .5f},
                     {gx + gw - inset - arrowSz, cy + arrowSz * .5f}, arrowColR);
    }

    int lfs = std::max(8, (int)arrowSz);
    const char *w = "WRAPS";
    int lw = ftMeasure(w, lfs);
    auto lcR = ::Color{255, 60, 20, static_cast<uint8_t>(arrowA / 2)};
    auto lcB = ::Color{40, 120, 255, static_cast<uint8_t>(arrowA / 2)};

    ft(w, (int)(gx + gw / 2) - lw / 2, (int)gy - lfs - 4, lfs, lcR);
    ft(w, (int)(gx + gw / 2) - lw / 2, (int)(gy + gh) + 4, lfs, lcB);
    ft(w, (int)gx - lw - 4, (int)(gy + gh / 2) - lfs / 2, lfs, lcB);
    ft(w, (int)(gx + gw) + 4, (int)(gy + gh / 2) - lfs / 2, lfs, lcR);
}
