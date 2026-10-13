#pragma once

#include <array>
#include <functional>
#include <string>
#include <vector>

#include "Client.hpp"
#include "../map/World.hpp"

class World;
class Drone;

inline int ai_cmd_cost(const std::string& cmd)
{
    if (cmd == "Inventory")   return 1;
    if (cmd == "Fork")        return 42;
    if (cmd == "Incantation") return 300;
    return 7;
}

inline std::string resource_name(int idx)
{
    switch (idx) {
        case 0: return "food";
        case 1: return "linemate";
        case 2: return "deraumere";
        case 3: return "sibur";
        case 4: return "mendiane";
        case 5: return "phiras";
        case 6: return "thystame";
        default: return "";
    }
}

inline std::string build_look_response(const std::vector<Tile*>& tiles)
{
    std::string resp = "[";
    for (std::size_t i = 0; i < tiles.size(); i++) {
        if (i > 0) resp += ", ";
        const auto& res = tiles[i]->getResources();
        std::string cell;

        for (int d = 0; d < tiles[i]->getDroneCount(); d++)
            cell += "player ";

        for (int r = 0; r < TOTAL_RESOURCES; r++) {
            for (int q = 0; q < res[r]; q++) {
                if (!cell.empty()) cell += " ";
                cell += resource_name(r);
            }
        }
        resp += cell;
    }
    resp += "]";
    return resp;
}

inline std::string build_inventory_response(const std::array<int, TOTAL_RESOURCES>& inv)
{
    std::string r = "[";
    const char* names[] = {"food","linemate","deraumere","sibur","mendiane","phiras","thystame"};
    for (int i = 0; i < TOTAL_RESOURCES; i++) {
        if (i > 0) r += ", ";
        r += names[i];
        r += " ";
        r += std::to_string(inv[i]);
    }
    r += "]";
    return r;
}

inline int resource_index(const std::string& name)
{
    if (name == "food")
        return 0;
    if (name == "linemate")  return 1;
    if (name == "deraumere") return 2;
    if (name == "sibur")     return 3;
    if (name == "mendiane")  return 4;
    if (name == "phiras")    return 5;
    if (name == "thystame")  return 6;
    return -1;
}

class AiProtocol {
public:
    std::function<void(int, const std::string&)> broadcast;

    AiProtocol(
        std::function<void(int, const std::string&)> broadcast_fn,
        std::function<void(Client&)> fork_fn)
        : broadcast(std::move(broadcast_fn)), fork_cb(std::move(fork_fn)) {}

    std::string execute(
        Client&      client,
        const std::string& cmd,
        World&       world,
        Drone*       drone)
    {
        if (cmd == "Forward") {
            drone->move(world.getSize());
            return "ok";
        }
        if (cmd == "Right") {
            drone->rotateR();
            return "ok";
        }
        if (cmd == "Left") {
            drone->rotateL();
            return "ok";
        }
        if (cmd == "Look") {
            auto tiles = drone->look(world);
            return build_look_response(tiles);
        }
        if (cmd == "Inventory") {
            return build_inventory_response(drone->inventory());
        }
        if (cmd.rfind("Broadcast ", 0) == 0) {
            std::string text = cmd.substr(10);
            if (text.size() > 1024)
                text.resize(1024);
            broadcast(client.fd, text);
            return "ok";
        }
        if (cmd == "Connect_nbr") {
            return std::to_string(drone->unusedSlots());
        }
        if (cmd == "Fork") {
            drone->drone_fork();
            fork_cb(client);
            return "ok";
        }
        if (cmd == "Eject") {
            drone->eject(world);
            return "ok";
        }
        if (cmd.rfind("Take ", 0) == 0) {
            std::string obj = cmd.substr(5);
            int idx = resource_index(obj);
            if (idx < 0) return "ko";
            std::array<int, TOTAL_RESOURCES> take{};
            take[idx] = 1;
            return drone->take(take, world) ? "ok" : "ko";
        }
        if (cmd.rfind("Set ", 0) == 0) {
            std::string obj = cmd.substr(4);
            int idx = resource_index(obj);
            if (idx < 0) return "ko";
            std::array<int, TOTAL_RESOURCES> set{};
            set[idx] = 1;
            drone->set(set, world);
            return "ok";
        }
        if (cmd == "Incantation")
            return "ko";
        return "ko";   // unknown command
    }

private:
    std::function<void(Client&)> fork_cb;
};
