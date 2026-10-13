#include "Ritualist.hpp"
#include "../../core/Agent.hpp"

void Ritualist::tick(Agent &agent)
{
    switch (_state) {
    case State::CALIBRATE: _doCalibrate(agent); break;
    case State::ARRIVED:   _doArrived(agent);   break;
    }
}
