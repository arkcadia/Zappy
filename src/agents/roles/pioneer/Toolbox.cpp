#include "Pioneer.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"
#include <sstream>

void Pioneer::_broadcast(Agent &agent, int code, const std::string &payload)
{
    verbose_logf("[Agent {}] Pioneer: sending broadcast code={} payload='{}'", agent.ID(), code, payload);
    std::string body = std::to_string(code) + " " + std::to_string(agent.ID());
    if (!payload.empty())
        body += " " + payload;
    std::string msg = agent.broadcastProtocol().encode(body);
    _sendAndRead(agent, "Broadcast " + msg);
}

void Pioneer::_sendHeartbeat(Agent &agent)
{
    _broadcast(agent, 1, std::to_string(agent.level()));
}

void Pioneer::_countFreshPeers(Agent &agent, int &total, int &atLvl2)
{
    total  = 1;
    atLvl2 = (agent.level() >= 2) ? 1 : 0;
    for (const auto &[id, p] : _agentLevels) {
        if (id == agent.ID()) continue;
        if (_localTick - p.second <= 100) {
            ++total;
            if (p.first >= 2) ++atLvl2;
        }
    }
}

bool Pioneer::_parseRoleAssignment(Agent &agent, const std::string &payload)
{
    std::istringstream ss(payload);
    int n = 0;
    ss >> n;
    bool found = false;
    for (int i = 0; i < n; ++i) {
        int targetId = 0, roleType = 0;
        if (!(ss >> targetId >> roleType)) break;
        if (targetId == agent.ID()) {
            _assignedRole = static_cast<AgentType>(roleType);
            found = true;
        }
    }
    return found;
}

void Pioneer::_updateLeader(Agent &agent, int senderId)
{
    int prevLeader = _leaderID;
    if (_leaderID < 0 || senderId < _leaderID)
        _leaderID = senderId;
    _isLeader = (agent.ID() <= _leaderID);
    if (_leaderID != prevLeader)
        debug_logf("[Agent {}] Pioneer: leader updated to {} (self isLeader={})",
                   agent.ID(), _leaderID, _isLeader);
}
