#include <catch2/catch_test_macros.hpp>
#include "../src/map/Drone.hpp"
#include "../src/map/World.hpp"

TEST_CASE("Team initial slots match constructor argument", "[team]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("alpha", 5);
    Team* t = w.getTeam("alpha");
    REQUIRE(t != nullptr);
    REQUIRE(t->slots() == 5);
}

TEST_CASE("Team createDrone decrements slots", "[team]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("alpha", 3);
    Team* t = w.getTeam("alpha");
    REQUIRE(t->slots() == 3);
    t->createDrone();
    REQUIRE(t->slots() == 2);
    t->createDrone();
    REQUIRE(t->slots() == 1);
}

TEST_CASE("Team createDrone returns nullptr when no eggs", "[team]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("alpha", 2);
    Team* t = w.getTeam("alpha");
    t->createDrone();
    t->createDrone();
    // All eggs consumed
    Drone* d = t->createDrone();
    REQUIRE(d == nullptr);
}

TEST_CASE("Team createDrone returns valid drone with unique id", "[team]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("alpha", 3);
    Team* t = w.getTeam("alpha");
    Drone* d1 = t->createDrone();
    Drone* d2 = t->createDrone();
    REQUIRE(d1 != nullptr);
    REQUIRE(d2 != nullptr);
    REQUIRE(d1->id != d2->id);
}

TEST_CASE("Team add_egg increases available spawns", "[team]") {
    World w(Vector2i(10, 10), 100);
    w.createTeam("alpha", 1);
    Team* t = w.getTeam("alpha");
    // Use the one egg
    Drone* d1 = t->createDrone();
    REQUIRE(d1 != nullptr);
    // No more eggs
    REQUIRE(t->createDrone() == nullptr);
    // Add an egg manually
    Point p(5, 5);
    t->add_egg(p);
    // Now we can create another drone
    Drone* d2 = t->createDrone();
    REQUIRE(d2 != nullptr);
}
