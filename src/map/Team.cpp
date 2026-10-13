#include "Team.hpp"
#include "Drone.hpp"
#include "Vector.hpp"
#include "World.hpp"

#include <algorithm>
#include <memory>

Team::Team(const std::string& name, int slots, const Vector2i size, World& w): name(name), free_slots(0), world_ref(w)
{
    for (int i = 0; i < slots; i++) {
        Point pos = {rand() % size.x, rand() % size.y};
        add_egg(pos);
    }

    debug_logf("[TEAM] '{}' created", name);
}

Team::~Team()
{}

int Team::slots() const
{
    return free_slots;
}

void Team::add_egg(const Point& pos)
{
    eggs.push_back(std::make_unique<Point>(pos));
    free_slots++;
}

Drone* Team::createDrone()
{
    if (eggs.empty())
        return nullptr;

    int rand_egg = rand() % eggs.size();
    Point spawn_pos = *eggs[rand_egg];
    eggs.erase(eggs.begin() + rand_egg);

    auto drone = std::make_unique<Drone>(*this, spawn_pos);
    Drone* ptr = drone.get();
    drones.push_back(std::move(drone));

    world_ref.registerDrone(drones.back().get());

    free_slots--;
    return ptr;
}

bool Team::removeDrone(int id)
{
    auto it = std::find_if(drones.begin(), drones.end(),
        [id](const auto& d) { return d->id == id; });
    if (it == drones.end())
        return false;
    drones.erase(it);
    return true;
}

void Team::tickDrones()
{
    for (size_t i = 0; i < drones.size(); i++)
        if (!drones[i]->tick()) {
            world_ref.unregisterDrone(drones[i]->id);
            drones.erase(drones.begin() + i);
            i--;
        }
}
