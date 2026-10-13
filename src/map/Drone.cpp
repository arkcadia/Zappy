#include "Drone.hpp"
#include "World.hpp"

#include <array>
#include <cassert>
#include <cstdlib>

Direction operator++(Direction& d, int)
{
    if (d == Direction::NONE) return d;

    d = static_cast<Direction>((static_cast<int>(d) % 8) + 1);
    return d;
}

Direction operator--(Direction& d, int)
{
    if (d == Direction::NONE) return d;

    int v = static_cast<int>(d);
    d = static_cast<Direction>((v - 2 + 8) % 8 + 1);
    return d;
}

std::atomic<int> Drone::next_id = 0;

Drone::Drone(Team& team, Point& spawn): id(next_id++), pos(spawn), team(team)
{
    dir = Direction::NORTH;

    survive_time = 1260;

    debug_logf("Drone {}: Spawned at {} towards {}", id, pos, dir);
}

Drone::~Drone()
{
    debug_logf("Drone {}: Destroyed", id);
}

void Drone::move(Point map_size)
{
    if (dir == Direction::NONE) return;

    pos = (pos + dir_vec[static_cast<int>(dir)]) % map_size;

    debug_logf("Drone {}: Moving to {}", id, pos);
}

void Drone::rotateR()
{
    if (dir == Direction::NONE) return;

    // two 45° CW rotations
    dir++;
    dir++;

    debug_logf("Drone {}: Rotating R to {}", id, dir);
}

void Drone::rotateL()
{
    if (dir == Direction::NONE) return;

    // two 45° CCW rotations
    dir--;
    dir--;

    debug_logf("Drone {}: Rotating L to {}", id, dir);
}

std::vector<Tile *> Drone::look(World& world) const
{
    // total cells in vision
    const int total_cells = level * level + level * 2 + 1;

    debug_logf("Drone {}: Looking at {} tiles towards {}", id, total_cells, dir);

    std::vector<Tile *> tiles;

    // for each line, how much to increase
    // go from line with tiles 1 to 3 to line with tiles 4 to 8
    Point line_vector = dir_vec[static_cast<int>(dir)];

    // for each line, how much to move for each tile
    // go from tile 1 to 2
    Direction line_dir = dir;
    line_dir++;
    line_dir++;
    Point column_vector = dir_vec[static_cast<int>(line_dir)];

    // at which tile is the drone actually looking at
    Point looking_at = pos;

    for (int i = 0; i < level + 1; i++) {
        const int line_cells = i * 2 + 1;
        verbose_logf("Drone {}: Cells at line {}: {}", id, i, line_cells);

        looking_at = pos + line_vector * i - column_vector * i;

        for (int j = 0; j < line_cells; j++) {
            verbose_logf("Drone {}: Looking at tile {} ({}/{})", id, looking_at, tiles.size() + 1, total_cells);

            tiles.push_back(&world[looking_at]);

            looking_at += column_vector;
        }
    }

    // TODO: add verbose print of resources in vision

    return tiles;
}

std::array<int, TOTAL_RESOURCES> Drone::inventory() const
{
    debug_logf("Drone {}: Inventory", id);

    return resources;
}

void Drone::broadcast() const
{
    debug_logf("Drone {}: Broadcast", id);
}

int Drone::unusedSlots() const
{
    return team.slots();
}

Egg Drone::drone_fork()
{
    debug_logf("Drone {}: Fork", id);

    team.add_egg(pos);

    egg_counter++;

    return {pos, id * 256 + egg_counter};
}

std::vector<int> Drone::eject(World& world)
{
    debug_logf("Drone {}: Eject", id);

    std::vector<int> ejected;
    Point move_vec = dir_vec[static_cast<int>(dir)];
    Point map_size = world.getSize();

    auto ids = world[pos].getDroneIds();
    for (int did : ids) {
        if (did == id) continue;
        Drone* d = world.getDrone(did);
        if (!d) continue;
        d->pos = (d->pos + move_vec) % map_size;
        ejected.push_back(did);
        debug_logf("Drone {}: ejected drone {} to {}", id, did, d->pos);
    }
    return ejected;
}

bool Drone::take(std::array<int, TOTAL_RESOURCES> objects, World& world)
{
    debug_logf("Drone {}: Take", id);
    verbose_logf("Drone {}: Wants {} {} {} {} {} {} {}", id, objects[0], objects[1], objects[2], objects[3], objects[4], objects[5], objects[6]);
    verbose_logf("Drone {}: Avail {} {} {} {} {} {} {}", id,
        world[pos].getResources()[0],
        world[pos].getResources()[1],
        world[pos].getResources()[2],
        world[pos].getResources()[3],
        world[pos].getResources()[4],
        world[pos].getResources()[5],
        world[pos].getResources()[6]);

    bool possible = world[pos] -= objects;

    if (!possible)
        return false;

    for (int i = 0; i < TOTAL_RESOURCES; i++)
        resources[i] += objects[i];

    return true;
}

bool Drone::set(std::array<int, TOTAL_RESOURCES> objects, World &world)
{
    debug_logf("Drone {}: Set", id);

    for (int i = 0; i < TOTAL_RESOURCES; i++)
        if (resources[i] < objects[i])
            return false;

    for (int i = 0; i < TOTAL_RESOURCES; i++)
        resources[i] -= objects[i];

    world[pos] += objects;
    return true;
}

bool Drone::canIncantate(const World& world) const
{
    if (level < 1 || level >= 8)
        return false;

    const Tile& tile = world[pos];
    int required = players_required[level - 1];

    int sameLevelCount = 0;
    for (int did : tile.getDroneIds()) {
        Drone* d = world.getDrone(did);
        if (d && d->level == level)
            ++sameLevelCount;
    }
    if (sameLevelCount < required)
        return false;

    const auto& cost = level_up_table[level - 1];
    const auto available = tile.getResources();
    for (int i = 0; i < TOTAL_RESOURCES; i++)
        if (cost[i] > available[i])
            return false;

    return true;
}

int Drone::startIncantation(World& world, const std::vector<int>& incanting_ids)
{
    debug_logf("Drone {}: Incantation resolve", id);

    if (level < 1 || level >= 8)
        return 0;

    int required = players_required[level - 1];
    int count = 0;
    for (int did : incanting_ids) {
        Drone* d = world.getDrone(did);
        if (d && d->getPos().x == pos.x && d->getPos().y == pos.y && d->level == level)
            ++count;
    }
    if (count < required)
        return 0;

    const auto& cost = level_up_table[level - 1];
    const auto available = world[pos].getResources();
    for (int i = 0; i < TOTAL_RESOURCES; i++)
        if (cost[i] > available[i])
            return 0;

    int oldLevel = level;
    std::array<int, TOTAL_RESOURCES> totalCost{};
    for (int i = 0; i < TOTAL_RESOURCES; i++)
        totalCost[i] = cost[i];
    world[pos] -= totalCost;

    for (int did : incanting_ids) {
        Drone* d = world.getDrone(did);
        if (d && d->getPos().x == pos.x && d->getPos().y == pos.y && d->level == oldLevel) {
            d->level = oldLevel + 1;
            debug_logf("Drone {}: leveled up to {} via incantation", did, d->level);
        }
    }

    return oldLevel + 1;
}

Point Drone::getPos() const
{
    return pos;
}

Direction Drone::getDir() const
{
    return dir;
}

int Drone::getLevel() const
{
    return level;
}

bool Drone::tick()
{
    survive_time--;

    if (survive_time > 0)
        return true;

    if (resources[0] == 0)
        return false;

    survive_time += FOOD_VALUE;
    resources[0]--;

    return true;
}
