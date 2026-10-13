#include "Worker.hpp"
#include "../../core/Agent.hpp"

void Worker::tick(Agent &agent)
{
    switch (_state) {
    case State::CALIBRATE: _doCalibrate(agent); break;
    case State::SCATTER:   _doScatter(agent);   break;
    case State::GATHER:    _doGather(agent);    break;
    case State::DELIVER:   _doDeliver(agent);   break;
    }
}
