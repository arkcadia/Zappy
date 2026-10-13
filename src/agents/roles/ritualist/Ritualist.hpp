#pragma once
#include "../ARole.hpp"

class Agent;

class Ritualist : public ARole {
public:
    void tick(Agent &agent) override;
private:
    enum class State { CALIBRATE, ARRIVED };
    State _state = State::CALIBRATE;
    int   _arrivedTick = 0;

    void _doCalibrate(Agent &agent);
    void _doArrived(Agent &agent);
    void _doIncantation(Agent &agent);
};
