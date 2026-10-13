#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @file world.hpp
 * @brief World data model: tiles, resources and their counts.
 *
 * Defines ResourceType, ResourceCount, Tile, and the World container that
 * holds the entire game grid received from the server.
 */

/**
 * @brief Identifies each of the seven Zappy resource types.
 *
 * The underlying type is `uint8_t` so values can be used directly as array
 * indices. `Count` is a sentinel that equals the number of valid resource types.
 */
enum class ResourceType : uint8_t {
    Food,      ///< Consumable food item (yellow).
    Linemate,  ///< Level-1 mineral (green).
    Deraumere, ///< Level-2 mineral (red).
    Sibur,     ///< Level-3 mineral (blue).
    Mendiane,  ///< Level-4 mineral (purple).
    Phiras,    ///< Level-5 mineral (gold).
    Thystame,  ///< Level-6 mineral (cyan).
    Count      ///< Sentinel — total number of resource types.
};

/**
 * @brief Stores the quantity of every resource type on a single tile.
 *
 * The amounts array is indexed by casting a ResourceType to uint8_t.
 */
struct ResourceCount {
    uint16_t amounts[static_cast<uint8_t>(ResourceType::Count)] =
        {}; ///< Per-type amounts, initialised to zero.

    /**
     * @brief Returns the quantity of a given resource type.
     * @param type The resource type to query.
     * @return Quantity currently stored for that type.
     */
    uint16_t get(ResourceType type) const { return amounts[static_cast<uint8_t>(type)]; }

    /**
     * @brief Sets the quantity of a given resource type, overwriting the previous value.
     * @param type   The resource type to set.
     * @param amount New quantity.
     */
    void set(ResourceType type, uint16_t amount) { amounts[static_cast<uint8_t>(type)] = amount; }

    /**
     * @brief Adds to the quantity of a given resource type.
     * @param type   The resource type to increment.
     * @param amount Amount to add.
     */
    void add(ResourceType type, uint16_t amount) { amounts[static_cast<uint8_t>(type)] += amount; }

    /**
     * @brief Returns true if at least one resource type has a non-zero quantity.
     * @return `true` when the tile contains any resource, `false` when empty.
     */
    bool hasAny() const {
        for (uint8_t i = 0; i < static_cast<uint8_t>(ResourceType::Count); ++i)
            if (amounts[i] > 0)
                return true;
        return false;
    }
};

/**
 * @brief Represents a single cell in the game grid.
 *
 * A Tile owns a ResourceCount and a dirty flag used to track cells that
 * changed since the last statistics update.
 */
struct Tile {
    ResourceCount resources; ///< Resource quantities on this tile.
    bool isDirty =
        false; ///< Set to true when resources are modified; cleared by World::clearDirtyFlags().
};

struct Player {
    int id = -1;
    int x = 0;
    int y = 0;
    float fromX = 0.0f;
    float fromY = 0.0f;
    float moveStartedAt = 0.0f;
    float moveDuration = 0.22f;
    bool hasMotion = false;
    int orientation = 0;
    int level = 1;
    std::string teamName;
    ResourceCount inventory;
    bool hasInventory = false;
    bool inIncantation = false;
    bool alive = true;
    std::string lastMessage;
    std::string lastEvent;
};

struct Egg {
    int id = -1;
    int parentId = -1;
    int x = 0;
    int y = 0;
    std::string teamName;
};

struct Incantation {
    int x = 0;
    int y = 0;
    int level = 0;
    std::vector<int> playerIds;
    bool active = false;
    bool hasResult = false;
    bool success = false;
};

/**
 * @brief Container for the full game grid received from the server.
 *
 * Tiles are stored in row-major order in a flat vector. All coordinate access
 * is bounds-checked and throws std::out_of_range on invalid input.
 */
class World {
  public:
    /**
     * @brief Constructs a world of the given dimensions, initialising all tiles to empty.
     * @param width  Number of tile columns.
     * @param height Number of tile rows.
     */
    World(uint16_t width, uint16_t height);

