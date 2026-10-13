#include "ARole.hpp"
#include "../core/Agent.hpp"
#include "../../debug.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

static const char *RES_NAME[7] = {
    "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame",
};

std::string ARole::_readNext(Agent &agent)
{
    while (true) {
        std::string line = agent.conn().readLine();
        if (line == "dead") {
            debug_logf("[Agent {}] received 'dead', connection will close", agent.ID());
            throw std::runtime_error("dead");
        }
        if (line.size() >= 8 && line.compare(0, 8, "message ") == 0) {
            _handleBroadcast(agent, line);
            continue;
        }
        if (line.size() >= 6 && line.compare(0, 6, "eject ") == 0) {
            debug_logf("[Agent {}] ejected: {}", agent.ID(), line);
            continue;
        }
        return line;
    }
}

std::string ARole::_sendAndRead(Agent &agent, const std::string &cmd)
{
    verbose_logf("[Agent {}] cmd -> {}", agent.ID(), cmd);
    agent.conn().sendLine(cmd);
    std::string resp = _readNext(agent);
    verbose_logf("[Agent {}] cmd <- {}", agent.ID(), resp);
    return resp;
}

std::vector<std::vector<std::string>> ARole::_parseLook(const std::string &raw)
{
    std::vector<std::vector<std::string>> tiles;
    auto start = raw.find('[');
    auto end   = raw.rfind(']');
    if (start == std::string::npos || end == std::string::npos) {
        verbose_logf("[ARole] _parseLook: malformed response '{}'", raw);
        return tiles;
    }

    std::string inner = raw.substr(start + 1, end - start - 1);
    std::istringstream ss(inner);
    std::string cell;
    while (std::getline(ss, cell, ',')) {
        std::vector<std::string> items;
        std::istringstream cs(cell);
        std::string tok;
        while (cs >> tok)
            items.push_back(tok);
        tiles.push_back(items);
    }
    verbose_logf("[ARole] _parseLook: {} tile(s) parsed", tiles.size());
    return tiles;
}

void ARole::_parseInventory(Agent &agent, const std::string &raw)
{
    auto start = raw.find('[');
    auto end   = raw.rfind(']');
    if (start == std::string::npos || end == std::string::npos) {
        verbose_logf("[Agent {}] _parseInventory: malformed response '{}'", agent.ID(), raw);
        return;
    }
    std::string inner = raw.substr(start + 1, end - start - 1);
    std::istringstream ss(inner);
    std::string entry;
    while (std::getline(ss, entry, ',')) {
        std::istringstream es(entry);
        std::string name;
        int qty = 0;
        if (!(es >> name >> qty))
            continue;
        for (int i = 0; i < 7; ++i)
            if (name == RES_NAME[i])
                agent.inventory().set(static_cast<Resource>(i), qty);
    }
    Inventory &bag = agent.inventory();
    verbose_logf("[Agent {}] inventory: food={} linemate={} deraumere={} sibur={} mendiane={} phiras={} thystame={}",
                 agent.ID(),
                 bag.get(Resource::Food), bag.get(Resource::Linemate), bag.get(Resource::Deraumere),
                 bag.get(Resource::Sibur), bag.get(Resource::Mendiane), bag.get(Resource::Phiras),
                 bag.get(Resource::Thystame));
}

void ARole::_handleBroadcast(Agent &agent, const std::string &line)
{
    std::string rest = line.substr(8);
    auto comma = rest.find(',');
    int  dir   = 0;
    std::string text;
    if (comma != std::string::npos) {
        try { dir = std::stoi(rest.substr(0, comma)); } catch (...) {}
        text = (comma + 2 < rest.size()) ? rest.substr(comma + 2) : "";
    } else {
        text = rest;
    }
    verbose_logf("[Agent {}] broadcast notification: dir={} text='{}'", agent.ID(), dir, text);
    agent.feedBroadcast(dir, text);
}

void ARole::_moveForward(Agent &agent)
{
    _sendAndRead(agent, "Forward");
    agent.moveForward();
}

void ARole::_turnLeft(Agent &agent)
{
    _sendAndRead(agent, "Left");
    agent.turnLeft();
}

void ARole::_turnRight(Agent &agent)
{
    _sendAndRead(agent, "Right");
    agent.turnRight();
}

void ARole::_turnAround(Agent &agent)
{
    _turnRight(agent);
    _turnRight(agent);
}

bool ARole::_stepTowardBeacon(Agent &agent, int dir, bool &justTurned)
{
    if (justTurned) {
        _moveForward(agent);
        justTurned = false;
        return true;
    }

    switch (dir) {
    case 1: case 2: case 3:
        _moveForward(agent);
        return true;
    case 4: case 5:
        _turnRight(agent);
        justTurned = true;
        break;
    case 6:
        _turnAround(agent);
        justTurned = true;
        break;
    case 7: case 8:
        _turnLeft(agent);
        justTurned = true;
        break;
    }
    return false;
}

void ARole::_navigateAwayFrom(Agent &agent, int beaconDir)
{
    switch (beaconDir) {
    case 0:
        _moveForward(agent);
        break;
    case 1: case 2: case 3:
        _turnAround(agent);
        _moveForward(agent);
        break;
    case 4: case 5:
        _turnLeft(agent);
        _moveForward(agent);
        break;
    case 6:
        _moveForward(agent);
        break;
    case 7: case 8:
        _turnRight(agent);
        _moveForward(agent);
        break;
    }
}

