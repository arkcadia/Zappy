#include "Pioneer.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"

void Pioneer::_doIncantate(Agent &agent)
{
    debug_logf("[Agent {}] Pioneer: starting incantation at lvl {}", agent.ID(), agent.level());
    std::string resp = _sendAndRead(agent, "Incantation");
    if (resp == "ko") {
        debug_logf("[Agent {}] Pioneer: incantation failed (ko), back to COLLECT", agent.ID());
        _state = State::COLLECT;
        return;
    }

    std::string lvlLine = (resp.compare(0, 13, "Current level") == 0)
        ? resp
        : _readNext(agent);

    int newLvl = agent.level() + 1;
    auto pos = lvlLine.rfind(' ');
    if (pos != std::string::npos) {
        try { newLvl = std::stoi(lvlLine.substr(pos + 1)); } catch (...) {}
    }
    agent.setLevel(newLvl);
    debug_logf("[Agent {}] Pioneer: incantation success, lvl {}", agent.ID(), newLvl);

    _sendHeartbeat(agent);

    _state = State::COLLECT;
    _processBroadcasts(agent);
}
