#include "../graphical/Graphical3d.hpp"
#include "world.hpp"
#include <algorithm>
#include <raylib.h>
#include <math.h>
#include <vector>

static float orientationToAngle(int orientation)
{
    if (orientation == 1) 
        return 180.0f;
    if (orientation == 2) 
        return 90.0f;
    if (orientation == 3) 
        return 0.0f;
    if (orientation == 4) 
        return 270.0f;
    return 0.0f;
}

void Graphical3D::renderOnePlayer(const Player &player)
{
    static const Vector3 dirs[5] = { {0,0,0}, {0,0,-0.25f}, {0.25f,0,0}, {0,0,0.25f}, {-0.25f,0,0} };
    Vector3 base = tileCenter(static_cast<uint16_t>(player.x),  static_cast<uint16_t>(player.y));
    Vector3 pos = {base.x, 0.3f, base.z};

    if (m_playerModelLoaded) {
        DrawModelEx(m_playerModel, pos, Vector3{0.0f, 1.0f, 0.0f}, orientationToAngle(player.orientation), 
        Vector3{0.3f, 0.3f, 0.3f}, WHITE);
    } else {
        DrawCylinder(pos, 0.08f, 0.08f, 0.35f, 8, SKYBLUE);
        DrawSphere({pos.x, pos.y + 0.22f, pos.z}, 0.10f, SKYBLUE);
    }
    int ori = (player.orientation >= 1 && player.orientation <= 4) ? player.orientation : 1;
    DrawSphere({pos.x + dirs[ori].x, pos.y + 0.15f, pos.z + dirs[ori].z}, 0.04f, YELLOW);
}

void Graphical3D::renderEggs()
{
    float pulse = 0.5f + 0.5f * sinf(m_time * 2.0f);

    for (const auto &[id, egg] : m_world->getEggs()) {
        (void)id;
        if (egg.x < 0 || egg.y < 0 || egg.x >= m_gridWidth || egg.y >= m_gridHeight)
            continue;
        Vector3 base = tileCenter(static_cast<uint16_t>(egg.x),
                                  static_cast<uint16_t>(egg.y));
        float size = 0.12f + 0.02f * pulse;
        DrawSphere({base.x, 0.4f, base.z}, size, GOLD);
        DrawSphereWires({base.x, 0.4f, base.z}, size, 6, 6, {255, 200, 0, 100});
    }
}

void Graphical3D::renderPlayerLabels()
{
    if (!m_world) return;

    for (const auto &[id, player] : m_world->getPlayers()) {
        if (!player.alive) continue;

        Vector3 pos3d = tileCenter(static_cast<uint16_t>(player.x),
                                   static_cast<uint16_t>(player.y));
        pos3d.y += 0.7f;

        Vector2 screenPos = GetWorldToScreen(pos3d, m_camera);

        std::string label = "Player #" + std::to_string(player.id) 
                  + " L" + std::to_string(player.level);
        int tw = MeasureText(label.c_str(), 10);
        DrawText(label.c_str(),
                 (int)screenPos.x - tw / 2,
                 (int)screenPos.y,
                 10, WHITE);
    }
}

void Graphical3D::renderPlayers()
{
    if (!m_world) 
        return;

    std::vector<const Player *> players;

    for (const auto &[id, player] : m_world->getPlayers()) {
        (void)id;
        players.push_back(&player);
    }
    std::sort(players.begin(), players.end(), [](const Player *a, const Player *b) {
        return a->id < b->id;
    });
    for (const Player *player : players) {
        if (!player->alive || player->x < 0 || player->y < 0 ||
            player->x >= m_gridWidth || player->y >= m_gridHeight)
            continue;
        renderOnePlayer(*player);
    }
    renderEggs();
}