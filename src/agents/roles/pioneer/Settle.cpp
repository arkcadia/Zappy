#include "Pioneer.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

void Pioneer::_settleNavigate(Agent &agent)
{
    bool gotFresh = false;
    while (auto ev = agent.pollBroadcast()) {
        if (ev->msg.code == 6) {
            _settleDir = ev->dir;
            gotFresh   = true;
            verbose_logf("[Agent {}] Pioneer settle: rally beacon dir={}", agent.ID(), _settleDir);
        } else if (ev->msg.code == 7) {
            _parseRoleAssignment(agent, ev->msg.payload);
        } else if (ev->msg.code == 8) {
            debug_logf("[Agent {}] Pioneer settle: code-8 received, transitioning to phase 2", agent.ID());
            _transitionToPhase2 = true;
            return;
        }
    }

    if (gotFresh && _settleDir == 0 && !_arrivedAtRally) {
        debug_logf("[Agent {}] Pioneer settle: arrived at rally point (0,0)", agent.ID());
        agent.setPos(0, 0);
        _arrivedAtRally = true;
        _broadcast(agent, 5);
        return;
    }

    if (_transitionToPhase2 || _arrivedAtRally)
        return;

    if (_settlePostForward) {
        _settlePostForward = false;
        _sendAndRead(agent, "Inventory");
        return;
    }

    if (!gotFresh || _settleDir < 0) {
        _sendAndRead(agent, "Inventory");
        return;
    }

    if (_stepTowardBeacon(agent, _settleDir, _settleJustTurned))
        _settlePostForward = true;
}

AgentType Pioneer::_pickRole(int level)
{
    if (level >= 2 && _ritualistSlotsLeft > 0) {
        --_ritualistSlotsLeft;
        return AgentType::Ritualist;
    }
    return AgentType::Worker;
}

void Pioneer::_assignSingleRole(Agent &agent, int targetId)
{
    int level = 1;
    auto it = _agentLevels.find(targetId);
    if (it != _agentLevels.end())
        level = it->second.first;

    AgentType role = _pickRole(level);
    std::string payload = "1 " + std::to_string(targetId) + " " +
                          std::to_string(static_cast<int>(role));
    debug_logf("[Agent {}] Pioneer settle (leader): assign {} (lvl {}) -> role {}",
               agent.ID(), targetId, level, static_cast<int>(role));
    _enqueueBroadcast(7, payload);
}

void Pioneer::_assignSelfAndFinalize(Agent &agent)
{
    std::vector<std::pair<int,int>> unarrived;
    for (const auto &[id, lvlTick] : _agentLevels) {
        if (id == agent.ID()) continue;
        if (_settleArrivals.count(id)) continue;
        unarrived.push_back({id, lvlTick.first});
    }
    std::sort(unarrived.begin(), unarrived.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    if (!unarrived.empty()) {
        std::string payload = std::to_string(static_cast<int>(unarrived.size()));
        for (auto &[id, lvl] : unarrived) {
            AgentType role = _pickRole(lvl);
            payload += " " + std::to_string(id) + " " + std::to_string(static_cast<int>(role));
            debug_logf("[Agent {}] Pioneer settle (leader): batch-assign {} (lvl {}) -> role {}",
                       agent.ID(), id, lvl, static_cast<int>(role));
        }
        _enqueueBroadcast(7, payload);
    }

    _assignedRole = AgentType::Leader;
    debug_logf("[Agent {}] Pioneer settle (leader): self -> Leader, sending code-8 catch-all",
               agent.ID());
    _enqueueBroadcast(8);
    _flushBroadcastBuf(agent);
    _transitionToPhase2 = true;
}

void Pioneer::_doSettle(Agent &agent)
{
    if (_settleTarget > 0) {
        bool wasArrival = false;
        while (auto ev = agent.pollBroadcast()) {
            if (ev->msg.code == 5) {
                int arrivalId = ev->msg.sender;
                if (!_settleArrivals.count(arrivalId)) {
                    _settleArrivals.insert(arrivalId);
                    _assignSingleRole(agent, arrivalId);
                    wasArrival = true;
                    debug_logf("[Agent {}] Pioneer settle (leader): rally {}/{} arrived, role assigned",
                               agent.ID(), _settleArrivals.size(), _settleTarget);
                }
            }
        }

        bool doFlush = (_settleTicks % 7 == 0);
        if (!_hasPendingBroadcast())
            _enqueueBroadcast(6);

        bool allAssigned = (static_cast<int>(_settleArrivals.size()) >= _settleTarget);
        bool timeout     = (++_settleTicks >= 80);

        if (allAssigned || timeout) {
            debug_logf("[Agent {}] Pioneer settle (leader): rally done ({}{}), finalizing",
                       agent.ID(), allAssigned ? "all assigned" : "timeout",
                       allAssigned ? "" : ", proceeding anyway");
            _assignSelfAndFinalize(agent);
        } else if (wasArrival || doFlush) {
            _flushBroadcastBuf(agent);
        } else {
            _sendAndRead(agent, "Inventory");
        }
    } else {
        if (++_settleTicks >= 80 * 3) {
            debug_logf("[Agent {}] Pioneer settle: timeout waiting for leader, defaulting Worker", agent.ID());
            _transitionToPhase2 = true;
            return;
        }

        if (_transitionToPhase2)
            return;

        if (_arrivedAtRally) {
            while (auto ev = agent.pollBroadcast()) {
                if (ev->msg.code == 7) {
                    if (_parseRoleAssignment(agent, ev->msg.payload)) {
                        debug_logf("[Agent {}] Pioneer settle: role assigned via code-7, departing",
                                   agent.ID());
                        _transitionToPhase2 = true;
                        return;
                    }
                } else if (ev->msg.code == 8) {
                    debug_logf("[Agent {}] Pioneer settle: code-8 received, transitioning to phase 2",
                               agent.ID());
                    _transitionToPhase2 = true;
                    return;
                }
            }
            _sendAndRead(agent, "Inventory");
        } else {
            _settleNavigate(agent);
        }
    }
}
