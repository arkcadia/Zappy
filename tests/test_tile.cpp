#include <catch2/catch_test_macros.hpp>
#include "../src/map/Drone.hpp"
#include "../src/map/World.hpp"

TEST_CASE("Tile default constructor initializes resources to 0", "[tile]") {
    Tile t;
    auto res = t.getResources();
    for (int i = 0; i < TOTAL_RESOURCES; i++)
        REQUIRE(res[i] == 0);
}

TEST_CASE("Tile operator[] access and modify", "[tile]") {
    Tile t;
    t[ResourceType::FOOD] = 5;
    t[ResourceType::THYSTAME] = 3;
    REQUIRE(t.getResources()[0] == 5);
    REQUIRE(t.getResources()[6] == 3);
}

TEST_CASE("Tile operator+= adds resources", "[tile]") {
    Tile t;
    std::array<int, TOTAL_RESOURCES> add = {1, 2, 3, 4, 5, 6, 7};
    t += add;
    auto res = t.getResources();
    for (int i = 0; i < TOTAL_RESOURCES; i++)
        REQUIRE(res[i] == add[i]);
}

TEST_CASE("Tile operator+= accumulates", "[tile]") {
    Tile t;
    std::array<int, TOTAL_RESOURCES> add = {1, 0, 0, 0, 0, 0, 0};
    t += add;
    t += add;
    REQUIRE(t.getResources()[0] == 2);
}

TEST_CASE("Tile operator-= succeeds when resources sufficient", "[tile]") {
    Tile t;
    std::array<int, TOTAL_RESOURCES> add = {5, 5, 5, 5, 5, 5, 5};
    t += add;
    std::array<int, TOTAL_RESOURCES> sub = {2, 2, 2, 2, 2, 2, 2};
    bool result = (t -= sub);
    REQUIRE(result);
    auto res = t.getResources();
    for (int i = 0; i < TOTAL_RESOURCES; i++)
        REQUIRE(res[i] == 3);
}

TEST_CASE("Tile operator-= fails when any resource insufficient", "[tile]") {
    Tile t;
    std::array<int, TOTAL_RESOURCES> add = {5, 5, 5, 5, 5, 5, 5};
    t += add;
    // Try to remove more thystame than available
    std::array<int, TOTAL_RESOURCES> sub = {0, 0, 0, 0, 0, 0, 10};
    bool result = (t -= sub);
    REQUIRE_FALSE(result);
    // Resources should be unchanged (all-or-nothing)
    auto res = t.getResources();
    for (int i = 0; i < TOTAL_RESOURCES; i++)
        REQUIRE(res[i] == 5);
}

TEST_CASE("Tile operator-= succeeds when resources equal (uses < check)", "[tile]") {
    Tile t;
    std::array<int, TOTAL_RESOURCES> add = {5, 5, 5, 5, 5, 5, 5};
    t += add;
    // Exact equal amount — the code uses < so this should succeed
    std::array<int, TOTAL_RESOURCES> sub = {5, 5, 5, 5, 5, 5, 5};
    bool result = (t -= sub);
    REQUIRE(result);
    auto res = t.getResources();
    for (int i = 0; i < TOTAL_RESOURCES; i++)
        REQUIRE(res[i] == 0);
}

TEST_CASE("Tile operator-= with zero subtraction", "[tile]") {
    Tile t;
    std::array<int, TOTAL_RESOURCES> add = {5, 5, 5, 5, 5, 5, 5};
    t += add;
    std::array<int, TOTAL_RESOURCES> sub = {0, 0, 0, 0, 0, 0, 0};
    // 5 <= 0 is false, so all pass the check -> should succeed
    bool result = (t -= sub);
    REQUIRE(result);
}
