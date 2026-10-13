#include "Worker.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"
#include <cstdlib>
#include <cmath>

static const char *RES[] = {
    "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame",
};

static bool _findFoodNoPlayers(const std::vector<std::vector<std::string>> &tiles, int &lateral)
{
    int bestDist = -1;
    for (std::size_t i = 1; i < tiles.size(); ++i) {
        bool hasFood = false, hasPlayer = false;
        for (const auto &item : tiles[i]) {
            if (item == "food")   hasFood = true;
            if (item == "player") hasPlayer = true;
        }
        if (!hasFood || hasPlayer) continue;

        int r    = static_cast<int>(std::sqrt(static_cast<double>(i)));
        int o    = static_cast<int>(i) - r * r;
        int lat  = o - r;
        int dist = r + std::abs(lat);
        if (bestDist < 0 || dist < bestDist) {
            bestDist = dist;
            lateral  = lat;
        }
    }
    return bestDist >= 0;
}

static bool _findNearestPlayerLateral(const std::vector<std::vector<std::string>> &tiles, int &lateral)
{
    int bestDist = -1;
    for (std::size_t i = 1; i < tiles.size(); ++i) {
        int players = 0;
        for (const auto &item : tiles[i])
            if (item == "player") ++players;
        if (players == 0) continue;

        int r    = static_cast<int>(std::sqrt(static_cast<double>(i)));
        int o    = static_cast<int>(i) - r * r;
        int lat  = o - r;
        int dist = r + std::abs(lat);
        if (bestDist < 0 || dist < bestDist) {
            bestDist = dist;
            lateral  = lat;
        }
    }
    return bestDist >= 0;
}

void Worker::_doGather(Agent &agent)
{
    while (agent.pollBroadcast()) {}

    _parseInventory(agent, _sendAndRead(agent, "Inventory"));

    if (agent.inventory().get(Resource::Food) >= 8) {
        debug_logf("[Agent {}] Worker: food {} >= {}, switching to DELIVER",
                   agent.ID(), agent.inventory().get(Resource::Food), 8);
        _state = State::DELIVER;
        return;
    }

    auto tiles = _parseLook(_sendAndRead(agent, "Look"));

    bool atBase = (agent.posX() == 0 && agent.posY() == 0);
    if (!atBase && !tiles.empty()) {
        for (const auto &item : tiles[0]) {
            if (item == "player") continue;
            for (int i = 0; i < 7; ++i) {
                if (item == RES[i]) {
                    if (_sendAndRead(agent, std::string("Take ") + RES[i]) == "ok")
                        agent.inventory().add(static_cast<Resource>(i), 1);
                    break;
                }
            }
        }
    }

    int lateral = 0;
    if (!tiles.empty() && _findFoodNoPlayers(tiles, lateral)) {
        if (lateral < 0)      { _turnLeft(agent);  _moveForward(agent); }
        else if (lateral > 0) { _turnRight(agent); _moveForward(agent); }
        else                  _moveForward(agent);
    } else if (!tiles.empty() && _findNearestFood(tiles, lateral)) {
        if (lateral < 0)      { _turnLeft(agent);  _moveForward(agent); }
        else if (lateral > 0) { _turnRight(agent); _moveForward(agent); }
        else                  _moveForward(agent);
    } else {
        int playerLat = 0;
        if (!tiles.empty() && _findNearestPlayerLateral(tiles, playerLat)) {
            if (playerLat <= 0) _turnRight(agent);
            else                _turnLeft(agent);
            _moveForward(agent);
        } else {
            if (rand() % 2 == 0)
                (rand() % 2 == 0) ? _turnLeft(agent) : _turnRight(agent);
            _moveForward(agent);
        }
    }
}
