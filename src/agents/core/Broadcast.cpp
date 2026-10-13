#include "Broadcast.hpp"
#include "../../debug.hpp"
#include <sstream>

std::string broadcast::encode(const std::string &msg) const
{
    std::istringstream ss(msg);
    int code = 0, id = 0;
    ss >> code >> id;

    std::string rest;
    std::getline(ss, rest);
    if (!rest.empty() && rest.front() == ' ')
        rest.erase(0, 1);

    std::string full = std::to_string(code) + " " + std::to_string(id) +
                       " " + std::to_string(_msgCount);
    if (!rest.empty())
        full += " " + rest;

    verbose_logf("[Broadcast {}] encode: code={} seq={} -> '{}'", id, code, _msgCount, full);
    ++_msgCount;
    return full;
}

std::optional<BroadcastMsg> broadcast::parse(const std::string &raw) const
{
    const std::string &plain = raw;
    std::istringstream ss(plain);
    BroadcastMsg msg{0, 0, 0, ""};

    if (!(ss >> msg.code >> msg.sender >> msg.msgCount)) {
        verbose_logf("[Broadcast] parse failed, malformed message: '{}'", raw);
        return std::nullopt;
    }

    auto it = _knownAgents.find(msg.sender);
    if (it != _knownAgents.end() && msg.msgCount <= it->second) {
        verbose_logf("[Broadcast] dropping stale/duplicate message from {} (seq {} <= last seen {})",
                     msg.sender, msg.msgCount, it->second);
        return std::nullopt;  // duplicate or stale (already-seen) message
    }
    _knownAgents[msg.sender] = msg.msgCount;

    std::getline(ss, msg.payload);
    if (!msg.payload.empty() && msg.payload.front() == ' ')
        msg.payload.erase(0, 1);

    verbose_logf("[Broadcast] parsed: code={} sender={} seq={} payload='{}'",
                 msg.code, msg.sender, msg.msgCount, msg.payload);
    return msg;
}

ParentReplyMsg broadcast::parseParentReply(const BroadcastMsg &msg)
{
    std::istringstream ss(msg.payload);
    ParentReplyMsg out{0, 0, 0};
    ss >> out.x >> out.y >> out.newbornId;

    int n = 0;
    ss >> n;
    for (int i = 0; i < n; ++i) {
        int id = 0, count = 0;
        ss >> id >> count;
        _knownAgents[id] = count;
    }
    ss >> _agentId;
    debug_logf("[Broadcast] parent reply: egg=({},{}) newborn={} ({} known agents merged)",
               out.x, out.y, out.newbornId, n);
    return out;
}

