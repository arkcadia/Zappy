#include "World.hpp"
#include "Drone.hpp"
#include "Vector.hpp"
#include <array>
#include <memory>
#include <vector>

Tile::Tile(): resources({0})
{}

Tile::~Tile()
{}

std::array<int, TOTAL_RESOURCES> Tile::getResources() const
{
    return resources;
}

int& Tile::operator[](ResourceType i)
{
    return resources[static_cast<int>(i)];
}

void Tile::operator+=(const std::array<int, TOTAL_RESOURCES> more)
{
    for (int i = 0; i < TOTAL_RESOURCES; i++)
        resources[i] += more[i];
    dirty = true;
}

bool Tile::operator-=(const std::array<int, TOTAL_RESOURCES> more)
{
    for (int i = 0; i < TOTAL_RESOURCES; i++)
        if (resources[i] < more[i])
            return false;

    for (int i = 0; i < TOTAL_RESOURCES; i++)
        resources[i] -= more[i];
    dirty = true;

    return true;
}

void Tile::addDrone(int id)
{
    drones_id.insert(id);
}

void Tile::removeDrone(int id)
{
    drones_id.erase(id);
}

void Tile::clearDrones()
{
    drones_id.clear();
}

int Tile::getDroneCount() const
{
    return drones_id.size();
}

World::World(const Vector2i &size, int frequence): size(size), frequence(frequence)
{
    srand(time(NULL));

    tiles.resize(size.x);
    for (auto& sub:tiles)
        sub.resize(size.y);

    debug_logf("[World] Creating map with size {}", size);

    spawnResources();
}

World::~World()
{}

Tile& World::operator[](const Point pos)
{
    return tiles[((pos.x % size.x) + size.x) % size.x][((pos.y % size.y) + size.y) % size.y];
}

const Tile& World::operator[](const Point pos) const
{
    return tiles[((pos.x % size.x) + size.x) % size.x][((pos.y % size.y) + size.y) % size.y];
}

Vector2i World::getSize() const
{
    return size;
}

void World::spawnResource(ResourceType resource, int quantity)
{
    int resource_count = size.x * size.y * density[static_cast<int>(resource)];

    verbose_logf("[World] Resource count for {}: {}", resource, resource_count);

    if (quantity >= resource_count)
        return;

    for (int i = quantity; i < resource_count; i++) {
        int x = rand() % size.x, y = rand() % size.y;

        tiles[x][y][resource]++;
        tiles[x][y].markDirty();

        verbose_logf("[World] Spawned {} at {}", resource, Point(x, y));
    }
}

void World::spawnResources()
{
    std::array<int, TOTAL_RESOURCES> total_resources = {0};

    debug_logf("[World] Spawning resources");

    // TODO: keep track of all resources on the map instead of counting each time
    for (int x = 0; x < size.x; x++)
        for (int y = 0; y < size.y; y++)
            for (size_t i = 0; i < total_resources.size(); i++)
                total_resources[i] += tiles[x][y].getResources()[i];

    for (int i = 0; i < TOTAL_RESOURCES; i++)
        spawnResource(static_cast<ResourceType>(i), total_resources[i]);
}

void World::tick()
{
    debug_logf("[World] Tick {}", tick_count);

    if (tick_count % 20 == 0)
        spawnResources();

    tick_count++;

    // in nanoseconds
    long time_ns = 1'000'000'000L / frequence;
    const struct timespec sleep_time {
        .tv_sec  = time_ns / 1'000'000'000L,
        .tv_nsec = time_ns % 1'000'000'000L,
    };
    nanosleep(&sleep_time, NULL);

    for (const auto& team:teams)
        team.second->tickDrones();

    for (int x = 0; x < size.x; x++)
        for (int y = 0; y < size.y; y++)
            tiles[x][y].clearDrones();

    for (const auto& drone:drone_index)
        (*this)[drone.second->getPos()].addDrone(drone.first);
}

void World::createTeam(const std::string &name, int slots)
{
    teams[name] = std::make_unique<Team>(name, slots, size, *this);
}

Team* World::getTeam(const std::string& name) const
{
    auto it = teams.find(name);
    if (it != teams.end())
        return it->second.get();
    return nullptr;
}

Drone* World::getDrone(int id) const
{
    verbose_logf("[WORLD] Looking for drone {}", id);

    auto it = drone_index.find(id);
    return it != drone_index.end() ? it->second : nullptr;
}

void World::registerDrone(Drone *d)
{
    drone_index[d->id] = d;
}

bool World::unregisterDrone(int id)
{
    auto it = drone_index.find(id);
    if (it == drone_index.end())
        return false;
    recently_died.push_back(id);
    drone_index.erase(it);
    return true;
}

void World::killDrone(int id)
{
    for (auto& [name, team] : teams)
        if (team->removeDrone(id))
            break;
    unregisterDrone(id);
}

std::vector<int> World::getAndClearRecentlyDied()
{
    std::vector<int> result = std::move(recently_died);
    recently_died.clear();
    return result;
}

std::vector<std::string> World::getTeamNames() const
{
    std::vector<std::string> names;

    for (const auto& name:teams)
        names.push_back(name.first);

    return names;
}

int World::getDroneCount() const
{
    return drone_index.size();
}

const std::unordered_map<int, Drone*>& World::getDrones() const
{
    return drone_index;
}