    /**
     * @brief Returns a mutable reference to the tile at (x, y).
     * @param x Column index [0, width).
     * @param y Row index    [0, height).
     * @throws std::out_of_range if coordinates are out of bounds.
     */
    Tile &getTile(uint16_t x, uint16_t y);

    /**
     * @brief Returns a const reference to the tile at (x, y).
     * @param x Column index [0, width).
     * @param y Row index    [0, height).
     * @throws std::out_of_range if coordinates are out of bounds.
     */
    const Tile &getTile(uint16_t x, uint16_t y) const;

    /**
     * @brief Overwrites the resource amount at the given tile and marks it dirty.
     * @param x      Column index.
     * @param y      Row index.
     * @param type   Resource type to update.
     * @param amount New quantity.
     */
    void setResource(uint16_t x, uint16_t y, ResourceType type, uint16_t amount);

    /**
     * @brief Adds to the resource amount at the given tile and marks it dirty.
     * @param x      Column index.
     * @param y      Row index.
     * @param type   Resource type to increment.
     * @param amount Amount to add.
     */
    void addResource(uint16_t x, uint16_t y, ResourceType type, uint16_t amount);

    /**
     * @brief Marks the tile at (x, y) as dirty without modifying its resources.
     * @param x Column index.
     * @param y Row index.
     */
    void markDirty(uint16_t x, uint16_t y);

    /**
     * @brief Clears the dirty flag on every tile in the grid.
     */
    void clearDirtyFlags();

    /** @brief Returns the number of tile columns. */
    uint16_t getWidth() const { return m_width; }

    /** @brief Returns the number of tile rows. */
    uint16_t getHeight() const { return m_height; }

    size_t getPlayerCount() const { return m_players.size(); }

    Player getPlayer(int id) const { return m_players.at(id); }
    const Player *findPlayer(int id) const;
    Player *findPlayer(int id);
    const std::unordered_map<int, Player> &getPlayers() const { return m_players; }
    std::vector<const Player *> getPlayersAt(int x, int y) const;

    void addTeam(const std::string &name);
    const std::vector<std::string> &getTeams() const { return m_teams; }

    void addPlayer(int id, int x, int y);
    void upsertPlayer(int id, int x, int y, int orientation, int level,
                      const std::string &teamName);
    void updatePlayerPosition(int id, int x, int y, int orientation, float now = -1.0f);
    void updatePlayerLevel(int id, int level);
    void updatePlayerInventory(int id, int x, int y, const ResourceCount &inventory);
    void setPlayerEvent(int id, const std::string &event);
    void setPlayerMessage(int id, const std::string &message);
    void removePlayer(int id);
    void clearPlayers();

    void addEgg(int id, int parentId, int x, int y);
    void removeEgg(int id);
    const std::unordered_map<int, Egg> &getEggs() const { return m_eggs; }
    std::vector<const Egg *> getEggsAt(int x, int y) const;

    void startIncantation(int x, int y, int level, const std::vector<int> &playerIds);
    void finishIncantation(int x, int y, bool success);
    const std::vector<Incantation> &getIncantations() const { return m_incantations; }

    void adjustPlayerResource(int id, ResourceType type, int delta);
    void adjustTileResource(int x, int y, ResourceType type, int delta);

  private:
    uint16_t m_width;          ///< Number of columns.
    uint16_t m_height;         ///< Number of rows.
    std::vector<Tile> m_tiles; ///< Flat row-major tile storage.
    std::unordered_map<int, Player> m_players = {};
    std::unordered_map<int, Egg> m_eggs = {};
    std::vector<std::string> m_teams = {};
    std::vector<Incantation> m_incantations = {};

    /**
     * @brief Converts 2-D coordinates to a flat vector index.
     * @param x Column index.
     * @param y Row index.
     * @return Flat index into m_tiles.
     * @throws std::out_of_range if (x, y) is outside the grid.
     */
    uint16_t index(uint16_t x, uint16_t y) const;
};
