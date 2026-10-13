#pragma once

#include <cstdint>
#include <functional>
#include <string>

class World;

enum class GuiProtocolAnimationType {
    Spawn,
    Move,
    Broadcast,
    Pickup,
    Drop,
    Eject,
    Fork,
    EggLaid,
    EggHatch,
    EggDeath,
    IncantationStart,
    IncantationEnd,
    Death,
    LevelUp,
};

struct GuiProtocolAnimation {
    GuiProtocolAnimationType type = GuiProtocolAnimationType::Spawn;
    float x = 0.0f;
    float y = 0.0f;
    std::string teamName;
    int resourceIndex = -1;
    bool success = true;
    std::string label;
    int playerId = -1;
    float duration = 0.8f;
};

struct GuiProtocolContext {
    World *world = nullptr;
    bool mapReceived = false;
    uint16_t gridWidth = 0;
    uint16_t gridHeight = 0;
    float time = 0.0f;

    std::function<void(uint16_t, uint16_t)> initializeWorld;
    std::function<void(const std::string &)> sendLine;
    std::function<void(const std::string &)> pushLog;
    std::function<void(const GuiProtocolAnimation &)> addAnimation;
    std::function<void()> updateMapStats;
};

bool handleGuiProtocolMessage(const std::string &message, GuiProtocolContext &context);
