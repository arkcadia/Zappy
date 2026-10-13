#include "Graphical3d.hpp"
#include "camera.h"
#include <algorithm>
#include <raylib.h>

static constexpr float TILE_SPACING = 0.625f;

void Graphical3D::initCamera()
{
    m_camera.up = {0.0f, 1.0f, 0.0f};
    m_camera.fovy = 45.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;
    m_camera.position = {10.0f, 15.0f, 20.0f};
    m_camera.target = {10.0f, 0.0f,  10.0f};
}

void Graphical3D::resetCamera()
{
    float cx = (m_gridWidth - 1) * TILE_SPACING * 0.5f;
    float cz = (m_gridHeight - 1) * TILE_SPACING * 0.5f;
    float dist = std::max(m_gridWidth, m_gridHeight) * TILE_SPACING * 0.75f;

    m_camera.target = {cx, 0.0f, cz};
    m_camera.position = {cx, dist * 0.8f, cz + dist};
}

void Graphical3D::handleInput()
{
    if (IsKeyPressed(KEY_ESCAPE))
        m_running = false;
    if (IsKeyPressed(KEY_R))
        resetCamera();
    UpdateCameraWithKeyboard(m_camera);
}