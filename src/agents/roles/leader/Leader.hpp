#pragma once
#include "../ARole.hpp"
#include "../../core/Broadcast.hpp"
#include <algorithm>
#include <vector>

class Agent;

class Leader : public ARole {
public:
    void tick(Agent &agent) override;
private:
    enum class State { CALIBRATE, MANAGE };
    State _state = State::CALIBRATE;
    int   _arrivedTick = 0;
    int   _totalAssigned = 0;
    int   _bodyguardIndex = 0;
    bool  _hadLatecomer = false;
    std::vector<int> _assignedAgents;

    void _doCalibrate(Agent &agent);
    void _doManage(Agent &agent);
    void _assignRoles(Agent &agent);
    void _checkIncantation(Agent &agent);
    AgentType _pickNewRole(bool solo);
};
