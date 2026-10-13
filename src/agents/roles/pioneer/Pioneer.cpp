#include "Pioneer.hpp"
#include "../worker/Worker.hpp"
#include "../bodyguard/Bodyguard.hpp"
#include "../ritualist/Ritualist.hpp"
#include "../chaos/Chaos.hpp"
#include "../leader/Leader.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"

static const char *roleName(AgentType t)
{
    switch (t) {
    case AgentType::Ritualist: return "Ritualist";
    case AgentType::Bodyguard: return "Bodyguard";
    case AgentType::Chaos:     return "Chaos";
    case AgentType::Leader:    return "Leader";
    default:                  return "Worker";
    }
}

void Pioneer::tick(Agent &agent)
{
    switch (_state) {
    case State::ANNOUNCE:  _doAnnounce(agent);  break;
    case State::COLLECT:   _doCollect(agent);   break;
    case State::INCANTATE: _doIncantate(agent); break;
    case State::FORK:      _doFork(agent);      break;
    case State::SETTLE:    _doSettle(agent);    break;
    }

    if (_transitionToPhase2) {
        debug_logf("[Agent {}] role transition: Pioneer -> {}", agent.ID(), roleName(_assignedRole));
        switch (_assignedRole) {
        case AgentType::Leader:    agent.setRole(std::make_unique<Leader>());    break;
        case AgentType::Ritualist: agent.setRole(std::make_unique<Ritualist>()); break;
        case AgentType::Bodyguard: agent.setRole(std::make_unique<Bodyguard>()); break;
        case AgentType::Chaos:     agent.setRole(std::make_unique<Chaos>());     break;
        default:                   agent.setRole(std::make_unique<Worker>());    break;
        }
    }
}
