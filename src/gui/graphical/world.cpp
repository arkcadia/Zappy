#include "world.hpp"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

World::World(uint16_t width, uint16_t height)
    : m_width(width), m_height(height),
      m_tiles(static_cast<size_t>(width) * static_cast<size_t>(height)) {}

uint16_t World::index(uint16_t x, uint16_t y) const {
    if (x >= m_width || y >= m_height) {
        throw std::out_of_range("Tile coordinates out of bounds");
    }
    return y * m_width + x;
}

Tile &World::getTile(uint16_t x, uint16_t y) { return m_tiles[index(x, y)]; }

const Tile &World::getTile(uint16_t x, uint16_t y) const { return m_tiles[index(x, y)]; }

void World::setResource(uint16_t x, uint16_t y, ResourceType type, uint16_t amount) {
    getTile(x, y).resources.set(type, amount);
    markDirty(x, y);
}

void World::addResource(uint16_t x, uint16_t y, ResourceType type, uint16_t amount) {
    getTile(x, y).resources.add(type, amount);
    markDirty(x, y);
}

void World::addPlayer(int id, int x, int y)
{
    updatePlayerPosition(id, x, y, 0);
}

const Player *World::findPlayer(int id) const
{
    auto it = m_players.find(id);
    if (it == m_players.end())
        return nullptr;
    return &it->second;
}

Player *World::findPlayer(int id)
{
    auto it = m_players.find(id);
    if (it == m_players.end())
        return nullptr;
    return &it->second;
}

std::vector<const Player *> World::getPlayersAt(int x, int y) const
{
    std::vector<const Player *> players;

    for (const auto &[id, player] : m_players) {
        (void)id;
        if (player.x == x && player.y == y && player.alive)
            players.push_back(&player);
    }
    std::sort(players.begin(), players.end(), [](const Player *a, const Player *b) {
        return a->id < b->id;
    });
    return players;
}

void World::addTeam(const std::string &name)
{
    if (name.empty())
        return;
    if (std::find(m_teams.begin(), m_teams.end(), name) == m_teams.end())
        m_teams.push_back(name);
}

void World::upsertPlayer(int id, int x, int y, int orientation, int level,
                         const std::string &teamName)
{
    Player &player = m_players[id];

    player.id = id;
    player.x = x;
    player.y = y;
    player.fromX = static_cast<float>(x);
    player.fromY = static_cast<float>(y);
    player.hasMotion = false;
    player.orientation = orientation;
    player.level = level;
    player.alive = true;
    if (!teamName.empty()) {
        player.teamName = teamName;
        addTeam(teamName);
    }
}

void World::updatePlayerPosition(int id, int x, int y, int orientation, float now)
{
    auto it = m_players.find(id);
    bool known = it != m_players.end();
    Player &player = m_players[id];

    if (known && (player.x != x || player.y != y) && now >= 0.0f) {
        float fromX = static_cast<float>(player.x);
        float fromY = static_cast<float>(player.y);
        int dx = x - player.x;
        int dy = y - player.y;

        if (m_width > 0 && std::abs(dx) * 2 > m_width)
            fromX += dx > 0 ? static_cast<float>(m_width) : -static_cast<float>(m_width);
        if (m_height > 0 && std::abs(dy) * 2 > m_height)
            fromY += dy > 0 ? static_cast<float>(m_height) : -static_cast<float>(m_height);
        player.fromX = fromX;
        player.fromY = fromY;
        player.moveStartedAt = now;
        player.hasMotion = true;
    } else if (!known) {
        player.fromX = static_cast<float>(x);
        player.fromY = static_cast<float>(y);
        player.hasMotion = false;
    }

    player.id = id;
    player.x = x;
    player.y = y;
    if (orientation >= 1 && orientation <= 4)
        player.orientation = orientation;
    player.alive = true;
}

void World::updatePlayerLevel(int id, int level)
{
    Player &player = m_players[id];

    player.id = id;
    player.level = level;
    player.alive = true;
}

void World::updatePlayerInventory(int id, int x, int y, const ResourceCount &inventory)
{
    Player &player = m_players[id];

    player.id = id;
    player.x = x;
    player.y = y;
    player.inventory = inventory;
    player.hasInventory = true;
    player.alive = true;
}

void World::setPlayerEvent(int id, const std::string &event)
{
    Player *player = findPlayer(id);
    if (player)
        player->lastEvent = event;
}

void World::setPlayerMessage(int id, const std::string &message)
{
    Player *player = findPlayer(id);
    if (player)
        player->lastMessage = message;
}

void World::removePlayer(int id)
{
    m_players.erase(id);
}

void World::clearPlayers()
{
    m_players.clear();
}

void World::addEgg(int id, int parentId, int x, int y)
{
    Egg egg;

    egg.id = id;
    egg.parentId = parentId;
    egg.x = x;
    egg.y = y;
    if (const Player *parent = findPlayer(parentId))
        egg.teamName = parent->teamName;
    m_eggs[id] = egg;
}

void World::removeEgg(int id)
{
    m_eggs.erase(id);
}

std::vector<const Egg *> World::getEggsAt(int x, int y) const
{
    std::vector<const Egg *> eggs;

    for (const auto &[id, egg] : m_eggs) {
        (void)id;
        if (egg.x == x && egg.y == y)
            eggs.push_back(&egg);
    }
    std::sort(eggs.begin(), eggs.end(), [](const Egg *a, const Egg *b) {
        return a->id < b->id;
    });
    return eggs;
}

void World::startIncantation(int x, int y, int level, const std::vector<int> &playerIds)
{
    Incantation incantation;

    incantation.x = x;
    incantation.y = y;
    incantation.level = level;
    incantation.playerIds = playerIds;
    incantation.active = true;
    m_incantations.push_back(incantation);

    for (int id : playerIds) {
        Player &player = m_players[id];
        player.id = id;
        player.x = x;
        player.y = y;
        player.level = level;
        player.inIncantation = true;
        player.lastEvent = "incantation";
    }
}

void World::finishIncantation(int x, int y, bool success)
{
    for (auto &incantation : m_incantations) {
        if (incantation.x != x || incantation.y != y || !incantation.active)
            continue;
        incantation.active = false;
        incantation.hasResult = true;
        incantation.success = success;
        for (int id : incantation.playerIds) {
            Player *player = findPlayer(id);
            if (!player)
                continue;
            player->inIncantation = false;
            player->lastEvent = success ? "incantation ok" : "incantation ko";
            if (success)
                player->level = std::max(player->level, incantation.level + 1);
        }
    }
}

void World::adjustPlayerResource(int id, ResourceType type, int delta)
{
    Player *player = findPlayer(id);
    if (!player || !player->hasInventory)
        return;

    uint16_t current = player->inventory.get(type);
    int next = static_cast<int>(current) + delta;
    if (next < 0)
        next = 0;
    player->inventory.set(type, static_cast<uint16_t>(next));
}

void World::adjustTileResource(int x, int y, ResourceType type, int delta)
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height)
        return;

    Tile &tile = getTile(static_cast<uint16_t>(x), static_cast<uint16_t>(y));
    uint16_t current = tile.resources.get(type);
    int next = static_cast<int>(current) + delta;
    if (next < 0)
        next = 0;
    tile.resources.set(type, static_cast<uint16_t>(next));
    markDirty(static_cast<uint16_t>(x), static_cast<uint16_t>(y));
}

void World::markDirty(uint16_t x, uint16_t y) { getTile(x, y).isDirty = true; }

void World::clearDirtyFlags() {
    for (auto &tile : m_tiles) {
        tile.isDirty = false;
    }
}
