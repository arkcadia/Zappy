#pragma once

#include "Vector.hpp"
#include <memory>
#include <vector>
#include <string>

class Drone;
class World;

class Team {
public:
    Team(const std::string& name, int slots, const Vector2i size, World& world);
    ~Team();

    int slots() const;
    void add_egg(const Point& pos);

    Drone* createDrone();
    bool removeDrone(int id);

    void tickDrones();

private:
    const std::string name;

    int free_slots;

    std::vector<std::unique_ptr<Drone>> drones;
    std::vector<std::unique_ptr<Point>> eggs;

    World &world_ref;
};
