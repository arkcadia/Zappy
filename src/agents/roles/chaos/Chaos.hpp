#pragma once
#include "../ARole.hpp"

class Agent;

class Chaos : public ARole {
public:
    void tick(Agent &agent) override;
private:
    enum class State { CALIBRATE, DEPART, HUNT };
    State _state       = State::CALIBRATE;
    int   _enemyDir    = -1;
    bool  _justTurned  = false;

    void _doCalibrate(Agent &agent);
    void _doDepart(Agent &agent);
    void _doHunt(Agent &agent);
    bool _nearOwnBase(const Agent &agent) const;
};
