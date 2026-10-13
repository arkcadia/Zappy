#include "Ritualist.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"

void Ritualist::_doCalibrate(Agent &agent)
{
    if (_sendAndRead(agent, "Take food") == "ok")
        agent.inventory().add(Resource::Food, 1);

    if (_beaconCalibrate(agent)) {
        debug_logf("[Agent {}] Ritualist: calibrated at (0,0)", agent.ID());

        _parseInventory(agent, _sendAndRead(agent, "Inventory"));
        int food = agent.inventory().get(Resource::Food);
        int keep = 5;
        while (food > keep) {
            if (_sendAndRead(agent, "Set food") != "ok")
                break;
            agent.inventory().add(Resource::Food, -1);
            --food;
        }
        debug_logf("[Agent {}] Ritualist: deposited food, keeping {} (had {})",
                   agent.ID(), keep, food);

        _state = State::ARRIVED;
    }
}
