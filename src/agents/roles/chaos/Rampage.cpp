#include "Chaos.hpp"
#include "../../core/Agent.hpp"
#include "../../core/Broadcast.hpp"
#include "../../../debug.hpp"
#include <cstdlib>

bool Chaos::_nearOwnBase(const Agent &agent) const
{
    int mx = agent.mapX(), my = agent.mapY();
    int dx = std::min(agent.posX(), mx - agent.posX());
    int dy = std::min(agent.posY(), my - agent.posY());
    return (dx + dy) <= std::max(mx, my) / 3;
}

void Chaos::_doCalibrate(Agent &agent)
{
    if (_beaconCalibrate(agent)) {
        debug_logf("[Agent {}] Chaos: calibrated at (0,0), departing", agent.ID());
        _state = State::DEPART;
    }
}

void Chaos::_doDepart(Agent &agent)
{
    while (agent.pollBroadcast()) {}
    while (agent.pollEnemyBroadcastDir()) {}

    _moveForward(agent);
    _moveForward(agent);
    _moveForward(agent);
    debug_logf("[Agent {}] Chaos: departed from base, entering HUNT", agent.ID());
    _state = State::HUNT;
}

void Chaos::_doHunt(Agent &agent)
{
    _parseInventory(agent, _sendAndRead(agent, "Inventory"));

    while (agent.pollBroadcast()) {}

    while (auto ev = agent.pollEnemyBroadcastDir()) {
        _enemyDir = ev->first;
        debug_logf("[Agent {}] Chaos: enemy broadcast detected, dir={}", agent.ID(), _enemyDir);
    }

    if (_nearOwnBase(agent)) {
        debug_logf("[Agent {}] Chaos: near own base, moving away", agent.ID());
        _navigateAwayFrom(agent, 0);
        return;
    }

    auto tiles = _parseLook(_sendAndRead(agent, "Look"));

    if (!tiles.empty()) {
        int playersHere = 0;
        for (const auto &item : tiles[0])
            if (item == "player") ++playersHere;

        if (playersHere > 1) {
            debug_logf("[Agent {}] Chaos: {} players on tile, ejecting!", agent.ID(), playersHere);
            _sendAndRead(agent, "Eject");
            return;
        }
    }

    if (_enemyDir >= 0) {
        if (_enemyDir == 0) {
            _moveForward(agent);
            if (rand() % 2 == 0)
                (rand() % 2 == 0) ? _turnLeft(agent) : _turnRight(agent);
        } else {
            _stepTowardBeacon(agent, _enemyDir, _justTurned);
        }
        return;
    }

    int lateral = 0;
    if (!tiles.empty() && _findNearestPlayer(tiles, lateral)) {
        if (lateral < 0)      { _turnLeft(agent);  _moveForward(agent); }
        else if (lateral > 0) { _turnRight(agent); _moveForward(agent); }
        else                  _moveForward(agent);
    } else {
        _moveForward(agent);
        if (rand() % 3 == 0)
            (rand() % 2 == 0) ? _turnLeft(agent) : _turnRight(agent);
    }
}
