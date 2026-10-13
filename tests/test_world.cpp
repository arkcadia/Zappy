#include <catch2/catch_test_macros.hpp>
#include "../src/map/Drone.hpp"
#include "../src/map/World.hpp"

TEST_CASE("World constructor creates correct size", "[world]") {
    World w(Vector2i(10, 15), 100);
    REQUIRE(w.getSize().x == 10);
    REQUIRE(w.getSize().y == 15);
}

TEST_CASE("World operator[] wraps positive out-of-bounds", "[world]") {
    World w(Vector2i(10, 10), 100);
    // Access (12, 13) should wrap to (2, 3)
    Tile& t1 = w[Point(12, 13)];
    Tile& t2 = w[Point(2, 3)];
    REQUIRE(&t1 == &t2);
}

TEST_CASE("World operator[] wraps negative coordinates", "[world]") {
    World w(Vector2i(10, 10), 100);
    // Access (-1, -1) should wrap to (9, 9)
    Tile& t1 = w[Point(-1, -1)];
    Tile& t2 = w[Point(9, 9)];
    REQUIRE(&t1 == &t2);
}

TEST_CASE("World operator[] wraps large negative", "[world]") {
    World w(Vector2i(10, 10), 100);
    Tile& t1 = w[Point(-25, -37)];
    Tile& t2 = w[Point(5, 3)];
    REQUIRE(&t1 == &t2);
}

TEST_CASE("World operator[] zero coordinate", "[world]") {
    World w(Vector2i(10, 10), 100);
    Tile& t1 = w[Point(0, 0)];
    Tile& t2 = w[Point(10, 10)];
    REQUIRE(&t1 == &t2);
}

TEST_CASE("World createTeam and getTeam", "[world]") {
    World w(Vector2i(5, 5), 100);
    w.createTeam("alpha", 3);
    REQUIRE(w.getTeam("alpha") != nullptr);
    REQUIRE(w.getTeam("nonexistent") == nullptr);
}

TEST_CASE("World getTeamNames", "[world]") {
    World w(Vector2i(5, 5), 100);
    w.createTeam("alpha", 2);
    w.createTeam("beta", 2);
    auto names = w.getTeamNames();
    REQUIRE(names.size() == 2);
    // Order not guaranteed in unordered_map, just check both exist
    bool has_alpha = false, has_beta = false;
    for (const auto& n : names) {
        if (n == "alpha") has_alpha = true;
        if (n == "beta") has_beta = true;
    }
    REQUIRE(has_alpha);
    REQUIRE(has_beta);
}

TEST_CASE("World tick increments counter", "[world]") {
    World w(Vector2i(5, 5), 10000); // high freq to avoid sleep
    REQUIRE(w.getTickCount() == 0);
    w.tick();
    REQUIRE(w.getTickCount() == 1);
}

TEST_CASE("World spawnResources puts resources on map", "[world]") {
    World w(Vector2i(10, 10), 100);
    // After construction, spawnResources was already called
    // Check that at least some resources exist
    int total = 0;
    for (int x = 0; x < 10; x++)
        for (int y = 0; y < 10; y++) {
            auto res = w[Point(x, y)].getResources();
            for (int i = 0; i < TOTAL_RESOURCES; i++)
                total += res[i];
        }
    REQUIRE(total > 0);
}
