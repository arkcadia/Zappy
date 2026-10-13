#include "Graphical3d.hpp"
#include "gui_protocol_handler.hpp"
#include "network_client.hpp"
#include <algorithm>
#include <raylib.h>

void Graphical3D::update(float dt)
{
    m_time += dt;
    if (m_musicLoaded)
        UpdateMusicStream(m_music);
    for (auto &entry : m_log)
        entry.alpha -= dt * 0.18f;
    auto isDead = [](const LogEntry3D &e) { return e.alpha <= 0.0f; };
    m_log.erase(std::remove_if(m_log.begin(), m_log.end(), isDead), m_log.end());
    processNetworkMessages();
}

void Graphical3D::processNetworkMessages()
{
    if (!m_networkClient.isConnected())
        return;

    std::string msg;
    while (!(msg = m_networkClient.readLine()).empty())
        handleServerMessage(msg);
}

void Graphical3D::handleServerMessage(const std::string &message)
{
    if (message.substr(0, 3) == "pnw" || message.substr(0, 3) == "tna")
        printf("MSG: %s\n", message.c_str());
    if (message.substr(0, 3) == "plv")
        printf("MSG: %s\n", message.c_str());
    GuiProtocolContext context;

    context.world = m_world.get();
    context.mapReceived = m_mapReceived;
    context.gridWidth = m_gridWidth;
    context.gridHeight = m_gridHeight;
    context.time = m_time;

    context.initializeWorld = [&](uint16_t w, uint16_t h) {
        initializeWorld(w, h);
        context.world = m_world.get();
        context.mapReceived = m_mapReceived;
        context.gridWidth = m_gridWidth;
        context.gridHeight = m_gridHeight;
    };
    context.sendLine = [&](const std::string &line) { m_networkClient.sendLine(line); };
    context.pushLog = [&](const std::string &text) { pushLog(text); };
    context.updateMapStats = [&]() { updateMapStats(); };
    context.addAnimation = [](const GuiProtocolAnimation &) {};

    handleGuiProtocolMessage(message, context);
}