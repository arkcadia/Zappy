#include "Pioneer.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"
#include <iostream>

void Pioneer::_processBroadcasts(Agent &agent)
{
    ++_localTick;
    _updateLeader(agent, agent.ID());
    bool gotCode6 = false;
    bool gotCode8 = false;
    while (auto ev = agent.pollBroadcast()) {
        if (ev->msg.code == 0 || ev->msg.code == 1) {
            _updateLeader(agent, ev->msg.sender);
            try { _agentLevels[ev->msg.sender] = {std::stoi(ev->msg.payload), _localTick}; } catch (...) {}
        } else if (ev->msg.code == 7) {
            if (_parseRoleAssignment(agent, ev->msg.payload)) {
                debug_logf("[Agent {}] Pioneer: recalled by Phase-2 leader, transitioning", agent.ID());
                _transitionToPhase2 = true;
                return;
            }
        } else if (ev->msg.code == 6) {
            gotCode6 = true;
        } else if (ev->msg.code == 8) {
            gotCode8 = true;
        }
    }
    if (gotCode8) {
        debug_logf("[Agent {}] Pioneer: code-8, transitioning to phase 2", agent.ID());
        _transitionToPhase2 = true;
        return;
    }
    if (gotCode6 && _state == State::COLLECT) {
        debug_logf("[Agent {}] Pioneer: code-6 rally beacon, switching to SETTLE", agent.ID());
        _state = State::SETTLE;
        return;
    }

    if (_isLeader && _state == State::COLLECT) {
        _agentLevels[agent.ID()] = {agent.level(), _localTick};
        int total = 0, atLvl2 = 0;
        _countFreshPeers(agent, total, atLvl2);

        bool full     = (total >= 7 && atLvl2 >= 7);
        bool allReady = (atLvl2 >= total && total >= 7
                         && _localTick > 100);
        if (full || allReady) {
            debug_logf("[Agent {}] phase 2 rally: {} agents, {} lvl2+ ({})",
                       agent.ID(), total, atLvl2, full ? "full" : "all-ready");
            _settleTarget = total - 1;
            agent.setPos(0, 0);
            _arrivedAtRally = true;
            _broadcast(agent, 6);
            _state = State::SETTLE;
        }
    }
}

void Pioneer::_doAnnounce(Agent &agent)
{
    _broadcast(agent, 0, std::to_string(agent.level()));
    _processBroadcasts(agent);
    if (_state != State::ANNOUNCE)
        return;

    if (++_announceTicks < 3)
        return;

    if (_isLeader)
        debug_logf("[Agent {}] Leader_ID = {}", agent.ID(), _leaderID);
    debug_logf("[Agent {}] Pioneer: ANNOUNCE -> COLLECT", agent.ID());
    _state = State::COLLECT;
}
