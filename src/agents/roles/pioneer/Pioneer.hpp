#pragma once
#include "../ARole.hpp"
#include "../../core/Broadcast.hpp"
#include <map>
#include <set>
#include <string>

class Agent;

class Pioneer : public ARole {
public:
    void tick(Agent &agent) override;

private:
    enum class State { ANNOUNCE, COLLECT, INCANTATE, FORK, SETTLE };

    State _state    = State::ANNOUNCE;
    int   _leaderID = -1;
    bool  _isLeader = false;
    int   _announceTicks = 0;

    int  _localTick       = 0;
    int  _pendingForks    = 0;

    std::map<int, std::pair<int,int>> _agentLevels;
    bool      _transitionToPhase2 = false;
    AgentType _assignedRole       = AgentType::Worker;

    bool          _arrivedAtRally    = false;
    int           _settleTicks       = 0;
    int           _settleTarget      = 0;
    int           _settleDir         = -1;
    bool          _settleJustTurned  = false;
    bool          _settlePostForward = false;
    std::set<int> _settleArrivals;
    int           _ritualistSlotsLeft = 6;

    void _processBroadcasts(Agent &agent);
    void _doAnnounce(Agent &agent);
    void _doCollect(Agent &agent);
    void _doIncantate(Agent &agent);
    void _doFork(Agent &agent);
    void _doSettle(Agent &agent);
    void _settleNavigate(Agent &agent);
    AgentType _pickRole(int level);
    void _assignSingleRole(Agent &agent, int targetId);
    void _assignSelfAndFinalize(Agent &agent);

    void _broadcast(Agent &agent, int code, const std::string &payload = "");
    void _sendHeartbeat(Agent &agent);
    void _countFreshPeers(Agent &agent, int &total, int &atLvl2);
    bool _parseRoleAssignment(Agent &agent, const std::string &payload);
    void _updateLeader(Agent &agent, int senderId);
};
