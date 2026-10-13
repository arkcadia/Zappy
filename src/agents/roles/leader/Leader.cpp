#include "Leader.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"
#include <array>

static constexpr std::array<std::array<int, 7>, 7> INCANT_REQ = {{
    {0, 1, 0, 0, 0, 0, 0},  // 1→2
    {0, 1, 1, 1, 0, 0, 0},  // 2→3
    {0, 2, 0, 1, 0, 2, 0},  // 3→4
    {0, 1, 1, 2, 0, 1, 0},  // 4→5
    {0, 1, 2, 1, 3, 0, 0},  // 5→6
    {0, 1, 2, 3, 0, 1, 0},  // 6→7
    {0, 2, 2, 2, 2, 2, 1},  // 7→8
}};

static const char *RES_NAME[7] = {
    "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame",
};

void Leader::tick(Agent &agent)
{
    switch (_state) {
    case State::CALIBRATE: _doCalibrate(agent); break;
    case State::MANAGE:    _doManage(agent);    break;
    }
}

AgentType Leader::_pickNewRole(bool solo)
{
    ++_totalAssigned;
    if (solo)
        return AgentType::Worker;
    int slot = _totalAssigned % 5;
    if (slot == 3)
        return AgentType::Bodyguard;
    if (slot == 0)
        return AgentType::Chaos;
    return AgentType::Worker;
}

void Leader::_doCalibrate(Agent &agent)
{
    if (_beaconCalibrate(agent)) {
        debug_logf("[Agent {}] Leader: calibrated at (0,0)", agent.ID());
        _state = State::MANAGE;
    }
}

void Leader::_assignRoles(Agent &agent)
{
    while (auto ev = agent.pollBroadcast()) {
        if (ev->msg.code == 5 || ev->msg.code == 1) {
            int id = ev->msg.sender;
            if (std::find(_assignedAgents.begin(), _assignedAgents.end(), id) != _assignedAgents.end())
                continue;
            _assignedAgents.push_back(id);
            AgentType role = _pickNewRole(agent.solo());
            int post = -1;
            if (role == AgentType::Bodyguard)
                post = _bodyguardIndex++ % 4;
            std::string payload = "1 " + std::to_string(id) + " " +
                                  std::to_string(static_cast<int>(role));
            if (post >= 0)
                payload += " " + std::to_string(post);
            debug_logf("[Agent {}] Leader: assign {} -> role {} post={}",
                       agent.ID(), id, static_cast<int>(role), post);
            _enqueueBroadcast(7, payload);
            _hadLatecomer = true;
        }
    }
}

void Leader::_checkIncantation(Agent &agent)
{
    if (agent.level() >= 8)
        return;

    auto tiles = _parseLook(_sendAndRead(agent, "Look"));
    if (tiles.empty())
        return;

    const auto &req = INCANT_REQ[agent.level() - 1];
    std::array<int, 7> tileRes = {};
    for (const auto &item : tiles[0]) {
        if (item == "player") continue;
        for (int i = 0; i < 7; ++i)
            if (item == RES_NAME[i]) { ++tileRes[i]; break; }
    }

    bool ready = true;
    for (int i = 1; i < 7; ++i)
        if (tileRes[i] < req[i]) { ready = false; break; }
    if (ready && tileRes[0] < 5)
        ready = false;

    if (ready) {
        debug_logf("[Agent {}] Leader: incant signal (code-12)", agent.ID());
        _enqueueBroadcast(12);
    } else {
        // Broadcast missing resources
        std::string payload;
        bool need = false;
        for (int i = 1; i < 7; ++i) {
            int d = req[i] - tileRes[i];
            if (d < 0) d = 0;
            if (d > 0) need = true;
            if (!payload.empty()) payload += " ";
            payload += std::to_string(d);
        }
        if (need)
            _enqueueBroadcast(11, payload);
    }
}

void Leader::_doManage(Agent &agent)
{
    _refillFood(agent);
    _assignRoles(agent);

    if (_arrivedTick % 5 == 0)
        _checkIncantation(agent);

    if (!_hasPendingBroadcast())
        _enqueueBroadcast(6);

    if (_arrivedTick % 7 == 0 || _hadLatecomer) {
        _flushBroadcastBuf(agent);
        _hadLatecomer = false;
    } else {
        _sendAndRead(agent, "Inventory");
    }

    ++_arrivedTick;
}
