#include "Pioneer.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"
#include <array>
#include <cstdlib>

static constexpr std::array<std::array<int,7>,7> INCANT_REQ = {{
    {0, 1, 0, 0, 0, 0, 0},  // 1→2
    {0, 1, 1, 1, 0, 0, 0},  // 2→3
    {0, 2, 0, 1, 0, 2, 0},  // 3→4
    {0, 1, 1, 2, 0, 1, 0},  // 4→5
    {0, 1, 2, 1, 3, 0, 0},  // 5→6
    {0, 1, 2, 3, 0, 1, 0},  // 6→7
    {0, 2, 2, 2, 2, 2, 1},  // 7→8
}};

static const char *RES[7] = {
    "food","linemate","deraumere","sibur","mendiane","phiras","thystame"
};

static bool tileCanIncantate(const std::vector<std::string> &tile, int level)
{
    if (level < 1 || level > 7) return false;
    std::array<int,7> count = {};
    int players = 0;
    for (const auto &item : tile) {
        if (item == "player") { ++players; continue; }
        for (int i = 0; i < 7; ++i)
            if (item == RES[i]) { ++count[i]; break; }
    }
    if (players == 0) return false;
    for (int i = 0; i < 7; ++i)
        if (count[i] < INCANT_REQ[level - 1][i]) return false;
    return true;
}

void Pioneer::_doCollect(Agent &agent)
{
    _processBroadcasts(agent);
    if (_state != State::COLLECT) return;

    if (_localTick % 20 == 0)
        _sendHeartbeat(agent);

    _parseInventory(agent, _sendAndRead(agent, "Inventory"));
    Inventory &bag = agent.inventory();

    auto tiles = _parseLook(_sendAndRead(agent, "Look"));

    if (!tiles.empty()) {
        for (const auto &item : tiles[0]) {
            if (item == "food" && _sendAndRead(agent, "Take food") == "ok") {
                bag.add(Resource::Food, 1);
                verbose_logf("[Agent {}] Pioneer: picked up food, bag now {}", agent.ID(), bag.get(Resource::Food));
            }
        }
    }

    if (_localTick > 30 && bag.get(Resource::Food) <= 2 + 1) {
        int lateral = 0;
        if (!tiles.empty() && _findNearestFood(tiles, lateral)) {
            debug_logf("[Agent {}] Pioneer: low food ({}), chasing visible food tile (lateral={})",
                       agent.ID(), bag.get(Resource::Food), lateral);
            if (lateral < 0)      _turnLeft(agent);
            else if (lateral > 0) _turnRight(agent);
            else                  _moveForward(agent);
        } else {
            debug_logf("[Agent {}] Pioneer: low food ({}), no food visible, wandering forward",
                       agent.ID(), bag.get(Resource::Food));
            _moveForward(agent);
        }
        return;
    }

    int total = 0, atLvl2 = 0;
    _countFreshPeers(agent, total, atLvl2);
    verbose_logf("[Agent {}] Pioneer: fresh peers total={} atLvl2={}", agent.ID(), total, atLvl2);

    if (!tiles.empty() && agent.level() < 2 && tileCanIncantate(tiles[0], agent.level())) {
        debug_logf("[Agent {}] Pioneer: tile can incantate (lvl {}), switching to INCANTATE",
                   agent.ID(), agent.level());
        _state = State::INCANTATE;
        return;
    }

    if (_isLeader && bag.get(Resource::Food) >= 5 && total < 7 && _localTick % 10 == 0) {
        debug_logf("[Agent {}] Pioneer: lvl={} food={} total={} < target, switching to FORK",
                   agent.ID(), agent.level(), bag.get(Resource::Food), total);
        _state = State::FORK;
        return;
    }

    if (_localTick <= 3) {
        int turns = rand() % 4;
        for (int i = 0; i < turns; ++i)
            _turnRight(agent);
        _moveForward(agent);
        return;
    }

    int lateral = 0;
    bool hasPlayerNearby = false;
    if (!tiles.empty()) {
        for (std::size_t i = 1; i < std::min(tiles.size(), (std::size_t)4); ++i)
            for (const auto &item : tiles[i])
                if (item == "player") { hasPlayerNearby = true; break; }
    }

    if (hasPlayerNearby && !_isLeader) {
        (rand() % 2 == 0) ? _turnLeft(agent) : _turnRight(agent);
        _moveForward(agent);
    } else if (!tiles.empty() && _findNearestFood(tiles, lateral)) {
        if (lateral < 0)      { _turnLeft(agent);  _moveForward(agent); }
        else if (lateral > 0) { _turnRight(agent); _moveForward(agent); }
        else                  _moveForward(agent);
    } else {
        _moveForward(agent);
        if (rand() % 3 == 0)
            (rand() % 2 == 0) ? _turnLeft(agent) : _turnRight(agent);
    }
}
