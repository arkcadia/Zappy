#pragma once
#include "../state/Inventory.hpp"
#include <optional>
#include <string>
#include <unordered_map>

enum class AgentType {
    Worker,
    Bodyguard,
    Ritualist,
    Chaos,
    Leader,
};

struct BroadcastMsg {
    int         code;
    int         sender;
    int         msgCount;
    std::string payload;
};

struct BroadcastEvent {
    int          dir;
    BroadcastMsg msg;
};

struct ParentReplyMsg {
    int x;
    int y;
    int newbornId;
};

class broadcast {
public:
    broadcast() = default;

    void setAgentId(int id) { _agentId = id; }

    std::string encode(const std::string &msg) const;

    std::optional<BroadcastMsg> parse(const std::string &raw) const;

    ParentReplyMsg parseParentReply(const BroadcastMsg &msg);

private:
    int         _agentId  = 0;
    mutable int _msgCount = 0;
    mutable std::unordered_map<int, int> _knownAgents;
};
