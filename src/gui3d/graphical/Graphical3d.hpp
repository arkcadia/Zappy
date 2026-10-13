#pragma once
#include "world.hpp"
#include "gui_protocol_handler.hpp"
#include <cstdint>
#include <memory>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

class World;

namespace Network {
class NetworkClient;
}

struct LogEntry3D {
    std::string text;
    float alpha;
};

struct PlayerAnimState {
    int currentAnim = 2;
    int frame = 0;
    float lastEventTime = -999.0f;
};

class Graphical3D {
public:
    explicit Graphical3D(Network::NetworkClient &networkClient);
    ~Graphical3D();

    void run();

    World &getWorld() { return *m_world; }
    const World &getWorld() const { return *m_world; }

private:
    Network::NetworkClient &m_networkClient;
    std::unique_ptr<World> m_world;

    Camera m_camera{};
    bool m_running = true;
    bool m_mapReceived = false;
    uint16_t m_gridWidth = 0;
    uint16_t m_gridHeight = 0;
    float m_time = 0.0f;

    GuiProtocolContext m_protocolCtx;

    Model m_tileModel{};
    bool m_tileModelLoaded = false;
    Model m_playerModel{};
    bool m_playerModelLoaded = false;
    Font m_font{};
    bool m_fontLoaded = false;
    Music m_music{};
    bool m_musicLoaded = false;

    std::unordered_map<int, PlayerAnimState> m_playerAnims;

    std::vector<LogEntry3D> m_log;
    int m_totalFood = 0;
    int m_totalMinerals = 0;
    int m_filledTiles = 0;

    void handleInput();
    void update(float dt);
    void processNetworkMessages();
    void handleServerMessage(const std::string &msg);
    void initializeWorld(uint16_t w, uint16_t h);
    void updateMapStats();
    void pushLog(const std::string &text);

    void render();
    void renderWorld();
    void renderPlayers();
    void renderHUD();
    void renderTeamPanel();
    void renderWaiting();
    void renderLog();
    void renderOnePlayer(const Player &player);
    void renderEggs();
    void renderPlayerLabels();

    void drawTile(uint16_t x, uint16_t y, const Vector3& center);
    void drawTileResources(const Tile& tile, const Vector3& center, float anim);
    void drawResourceCube(float x, float y, float z, float scale, ::Color);

    void initCamera();
    void resetCamera();

    Vector3 tileCenter(uint16_t x, uint16_t y) const;
    void drawText(const char *text, int x, int y, int size, Color color) const;
};