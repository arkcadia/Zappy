#include "Inventory.hpp"

Inventory::Inventory() : _items{} {}

int Inventory::get(Resource r) const
{
    return _items[static_cast<std::size_t>(r)];
}

void Inventory::set(Resource r, int quantity)
{
    _items[static_cast<std::size_t>(r)] = quantity;
}

void Inventory::add(Resource r, int quantity)
{
    _items[static_cast<std::size_t>(r)] += quantity;
}
