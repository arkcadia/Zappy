#pragma once
#include "ARole.hpp"
#include "../core/Broadcast.hpp"
#include <string>

class Agent;

class Queue : public ARole {
public:
    void tick(Agent &agent) override;

private:
    int       _probeCount   = 0;
    bool      _querySent    = false;
    bool      _phase2Active = false;
    AgentType _assignedRole = AgentType::Worker;
    int       _assignedPost = -1;
    bool      _toPhase1     = false;
    bool      _toPhase2     = false;

    bool _parseRole(Agent &agent, const std::string &payload);
};
