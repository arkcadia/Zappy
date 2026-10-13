#include "Chaos.hpp"
#include "../../core/Agent.hpp"

void Chaos::tick(Agent &agent)
{
    switch (_state) {
    case State::CALIBRATE: _doCalibrate(agent); break;
    case State::DEPART:    _doDepart(agent);    break;
    case State::HUNT:      _doHunt(agent);      break;
    }
}
