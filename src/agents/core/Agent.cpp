#include "Agent.hpp"
#include "../../debug.hpp"
#include "../roles/ARole.hpp"
#include "../roles/queue.hpp"
#include <sstream>
#include <stdexcept>
#include <unistd.h>

Agent::Agent(const std::string &host, int port, const std::string &teamName, bool solo)
    : _connection(host, port), _teamName(teamName), _solo(solo),
      _posX(0), _posY(0), _lvl(1),
      _mapX(0), _mapY(0), _ID(0)
{
    _connection.connect();

    std::string welcome = _connection.readLine();
    if (welcome != "WELCOME")
        throw std::runtime_error("expected WELCOME, got: " + welcome);

    debug_logf("[Agent] handshake: WELCOME received, joining team '{}'", teamName);
    _connection.sendLine(_teamName);

    std::string slotLine = _connection.readLine();
    if (slotLine == "ko") {
        debug_logf("[Agent] handshake: no slot available for team '{}', exiting", teamName);
        std::exit(0);  // no slot available (expected during fork-egg polling)
    }
    {
        int nb = 0;
        try { nb = std::stoi(slotLine); } catch (...) {}
        _clientsNb = nb;
        setID(static_cast<int>(getpid()));
        debug_logf("[Agent {}] handshake: {} unused slot(s) left for team '{}'", _ID, nb, teamName);
    }

    std::string mapLine = _connection.readLine();
    int w = 0, h = 0;
    std::istringstream(mapLine) >> w >> h;
    setMap(w, h);
    debug_logf("[Agent {}] handshake complete: map {}x{}", _ID, w, h);

    debug_logf("[Agent {}] role -> Queue (phase detection)", _ID);
    setRole(std::make_unique<Queue>());
}

Agent::~Agent() {}

static constexpr int DX[4] = { 0, 1,  0, -1};
static constexpr int DY[4] = {-1, 0,  1,  0};

void Agent::moveForward()
{
    int d = static_cast<int>(_dir);
    if (_mapX > 0) _posX = ((_posX + DX[d]) % _mapX + _mapX) % _mapX;
    if (_mapY > 0) _posY = ((_posY + DY[d]) % _mapY + _mapY) % _mapY;
    verbose_logf("[Agent {}] moved forward -> pos=({},{})", _ID, _posX, _posY);
}

void Agent::turnLeft()
{
    _dir = static_cast<Dir>((static_cast<int>(_dir) + 3) % 4);
    verbose_logf("[Agent {}] turned left -> dir={}", _ID, static_cast<int>(_dir));
}

void Agent::turnRight()
{
    _dir = static_cast<Dir>((static_cast<int>(_dir) + 1) % 4);
    verbose_logf("[Agent {}] turned right -> dir={}", _ID, static_cast<int>(_dir));
}

void Agent::setRole(std::unique_ptr<ARole> role) { _role = std::move(role); }

void Agent::run()
{
    debug_logf("[Agent {}] starting run loop", _ID);
    while (_role)
        _role->tick(*this);
    debug_logf("[Agent {}] run loop ended", _ID);
}

void Agent::feedBroadcast(int dir, const std::string &raw)
{
    auto parsed = _broadcastProtocol.parse(raw);
    if (!parsed) {
        if (_enemyBroadcasts.size() < 8) {
            _enemyBroadcasts.push(raw);
            _enemyBroadcastsDir.push({dir, raw});
        }
        verbose_logf("[Agent {}] broadcast dropped (unparseable or stale): '{}'", _ID, raw);
        return;
    }

    if (parsed->code == 10) {
        _unpackBroadcast(dir, *parsed);
        return;
    }

    verbose_logf("[Agent {}] broadcast decoded: dir={} code={} sender={} payload='{}'",
                 _ID, dir, parsed->code, parsed->sender, parsed->payload);

    BroadcastEvent ev{dir, *parsed};
    _applyBroadcast(ev);
    _broadcastQueue.push(ev);
}

void Agent::_unpackBroadcast(int dir, const BroadcastMsg &envelope)
{
    std::istringstream ss(envelope.payload);
    std::string sub;
    while (std::getline(ss, sub, '|')) {
        if (sub.empty()) continue;
        std::istringstream subss(sub);
        int subcode = 0;
        if (!(subss >> subcode)) continue;
        std::string subPayload;
        std::getline(subss, subPayload);
        if (!subPayload.empty() && subPayload.front() == ' ')
            subPayload.erase(0, 1);

        BroadcastMsg msg{subcode, envelope.sender, envelope.msgCount, subPayload};
        verbose_logf("[Agent {}] unpacked broadcast: dir={} code={} sender={} payload='{}'",
                     _ID, dir, subcode, envelope.sender, subPayload);
        BroadcastEvent ev{dir, msg};
        _applyBroadcast(ev);
        _broadcastQueue.push(ev);
    }
}

std::optional<BroadcastEvent> Agent::pollBroadcast()
{
    if (_broadcastQueue.empty())
        return std::nullopt;
    BroadcastEvent ev = _broadcastQueue.front();
    _broadcastQueue.pop();
    return ev;
}

std::optional<std::string> Agent::pollEnemyBroadcast()
{
    if (_enemyBroadcasts.empty())
        return std::nullopt;
    std::string msg = _enemyBroadcasts.front();
    _enemyBroadcasts.pop();
    return msg;
}

std::optional<std::pair<int, std::string>> Agent::pollEnemyBroadcastDir()
{
    if (_enemyBroadcastsDir.empty())
        return std::nullopt;
    auto ev = _enemyBroadcastsDir.front();
    _enemyBroadcastsDir.pop();
    return ev;
}

void Agent::_applyBroadcast(BroadcastEvent &ev)
{
    if (ev.msg.sender == _ID)
        return;

    if (ev.msg.code == 3 && ev.dir == 0) {
        ParentReplyMsg pr = _broadcastProtocol.parseParentReply(ev.msg);
        if (pr.newbornId == _ID) {
            debug_logf("[Agent {}] parent reply matched: spawning at ({},{})", _ID, pr.x, pr.y);
            setPos(pr.x, pr.y);
        }
        return;
    }

}
