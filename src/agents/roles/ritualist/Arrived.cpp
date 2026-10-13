#include "Ritualist.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"
#include <array>

static constexpr int PLAYERS_REQ[7] = {1, 2, 2, 4, 4, 6, 6};
static constexpr std::array<std::array<int,7>,7> INCANT_REQ = {{
    {0, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 0, 0, 0},
    {0, 2, 0, 1, 0, 2, 0},
    {0, 1, 1, 2, 0, 1, 0},
    {0, 1, 2, 1, 3, 0, 0},
    {0, 1, 2, 3, 0, 1, 0},
    {0, 2, 2, 2, 2, 2, 1},
}};
static const char *RES[7] = {
    "food","linemate","deraumere","sibur","mendiane","phiras","thystame"
};

static bool tileReady(const std::vector<std::vector<std::string>> &tiles, int level)
{
    if (level < 1 || level > 7 || tiles.empty())
        return false;
    int players = 0;
    std::array<int,7> count = {};
    for (const auto &item : tiles[0]) {
        if (item == "player") { ++players; continue; }
        for (int i = 0; i < 7; ++i)
            if (item == RES[i]) { ++count[i]; break; }
    }
    if (players < PLAYERS_REQ[level - 1])
        return false;
    for (int i = 0; i < 7; ++i)
        if (count[i] < INCANT_REQ[level - 1][i])
            return false;
    return true;
}

void Ritualist::_doIncantation(Agent &agent)
{
    debug_logf("[Agent {}] Ritualist: sending Incantation (lvl {})", agent.ID(), agent.level());

    std::string resp = _sendAndRead(agent, "Incantation");
    if (resp == "ko") {
        debug_logf("[Agent {}] Ritualist: incantation failed (ko)", agent.ID());
        return;
    }

    std::string lvlLine = (resp.compare(0, 13, "Current level") == 0)
        ? resp : _readNext(agent);

    if (lvlLine.compare(0, 13, "Current level") == 0) {
        auto sp = lvlLine.rfind(' ');
        if (sp != std::string::npos) {
            try { agent.setLevel(std::stoi(lvlLine.substr(sp + 1))); } catch (...) {}
        }
        debug_logf("[Agent {}] Ritualist: now lvl {}", agent.ID(), agent.level());
    }
}

void Ritualist::_doArrived(Agent &agent)
{
    _refillFood(agent);

    if (_arrivedTick % 10 == 0 && agent.inventory().get(Resource::Food) >= 4) {
        std::string resp = _sendAndRead(agent, "Connect_nbr");
        int eggs = 0;
        try { eggs = std::stoi(resp); } catch (...) {}
        if (eggs < 4) {
            debug_logf("[Agent {}] Ritualist: eggs={} < 4, forking", agent.ID(), eggs);
            _sendAndRead(agent, "Fork");
        }
    }

    while (agent.pollBroadcast()) {}

    if (agent.level() < 8 && _arrivedTick % 5 == 0
        && agent.inventory().get(Resource::Food) >= 4) {
        auto tiles = _parseLook(_sendAndRead(agent, "Look"));
        if (tileReady(tiles, agent.level())) {
            _doIncantation(agent);
            ++_arrivedTick;
            return;
        }
    }

    if (!_hasPendingBroadcast())
        _enqueueBroadcast(6);

    if (_arrivedTick % 7 == 0)
        _flushBroadcastBuf(agent);
    else
        _sendAndRead(agent, "Inventory");

    ++_arrivedTick;
}
