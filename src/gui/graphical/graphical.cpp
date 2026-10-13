#include "graphical.hpp"
#include "../network/network_client.hpp"
#include "../utils/utils.hpp"
#include "config.hpp"
#include "gui_protocol_handler.hpp"
#include "ressource_renderer.hpp"
#include "window.hpp"
#include "world.hpp"
#include <algorithm>

static InteractionAnimationType toInteractionAnimationType(GuiProtocolAnimationType type)
{
    switch (type) {
    case GuiProtocolAnimationType::Spawn:
        return InteractionAnimationType::Spawn;
    case GuiProtocolAnimationType::Move:
        return InteractionAnimationType::Move;
    case GuiProtocolAnimationType::Broadcast:
        return InteractionAnimationType::Broadcast;
    case GuiProtocolAnimationType::Pickup:
        return InteractionAnimationType::Pickup;
    case GuiProtocolAnimationType::Drop:
        return InteractionAnimationType::Drop;
    case GuiProtocolAnimationType::Eject:
        return InteractionAnimationType::Eject;
    case GuiProtocolAnimationType::Fork:
        return InteractionAnimationType::Fork;
    case GuiProtocolAnimationType::EggLaid:
        return InteractionAnimationType::EggLaid;
    case GuiProtocolAnimationType::EggHatch:
        return InteractionAnimationType::EggHatch;
    case GuiProtocolAnimationType::EggDeath:
        return InteractionAnimationType::EggDeath;
    case GuiProtocolAnimationType::IncantationStart:
        return InteractionAnimationType::IncantationStart;
    case GuiProtocolAnimationType::IncantationEnd:
        return InteractionAnimationType::IncantationEnd;
    case GuiProtocolAnimationType::Death:
        return InteractionAnimationType::Death;
    case GuiProtocolAnimationType::LevelUp:
        return InteractionAnimationType::LevelUp;
    }
    return InteractionAnimationType::Spawn;
}

Graphical::Graphical(Network::NetworkClient &networkClient)
    : m_networkClient(networkClient),
      m_window(std::make_unique<Window>(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT,
                                        Config::WINDOW_TITLE)),
      m_world(nullptr), m_resourceRenderer(std::make_unique<ResourceRenderer>()), m_running(true),
      m_mapReceived(false), m_gridWidth(0), m_gridHeight(0), m_tileSize(Config::BASE_TILE_SIZE),
      m_time(0.0f), m_hoveredTileX(-1), m_hoveredTileY(-1), m_selectedTileX(0), m_selectedTileY(0),
      m_hasSelection(false), m_mapX(Config::PANEL_W), m_mapY(Config::HUD_H),
      m_mapW(Config::WINDOW_WIDTH - Config::PANEL_W * 2),
      m_mapH(Config::WINDOW_HEIGHT - Config::HUD_H - Config::STRIP_H), m_totalFood(0),
      m_totalMinerals(0), m_filledTiles(0), m_bgTexture{}, m_bgLoaded(false), m_font{},
      m_fontLoaded(false) {
    m_bgTexture = LoadTexture(Config::BG_IMAGE_PATH);
    if (m_bgTexture.id != 0) {
        SetTextureFilter(m_bgTexture, TEXTURE_FILTER_BILINEAR);
        m_bgLoaded = true;
    }

    m_font = LoadFontEx(Config::FONT_PATH, 32, nullptr, 0);
    if (m_font.texture.id != 0) {
        SetTextureFilter(m_font.texture, TEXTURE_FILTER_BILINEAR);
        m_fontLoaded = true;
    }
}

Graphical::~Graphical() {
    if (m_bgLoaded)
        UnloadTexture(m_bgTexture);
    if (m_fontLoaded)
        UnloadFont(m_font);
}

void Graphical::initializeWorld(uint16_t w, uint16_t h) {
    m_gridWidth = w;
    m_gridHeight = h;
    m_tileSize = Utils::calculateTileSize(w, h);
    m_world = std::make_unique<World>(w, h);
    m_mapReceived = true;
    m_hasSelection = true;
    m_selectedTileX = 0;
    m_selectedTileY = 0;
    initLayout();
    resetCamera();
    pushLog("MAP " + std::to_string(w) + "x" + std::to_string(h) + " LOADED");
}

void Graphical::run() {
    while (m_running && m_window->isOpen()) {
        handleInput();
        update(m_window->getDeltaTime());
        render();
    }
}

void Graphical::update(float dt) {
    m_time += dt;

    for (auto &entry : m_log)
        entry.alpha -= dt * 0.18f;
    m_log.erase(std::remove_if(m_log.begin(), m_log.end(),
                               [](const LogEntry &e) { return e.alpha <= 0.0f; }),
                m_log.end());

    updateInteractionAnimations();
    processNetworkMessages();
}

void Graphical::addInteractionAnimation(InteractionAnimationType type, float x, float y,
                                        const std::string &teamName, int resourceIndex,
                                        bool success, const std::string &label, int playerId,
                                        float duration)
{
    InteractionAnimation animation;

    animation.type = type;
    animation.x = x;
    animation.y = y;
    animation.startedAt = m_time;
    animation.duration = duration > 0.0f ? duration : 0.8f;
    animation.resourceIndex = resourceIndex;
    animation.success = success;
    animation.teamName = teamName;
    animation.label = label;
    animation.playerId = playerId;
    m_interactionAnimations.push_back(animation);
    if (m_interactionAnimations.size() > 128)
        m_interactionAnimations.erase(m_interactionAnimations.begin());
}

void Graphical::updateInteractionAnimations()
{
    if (m_interactionAnimations.empty())
        return;

    m_interactionAnimations.erase(
        std::remove_if(m_interactionAnimations.begin(), m_interactionAnimations.end(),
                       [&](const InteractionAnimation &animation) {
                           return m_time - animation.startedAt >= animation.duration;
                       }),
        m_interactionAnimations.end());
}

void Graphical::processNetworkMessages() {
    if (!m_networkClient.isConnected())
        return;
    std::string msg;
    while (!(msg = m_networkClient.readLine()).empty())
        handleServerMessage(msg);
}

void Graphical::handleServerMessage(const std::string &message) {
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
    context.addAnimation = [&](const GuiProtocolAnimation &animation) {
        addInteractionAnimation(toInteractionAnimationType(animation.type), animation.x, animation.y,
                                animation.teamName, animation.resourceIndex, animation.success,
                                animation.label, animation.playerId, animation.duration);
    };
    context.updateMapStats = [&]() { updateMapStats(); };
    handleGuiProtocolMessage(message, context);
}

void Graphical::pushLog(const std::string &text) {
    m_log.push_back({text, 6.0f});
    if (m_log.size() > 24)
        m_log.erase(m_log.begin());
}

void Graphical::updateMapStats() {
    if (!m_world)
        return;

    m_totalFood = 0;
    m_totalMinerals = 0;
    m_filledTiles = 0;

    for (uint16_t y = 0; y < m_gridHeight; y++) {
        for (uint16_t x = 0; x < m_gridWidth; x++) {
            const Tile &t = m_world->getTile(x, y);
            bool hasAny = false;
            for (uint8_t r = 0; r < static_cast<uint8_t>(ResourceType::Count); r++) {
                uint16_t amt = t.resources.get(static_cast<ResourceType>(r));
                if (amt > 0) {
                    hasAny = true;
                    if (r == 0)
                        m_totalFood += amt;
                    else
                        m_totalMinerals += amt;
                }
            }
            if (hasAny)
                m_filledTiles++;
        }
    }
}