static Dir _dirToward(const Agent &agent, int tx, int ty)
{
    int mx = agent.mapX();
    int my = agent.mapY();

    int raw_dx = (mx > 0) ? ((tx - agent.posX() + mx) % mx) : 0;
    int raw_dy = (my > 0) ? ((ty - agent.posY() + my) % my) : 0;

    int distX = (mx > 0) ? std::min(raw_dx, mx - raw_dx) : 0;
    int distY = (my > 0) ? std::min(raw_dy, my - raw_dy) : 0;

    if (distX >= distY && distX > 0)
        return (raw_dx * 2 <= mx) ? Dir::East : Dir::West;
    if (distY > 0)
        return (raw_dy * 2 <= my) ? Dir::South : Dir::North;
    return agent.dir();
}

bool ARole::_findNearestFood(const std::vector<std::vector<std::string>> &tiles, int &lateral)
{
    int bestDist = -1;
    for (std::size_t i = 1; i < tiles.size(); ++i) {
        bool hasFood = false;
        for (const auto &item : tiles[i])
            if (item == "food") { hasFood = true; break; }
        if (!hasFood) continue;

        int r    = static_cast<int>(std::sqrt(static_cast<double>(i)));
        int o    = static_cast<int>(i) - r * r;
        int lat  = o - r;
        int dist = r + std::abs(lat);
        if (bestDist < 0 || dist < bestDist) {
            bestDist = dist;
            lateral  = lat;
        }
    }
    return bestDist >= 0;
}

bool ARole::_findNearestPlayer(const std::vector<std::vector<std::string>> &tiles, int &lateral)
{
    int bestDist = -1;
    for (std::size_t i = 1; i < tiles.size(); ++i) {
        bool hasPlayer = false;
        for (const auto &item : tiles[i])
            if (item == "player") { hasPlayer = true; break; }
        if (!hasPlayer) continue;

        int r    = static_cast<int>(std::sqrt(static_cast<double>(i)));
        int o    = static_cast<int>(i) - r * r;
        int lat  = o - r;
        int dist = r + std::abs(lat);
        if (bestDist < 0 || dist < bestDist) {
            bestDist = dist;
            lateral  = lat;
        }
    }
    return bestDist >= 0;
}


void ARole::_enqueueBroadcast(int code, const std::string &payload)
{
    _broadcastBuf.push_back({code, payload});
}

bool ARole::_flushBroadcastBuf(Agent &agent)
{
    if (_broadcastBuf.empty())
        return false;

    if (_broadcastBuf.size() == 1) {
        auto &[code, payload] = _broadcastBuf[0];
        std::string body = std::to_string(code) + " " + std::to_string(agent.ID());
        if (!payload.empty()) body += " " + payload;
        verbose_logf("[Agent {}] broadcast flush (single code={})", agent.ID(), code);
        _sendAndRead(agent, "Broadcast " + agent.broadcastProtocol().encode(body));
    } else {
        std::string packed;
        for (auto &[code, payload] : _broadcastBuf) {
            if (!packed.empty()) packed += "|";
            packed += std::to_string(code);
            if (!payload.empty()) packed += " " + payload;
        }
        std::string body = "10 " + std::to_string(agent.ID()) + " " + packed;
        verbose_logf("[Agent {}] broadcast flush (packed {} msgs)", agent.ID(), _broadcastBuf.size());
        _sendAndRead(agent, "Broadcast " + agent.broadcastProtocol().encode(body));
    }

    _broadcastBuf.clear();
    return true;
}

bool ARole::_navigateTo(Agent &agent, int tx, int ty)
{
    if (agent.posX() == tx && agent.posY() == ty)
        return true;

    Dir target = _dirToward(agent, tx, ty);
    int diff = (static_cast<int>(target) - static_cast<int>(agent.dir()) + 4) % 4;
    if (diff == 1)      _turnRight(agent);
    else if (diff == 2) _turnAround(agent);
    else if (diff == 3) _turnLeft(agent);

    _moveForward(agent);
    bool arrived = agent.posX() == tx && agent.posY() == ty;
    if (arrived)
        debug_logf("[Agent {}] navigateTo: arrived at ({},{})", agent.ID(), tx, ty);
    return arrived;
}

bool ARole::_beaconCalibrate(Agent &agent)
{
    while (auto ev = agent.pollBroadcast()) {
        if (ev->msg.code == 6) {
            _calDir = ev->dir;
            if (_calDir == 0) {
                agent.setPos(0, 0);
                return true;
            }
        }
    }
    if (_calDir < 0) {
        _sendAndRead(agent, "Inventory");
        return false;
    }
    _stepTowardBeacon(agent, _calDir, _calJustTurned);
    return false;
}

void ARole::_refillFood(Agent &agent)
{
    _parseInventory(agent, _sendAndRead(agent, "Inventory"));
    if (agent.inventory().get(Resource::Food) < 5) {
        if (_sendAndRead(agent, "Take food") == "ok")
            agent.inventory().add(Resource::Food, 1);
    }
}
