#include "Graphical3d.hpp"
#include "world.hpp"
#include <string>

void Graphical3D::initializeWorld(uint16_t w, uint16_t h)
{
    m_gridWidth = w;
    m_gridHeight = h;
    m_world = std::make_unique<World>(w, h);
    m_mapReceived = true;

    resetCamera();
    pushLog("MAP " + std::to_string(w) + "x" + std::to_string(h) + " LOADED");
}

void Graphical3D::updateMapStats()
{
    m_totalFood = m_totalMinerals = m_filledTiles = 0;

    for (uint16_t y = 0; y < m_gridHeight; y++) {
        for (uint16_t x = 0; x < m_gridWidth; x++) {
            const Tile &t = m_world->getTile(x, y);
            bool hasAny = false;
            for (uint8_t r = 0; r < static_cast<uint8_t>(ResourceType::Count); r++) {
                uint16_t amt = t.resources.get(static_cast<ResourceType>(r));
                if (amt > 0) {
                    hasAny = true;
                    (r == 0 ? m_totalFood : m_totalMinerals) += amt;
                }
            }
            if (hasAny) m_filledTiles++;
        }
    }
}

void Graphical3D::pushLog(const std::string &text)
{
    m_log.push_back({text, 6.0f});
    if (m_log.size() > 20)
        m_log.erase(m_log.begin());
}