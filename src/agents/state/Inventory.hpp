#pragma once

#include <array>
#include <cstddef>

enum class Resource {
    Food,
    Linemate,
    Deraumere,
    Sibur,
    Mendiane,
    Phiras,
    Thystame,
    Count
};

const std::size_t RESOURCE_COUNT = static_cast<std::size_t>(Resource::Count);

class Inventory {
public:
    Inventory();
    ~Inventory() = default;

    int get(Resource r) const;
    void set(Resource r, int quantity);
    void add(Resource r, int quantity);

private:
    std::array<int, RESOURCE_COUNT> _items;
};
