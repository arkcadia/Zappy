#pragma once
class ARole;
#include "Broadcast.hpp"
#include "Connection.hpp"
#include "../state/Inventory.hpp"
#include <memory>
#include <optional>
#include <queue>
#include <string>

enum class Dir { North = 0, East = 1, South = 2, West = 3 };

class Agent {
public:
    Agent(const std::string &host, int port, const std::string &teamName, bool solo = false);
    ~Agent();

    void run();

    Connection &conn() { return _connection; }
    Inventory  &inventory() { return _inventory; }
    broadcast  &broadcastProtocol() { return _broadcastProtocol; }

    int  level() const { return _lvl; }
    int  posX()  const { return _posX; }
    int  posY()  const { return _posY; }
    int  mapX()  const { return _mapX; }
    int  mapY()  const { return _mapY; }
    int  ID()         const { return _ID; }
    int  clientsNb()  const { return _clientsNb; }
    Dir  dir()        const { return _dir; }

    void setID(int id)          { _ID = id; _broadcastProtocol.setAgentId(id); }
    void setPos(int x, int y)   { _posX = x; _posY = y; }
    void setLevel(int lvl)      { _lvl = lvl; }
    void setMap(int x, int y)   { _mapX = x; _mapY = y; }

    void moveForward();
    void turnLeft();
    void turnRight();

    const std::string &teamName() const { return _teamName; }
    bool  solo() const { return _solo; }

    void setRole(std::unique_ptr<ARole> role);

    void feedBroadcast(int dir, const std::string &raw);

    std::optional<BroadcastEvent> pollBroadcast();
    std::optional<std::string>    pollEnemyBroadcast();
    std::optional<std::pair<int, std::string>> pollEnemyBroadcastDir();

private:
    void _applyBroadcast(BroadcastEvent &ev);
    void _unpackBroadcast(int dir, const BroadcastMsg &envelope);

    std::unique_ptr<ARole>       _role;
    broadcast                    _broadcastProtocol;
    Connection                   _connection;
    Inventory                    _inventory;
    std::queue<BroadcastEvent>   _broadcastQueue;
    std::queue<std::string>                _enemyBroadcasts;
    std::queue<std::pair<int, std::string>> _enemyBroadcastsDir;
    std::string                  _teamName;
    bool                         _solo;
    int _posX;
    int _posY;
    int _lvl;
    int _mapX;
    int _mapY;
    int _ID;
    int _clientsNb = 0;
    Dir _dir = Dir::North;
};
