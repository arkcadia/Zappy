#include "config.hpp"
#include "graphical.hpp"
#include "window.hpp"
#include <algorithm>
#include <raylib.h>

void Graphical::resetCamera() {
    float gridW = static_cast<float>(m_gridWidth * m_tileSize);
    float gridH = static_cast<float>(m_gridHeight * m_tileSize);
    float sw = static_cast<float>(m_window->getWidth());
    float sh = static_cast<float>(m_window->getHeight());
    m_camera.offsetX = (sw - gridW) * 0.5f;
    m_camera.offsetY = (sh - gridH) * 0.5f;
    m_camera.zoom = 1.0f;
}

Vector2 Graphical::worldToScreen(float wx, float wy) const {
    float ts = static_cast<float>(m_tileSize) * m_camera.zoom;
    return {m_camera.offsetX + wx * ts, m_camera.offsetY + wy * ts};
}

Vector2 Graphical::screenToWorld(float sx, float sy) const {
    float ts = static_cast<float>(m_tileSize) * m_camera.zoom;
    return {(sx - m_camera.offsetX) / ts, (sy - m_camera.offsetY) / ts};
}

void Graphical::updateCamera() {
    Vector2 mouse = m_window->getMousePosition();

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        float prevZoom = m_camera.zoom;
        m_camera.zoom = std::clamp(m_camera.zoom + wheel * Config::ZOOM_STEP, Config::MIN_ZOOM,
                                   Config::MAX_ZOOM);
        float zoomRatio = m_camera.zoom / prevZoom;
        m_camera.offsetX = mouse.x - (mouse.x - m_camera.offsetX) * zoomRatio;
        m_camera.offsetY = mouse.y - (mouse.y - m_camera.offsetY) * zoomRatio;
    }

    if (m_window->isMouseButtonPressed(MOUSE_BUTTON_MIDDLE) ||
        m_window->isMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        m_camera.dragging = true;
        m_camera.dragStartX = mouse.x;
        m_camera.dragStartY = mouse.y;
        m_camera.dragOriginX = m_camera.offsetX;
        m_camera.dragOriginY = m_camera.offsetY;
    }

    if (m_camera.dragging) {
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            m_camera.offsetX = m_camera.dragOriginX + (mouse.x - m_camera.dragStartX);
            m_camera.offsetY = m_camera.dragOriginY + (mouse.y - m_camera.dragStartY);
        } else {
            m_camera.dragging = false;
        }
    }

    if (m_window->isKeyPressed(KEY_R))
        resetCamera();

    Vector2 worldPos = screenToWorld(mouse.x, mouse.y);
    m_hoveredTileX = static_cast<int>(worldPos.x);
    m_hoveredTileY = static_cast<int>(worldPos.y);
    if (!m_mapReceived || m_hoveredTileX < 0 || m_hoveredTileX >= m_gridWidth ||
        m_hoveredTileY < 0 || m_hoveredTileY >= m_gridHeight) {
        m_hoveredTileX = -1;
        m_hoveredTileY = -1;
    }
}
