#pragma once
#include "../ARole.hpp"

class Agent;

class Worker : public ARole {
public:
    void tick(Agent &agent) override;
private:
    enum class State { CALIBRATE, SCATTER, GATHER, DELIVER };
    State _state = State::CALIBRATE;
    int   _scatterTurns = -1;

    void _doCalibrate(Agent &agent);
    void _doScatter(Agent &agent);
    void _doGather(Agent &agent);
    void _doDeliver(Agent &agent);
};
