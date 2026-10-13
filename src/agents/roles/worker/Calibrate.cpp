#include "Worker.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"
#include <cstdlib>

void Worker::_doCalibrate(Agent &agent)
{
    if (_beaconCalibrate(agent)) {
        _scatterTurns = rand() % 4;
        int scatterSteps = 2 + rand() % 4;
        debug_logf("[Agent {}] Worker: calibrated, scatter turns={} steps={}",
                   agent.ID(), _scatterTurns, scatterSteps);
        _state = State::SCATTER;
    }
}

void Worker::_doScatter(Agent &agent)
{
    while (agent.pollBroadcast()) {}

    if (_scatterTurns > 0) {
        _turnRight(agent);
        --_scatterTurns;
        return;
    }

    for (int i = 0; i < 2 + rand() % 3; ++i)
        _moveForward(agent);

    debug_logf("[Agent {}] Worker: scattered, switching to GATHER", agent.ID());
    _state = State::GATHER;
}
