#include "Worker.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"

static const char *RES[] = {
    "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame",
};

void Worker::_doDeliver(Agent &agent)
{
    while (agent.pollBroadcast()) {}

    if (!_navigateTo(agent, 0, 0))
        return;

    int totalDropped = 0;
    for (int i = 0; i < 7; ++i) {
        Resource r = static_cast<Resource>(i);
        int count = agent.inventory().get(r);
        for (int j = 0; j < count; ++j) {
            if (_sendAndRead(agent, std::string("Set ") + RES[i]) == "ok") {
                agent.inventory().add(r, -1);
                ++totalDropped;
            }
        }
    }
    debug_logf("[Agent {}] Worker: delivered {} items at (0,0)", agent.ID(), totalDropped);

    _sendAndRead(agent, "Fork");
    _moveForward(agent);
    _state = State::GATHER;
}
