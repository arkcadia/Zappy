#include "graphical.hpp"
#include "window.hpp"
#include <raylib.h>

void Graphical::handleInput() {
    if (m_window->isKeyPressed(KEY_ESCAPE))
        m_running = false;
    updateCamera();
    handleArrowKeys();
}

void Graphical::handleArrowKeys() {
    if (!m_mapReceived || !m_hasSelection)
        return;

    int dx = 0;
    int dy = 0;

    if (m_window->isKeyPressed(KEY_RIGHT) || m_window->isKeyPressed(KEY_D))
        dx = +1;
    if (m_window->isKeyPressed(KEY_LEFT) || m_window->isKeyPressed(KEY_A))
        dx = -1;
    if (m_window->isKeyPressed(KEY_DOWN) || m_window->isKeyPressed(KEY_S))
        dy = +1;
    if (m_window->isKeyPressed(KEY_UP) || m_window->isKeyPressed(KEY_W))
        dy = -1;

    if (dx == 0 && dy == 0)
        return;

    m_selectedTileX = ((m_selectedTileX + dx) % m_gridWidth + m_gridWidth) % m_gridWidth;
    m_selectedTileY = ((m_selectedTileY + dy) % m_gridHeight + m_gridHeight) % m_gridHeight;
}
