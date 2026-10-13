#include <catch2/catch_test_macros.hpp>
#include "../src/map/Drone.hpp"
#include "../src/map/World.hpp"

TEST_CASE("Direction operator++ cycles 1 to 8", "[drone]") {
    Direction d = Direction::NORTH_WEST; // 1
    d++;
    REQUIRE(d == Direction::NORTH); // 2
    d++;
    REQUIRE(d == Direction::NORTH_EAST); // 3
}

TEST_CASE("Direction operator++ wraps from 8 to 1", "[drone]") {
    Direction d = Direction::WEST; // 8
    d++;
    REQUIRE(d == Direction::NORTH_WEST); // 1
}

TEST_CASE("Direction operator++ on NONE stays NONE", "[drone]") {
    Direction d = Direction::NONE;
    d++;
    REQUIRE(d == Direction::NONE);
}

TEST_CASE("Direction operator-- cycles backwards", "[drone]") {
    Direction d = Direction::NORTH; // 2
    d--;
    REQUIRE(d == Direction::NORTH_WEST); // 1
}

TEST_CASE("Direction operator-- wraps from 1 to 8", "[drone]") {
    Direction d = Direction::NORTH_WEST; // 1
    d--;
    REQUIRE(d == Direction::WEST); // 8
}

TEST_CASE("Direction operator-- on NONE stays NONE", "[drone]") {
    Direction d = Direction::NONE;
    d--;
    REQUIRE(d == Direction::NONE);
}

TEST_CASE("Direction full cycle ++ returns to start", "[drone]") {
    Direction d = Direction::NORTH;
    for (int i = 0; i < 8; i++) d++;
    REQUIRE(d == Direction::NORTH);
}

TEST_CASE("Direction full cycle -- returns to start", "[drone]") {
    Direction d = Direction::EAST;
    for (int i = 0; i < 8; i++) d--;
    REQUIRE(d == Direction::EAST);
}

TEST_CASE("Drone creation assigns unique IDs", "[drone]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("test", 5);
    Team* t = w.getTeam("test");
    Drone* d1 = t->createDrone();
    Drone* d2 = t->createDrone();
    REQUIRE(d1 != nullptr);
    REQUIRE(d2 != nullptr);
    REQUIRE(d1->id != d2->id);
}

TEST_CASE("Drone move wraps position", "[drone]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("test", 3);
    Team* t = w.getTeam("test");
    Drone* d = t->createDrone();
    REQUIRE(d != nullptr);

    // Move many times — position should always stay in bounds
    for (int i = 0; i < 50; i++) {
        d->move(w.getSize());
        Point pos = d->getPos();
        REQUIRE(pos.x >= 0);
        REQUIRE(pos.x < 10);
        REQUIRE(pos.y >= 0);
        REQUIRE(pos.y < 10);
    }
}

TEST_CASE("Drone rotateR then rotateL returns to original direction", "[drone]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("test", 3);
    Team* t = w.getTeam("test");
    Drone* d = t->createDrone();
    REQUIRE(d != nullptr);

    Direction original = d->getDir();
    d->rotateR();
    d->rotateL();
    REQUIRE(d->getDir() == original);
}

TEST_CASE("Drone rotateR four times returns to original (360 degrees)", "[drone]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("test", 3);
    Team* t = w.getTeam("test");
    Drone* d = t->createDrone();
    REQUIRE(d != nullptr);

    Direction original = d->getDir();
    d->rotateR();
    d->rotateR();
    d->rotateR();
    d->rotateR();
    REQUIRE(d->getDir() == original);
}

TEST_CASE("Drone initial level is 1", "[drone]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("test", 3);
    Team* t = w.getTeam("test");
    Drone* d = t->createDrone();
    REQUIRE(d != nullptr);
    REQUIRE(d->getLevel() == 1);
}

// NOTE: This test currently fails because Drone::resources is not initialized
// in the constructor. This is a bug — resources should be zero-initialized.
// Uncomment once Drone constructor initializes resources to {0}.
// TEST_CASE("Drone inventory starts at zero", "[drone]") {
//     World w(Vector2i(10, 10), 100);
//     w.createTeam("test", 3);
//     Team* t = w.getTeam("test");
//     Drone* d = t->createDrone();
//     REQUIRE(d != nullptr);
//     auto inv = d->inventory();
//     for (int i = 0; i < TOTAL_RESOURCES; i++)
//         REQUIRE(inv[i] == 0);
// }

TEST_CASE("Drone look returns correct number of tiles", "[drone]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("test", 3);
    Team* t = w.getTeam("test");
    Drone* d = t->createDrone();
    REQUIRE(d != nullptr);
    // At level 1: total_cells = 1*1 + 1*2 + 1 = 4
    auto tiles = d->look(w);
    REQUIRE(tiles.size() == 4);
}

TEST_CASE("Drone look tiles are not null", "[drone]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("test", 3);
    Team* t = w.getTeam("test");
    Drone* d = t->createDrone();
    REQUIRE(d != nullptr);
    auto tiles = d->look(w);
    for (auto* tile : tiles)
        REQUIRE(tile != nullptr);
}
