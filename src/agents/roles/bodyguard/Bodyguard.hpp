#pragma once
#include "../ARole.hpp"
#include "../../core/Agent.hpp"
#include <utility>

class Agent;

class Bodyguard : public ARole {
public:
    Bodyguard(int post = 0) : _claimedPos(post) {}
    void tick(Agent &agent) override;

private:
    enum class State { NAVIGATE, GUARD };
    State _state      = State::NAVIGATE;
    int   _claimedPos = -1;
    bool  _calibrated = false;

    std::pair<int,int> _postCoords(const Agent &agent) const;
    Dir                _outwardDir()                   const;

    void _doNavigate(Agent &agent);
    void _doGuard   (Agent &agent);
};
