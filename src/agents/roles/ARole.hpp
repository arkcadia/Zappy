#pragma once

#include "../state/Inventory.hpp"
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class Agent;

class ARole {
public:
    ARole() = default;
    virtual ~ARole() = default;

    virtual void tick(Agent &agent) = 0;

    std::string _readNext(Agent &agent);
    std::string _sendAndRead(Agent &agent, const std::string &cmd);
    std::vector<std::vector<std::string>> _parseLook(const std::string &raw);
    void _parseInventory(Agent &agent, const std::string &raw);
    void _handleBroadcast(Agent &agent, const std::string &line);

    bool _navigateTo(Agent &agent, int tx, int ty);
    bool _beaconCalibrate(Agent &agent);
    void _refillFood(Agent &agent);

    static bool _findNearestFood(const std::vector<std::vector<std::string>> &tiles, int &lateral);
    static bool _findNearestPlayer(const std::vector<std::vector<std::string>> &tiles, int &lateral);

    void _moveForward(Agent &agent);
    void _turnLeft(Agent &agent);
    void _turnRight(Agent &agent);
    void _turnAround(Agent &agent);

    bool _stepTowardBeacon(Agent &agent, int dir, bool &justTurned);
    void _navigateAwayFrom(Agent &agent, int beaconDir);

    void _enqueueBroadcast(int code, const std::string &payload = "");
    bool _flushBroadcastBuf(Agent &agent);
    bool _hasPendingBroadcast() const { return !_broadcastBuf.empty(); }

protected:
    int  _calDir        = -1;
    bool _calJustTurned = false;
private:
    std::vector<std::pair<int, std::string>> _broadcastBuf;
};
