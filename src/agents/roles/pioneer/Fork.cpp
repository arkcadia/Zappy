#include "Pioneer.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"

void Pioneer::_doFork(Agent &agent)
{
    _sendHeartbeat(agent);

    debug_logf("[Agent {}] Pioneer: forking a new egg (pending={})", agent.ID(), _pendingForks + 1);
    _sendAndRead(agent, "Fork");
    ++_pendingForks;
    _state = State::COLLECT;
}
