#pragma once

#include "Vector.hpp"
#include "Team.hpp"
#include "../debug.hpp"

#include <array>
#include <atomic>
#include <format>
#include <vector>

#define TOTAL_RESOURCES 7
#define FOOD_VALUE 126

/* directions mapping
// 1 2 3
// 4 0 5
// 6 7 8
*/
enum class Direction {
    NONE       = 0,
    NORTH_WEST = 1,
    NORTH      = 2,
    NORTH_EAST = 3,
    EAST       = 4,
    SOUTH_EAST = 5,
    SOUTH      = 6,
    SOUTH_WEST = 7,
    WEST       = 8,
};

Direction operator++(Direction& d, int);

Direction operator--(Direction& d, int);

template <>
struct std::formatter<Direction> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const Direction& v, std::format_context& ctx) const
    {
        string_view name;

        switch (v) {
            case Direction::NONE:       name = "None";    break;
            case Direction::NORTH_WEST: name = "NW";      break;
            case Direction::NORTH:      name = "N";       break;
            case Direction::NORTH_EAST: name = "NE";      break;
            case Direction::EAST:       name = "E";       break;
            case Direction::SOUTH_EAST: name = "SE";      break;
            case Direction::SOUTH:      name = "S";       break;
            case Direction::SOUTH_WEST: name = "SW";      break;
            case Direction::WEST:       name = "W";       break;
            default:                    name = "Unknown"; break;
        }
        return std::format_to(ctx.out(), "{}", name);
    }
};

inline static constexpr Point dir_vec[] = {
    { 0,  0}, // NONE
    {-1, -1}, // NORTH_WEST
    { 0, -1}, // NORTH
    { 1, -1}, // NORTH_EAST
    { 1,  0}, // EAST
    { 1,  1}, // SOUTH_EAST
    { 0,  1}, // SOUTH
    {-1,  1}, // SOUTH_WEST
    {-1,  0}, // WEST
};

typedef enum {
    MOVE,
    ROTATE_R,
    ROTATE_L,
    LOOK,
    INVENTORY,
    BROADCAST,
    FORK,
    EJECT,
    TAKE,
    SET,
    CHANT, // incantation
} action_t;

// Columns: food(0) linemate(1) deraumere(2) sibur(3) mendiane(4) phiras(5) thystame(6)
// food is never required for incantation (always 0).
// The players column from the spec is NOT stored here (checked separately).
inline static constexpr std::array<int, 7> players_required = {
    1, 2, 2, 4, 4, 6, 6,
};

inline static constexpr std::array<std::array<int, TOTAL_RESOURCES>, 7> level_up_table = {{
    {0, 1, 0, 0, 0, 0, 0},  // 1->2:  1 linemate
    {0, 1, 1, 1, 0, 0, 0},  // 2->3:  1 linemate, 1 deraumere, 1 sibur
    {0, 2, 0, 1, 0, 2, 0},  // 3->4:  2 linemate, 1 sibur, 2 phiras
    {0, 1, 1, 2, 0, 1, 0},  // 4->5:  1 linemate, 1 deraumere, 2 sibur, 1 phiras
    {0, 1, 2, 1, 3, 0, 0},  // 5->6:  1 linemate, 2 deraumere, 1 sibur, 3 mendiane
    {0, 1, 2, 3, 0, 1, 0},  // 6->7:  1 linemate, 2 deraumere, 3 sibur, 1 phiras
    {0, 2, 2, 2, 2, 2, 1},  // 7->8:  2 linemate, 2 deraumere, 2 sibur, 2 mendiane, 2 phiras, 1 thystame
}};

class Tile;

class World;

struct Egg {
    Point pos;
    int id;
};

class Drone {
public:
    Drone(Team& team, Point& spawn);
    ~Drone();

    void move(Point map_size);
    void rotateR();
    void rotateL();

    std::vector<Tile *> look(World &world) const;
    std::array<int, TOTAL_RESOURCES> inventory() const;
    void broadcast() const;

    int unusedSlots() const;
    Egg drone_fork();
    std::vector<int> eject(World& world);

    bool take(std::array<int, TOTAL_RESOURCES> objects, World& world);
    bool set(std::array<int, TOTAL_RESOURCES> objects, World& world);

    Point getPos() const;
    Direction getDir() const;
    int getLevel() const;

    bool canIncantate(const World& world) const;
    // Only levels up drones from incanting_ids that are on the same tile at the same level.
    int startIncantation(World& world, const std::vector<int>& incanting_ids);

    bool tick();

    const int id;

private:
    Point pos;
    Direction dir;

    int level = 1;

    int survive_time = 0;
    std::array<int, TOTAL_RESOURCES> resources = {0};

    Team& team;

    int egg_counter;

    static std::atomic<int> next_id;
};
