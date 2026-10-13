#include "queue.hpp"
#include "../core/Agent.hpp"
#include "../../debug.hpp"
#include "pioneer/Pioneer.hpp"
#include "worker/Worker.hpp"
#include "bodyguard/Bodyguard.hpp"
#include "ritualist/Ritualist.hpp"
#include "chaos/Chaos.hpp"
#include <sstream>

bool Queue::_parseRole(Agent &agent, const std::string &payload)
{
    std::istringstream ss(payload);
    int n = 0;
    ss >> n;
    for (int i = 0; i < n; ++i) {
        int targetId = 0, roleType = 0;
        if (!(ss >> targetId >> roleType)) break;
        if (targetId == agent.ID()) {
            _assignedRole = static_cast<AgentType>(roleType);
            int post = -1;
            ss >> post;
            _assignedPost = post;
            return true;
        }
    }
    return false;
}

void Queue::tick(Agent &agent)
{
    if (!_querySent) {
        debug_logf("[Agent {}] Queue: broadcasting code-5 (phase query)", agent.ID());
        _enqueueBroadcast(5);
        _flushBroadcastBuf(agent);
        _querySent = true;
        return;
    }

    while (auto ev = agent.pollBroadcast()) {
        if (ev->msg.code == 7 && _parseRole(agent, ev->msg.payload)) {
            debug_logf("[Agent {}] Queue: role assigned by leader (code-7) -> Phase 2",
                       agent.ID());
            _toPhase2 = true;
            break;
        }
        if (ev->msg.code == 6) {
            _phase2Active = true;
        }
        if (!_phase2Active && (ev->msg.code == 0 || ev->msg.code == 1)) {
            debug_logf("[Agent {}] Queue: Phase-1 heartbeat (code {}), -> Pioneer",
                       agent.ID(), ev->msg.code);
            _toPhase1 = true;
            break;
        }
    }

    if (_toPhase1 || _toPhase2) {
    } else {
        ++_probeCount;
        if (_phase2Active) {
            if (_probeCount % 10 == 0) {
                debug_logf("[Agent {}] Queue: Phase 2 active, re-sending code-5 (probe {})",
                           agent.ID(), _probeCount);
                _enqueueBroadcast(5);
                _flushBroadcastBuf(agent);
            } else {
                _sendAndRead(agent, "Inventory");
            }
            if (_probeCount >= 400) {
                debug_logf("[Agent {}] Queue: Phase 2 timeout after {} probes -> Worker",
                           agent.ID(), _probeCount);
                _toPhase2 = true;
            } else {
                return;
            }
        } else {
            if (_probeCount >= 10) {
                debug_logf("[Agent {}] Queue: no leader response after {} probes -> Pioneer",
                           agent.ID(), _probeCount);
                _toPhase1 = true;
            } else {
                _sendAndRead(agent, "Inventory");
                return;
            }
        }
    }

    if (_toPhase1) {
        debug_logf("[Agent {}] Queue -> Pioneer", agent.ID());
        agent.setRole(std::make_unique<Pioneer>());
    } else {
        debug_logf("[Agent {}] Queue -> role {}", agent.ID(), static_cast<int>(_assignedRole));
        switch (_assignedRole) {
        case AgentType::Ritualist: agent.setRole(std::make_unique<Ritualist>()); break;
        case AgentType::Bodyguard: agent.setRole(std::make_unique<Bodyguard>(_assignedPost)); break;
        case AgentType::Chaos:     agent.setRole(std::make_unique<Chaos>());          break;
        default:                   agent.setRole(std::make_unique<Worker>());         break;
        }
    }
}
