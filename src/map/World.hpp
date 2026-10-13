#pragma once

#include "Drone.hpp"
#include "Vector.hpp"
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

enum class ResourceType {
    FOOD = 0,
    LINEMATE = 1,
    DERAUMERE = 2,
    SIBUR = 3,
    MENDIANE = 4,
    PHIRAS = 5,
    THYSTAME = 6,
};

template <>
struct std::formatter<ResourceType> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const ResourceType& v, std::format_context& ctx) const
    {
        string_view name;

        switch (v) {
            case ResourceType::FOOD:      name = "Food";      break;
            case ResourceType::LINEMATE:  name = "Linemate";  break;
            case ResourceType::DERAUMERE: name = "Deraumere"; break;
            case ResourceType::SIBUR:     name = "Sibur";     break;
            case ResourceType::MENDIANE:  name = "Mendiane";  break;
            case ResourceType::PHIRAS:    name = "Phiras";    break;
            case ResourceType::THYSTAME:  name = "Thystame";  break;
            default:                      name = "Unknown";   break;
        }
        return std::format_to(ctx.out(), "{}", name);
    }
};

inline static constexpr float density[TOTAL_RESOURCES] = {
    0.5f,
    0.3f,
    0.15f,
    0.1f,
    0.1f,
    0.08f,
    0.05f,
};

class Tile {
public:
    Tile();
    ~Tile();

    std::array<int, TOTAL_RESOURCES> getResources() const;

    int& operator[](const ResourceType i);
    void operator+=(const std::array<int, TOTAL_RESOURCES> more);
    bool operator-=(const std::array<int, TOTAL_RESOURCES> more);

    void addDrone(int id);
    void removeDrone(int id);
    void clearDrones();
    int getDroneCount() const;
    const std::set<int>& getDroneIds() const { return drones_id; }

    /// Tracks whether this tile's resources changed since the last GUI sync,
    /// so the server only needs to (re-)send tiles that actually changed
    /// instead of the whole map every tick.
    bool isDirty() const { return dirty; }
    void markDirty() { dirty = true; }
    void clearDirty() { dirty = false; }

private:
    std::array<int, TOTAL_RESOURCES> resources;
    std::set<int> drones_id;
    bool dirty = true;
};

class World {
public:
    World(const Vector2i &size, int frequence);
    ~World();

    Tile& operator[](const Point pos);
    const Tile& operator[](const Point pos) const;

    Vector2i getSize() const;

    void spawnResources();
    void spawnResource(const ResourceType resource, int quantity);

    void createTeam(const std::string& name, int slots);
    Team* getTeam(const std::string& name) const;
    std::vector<std::string> getTeamNames() const;

    Drone* getDrone(int drone_id) const;
    int getDroneCount() const;
    const std::unordered_map<int, Drone*>& getDrones() const;

    void tick();
    int getTickCount() const { return tick_count; }

    void registerDrone(Drone *d);
    bool unregisterDrone(int id);
    void killDrone(int id);

    std::vector<int> getAndClearRecentlyDied();

private:
    const Vector2i size;
    const int frequence;

    int tick_count = 0;

    std::vector<std::vector<Tile>> tiles;
    std::unordered_map<std::string, std::unique_ptr<Team>> teams;

    std::unordered_map<int, Drone*> drone_index;
    std::vector<int> recently_died;
};
