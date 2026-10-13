#include <catch2/catch_test_macros.hpp>
#include "../src/server/AIProtocol.hpp"

// ── resource_name ──

TEST_CASE("resource_name returns correct names", "[ai_protocol]") {
    REQUIRE(resource_name(0) == "food");
    REQUIRE(resource_name(1) == "linemate");
    REQUIRE(resource_name(2) == "deraumere");
    REQUIRE(resource_name(3) == "sibur");
    REQUIRE(resource_name(4) == "mendiane");
    REQUIRE(resource_name(5) == "phiras");
    REQUIRE(resource_name(6) == "thystame");
}

TEST_CASE("resource_name returns empty for invalid index", "[ai_protocol]") {
    REQUIRE(resource_name(-1) == "");
    REQUIRE(resource_name(7) == "");
    REQUIRE(resource_name(100) == "");
}

// ── resource_index ──

TEST_CASE("resource_index returns correct indices", "[ai_protocol]") {
    REQUIRE(resource_index("food") == 0);
    REQUIRE(resource_index("linemate") == 1);
    REQUIRE(resource_index("deraumere") == 2);
    REQUIRE(resource_index("sibur") == 3);
    REQUIRE(resource_index("mendiane") == 4);
    REQUIRE(resource_index("phiras") == 5);
    REQUIRE(resource_index("thystame") == 6);
}

TEST_CASE("resource_index returns -1 for invalid name", "[ai_protocol]") {
    REQUIRE(resource_index("") == -1);
    REQUIRE(resource_index("gold") == -1);
    REQUIRE(resource_index("Food") == -1);  // case sensitive
    REQUIRE(resource_index("LINEMATE") == -1);
}

TEST_CASE("resource_name and resource_index roundtrip", "[ai_protocol]") {
    for (int i = 0; i < TOTAL_RESOURCES; i++) {
        std::string name = resource_name(i);
        REQUIRE(resource_index(name) == i);
    }
}

// ── ai_cmd_cost ──

TEST_CASE("ai_cmd_cost known commands", "[ai_protocol]") {
    REQUIRE(ai_cmd_cost("Inventory") == 1);
    REQUIRE(ai_cmd_cost("Fork") == 42);
    REQUIRE(ai_cmd_cost("Incantation") == 300);
}

TEST_CASE("ai_cmd_cost default is 7", "[ai_protocol]") {
    REQUIRE(ai_cmd_cost("Forward") == 7);
    REQUIRE(ai_cmd_cost("Right") == 7);
    REQUIRE(ai_cmd_cost("Left") == 7);
    REQUIRE(ai_cmd_cost("Look") == 7);
    REQUIRE(ai_cmd_cost("unknown_command") == 7);
}

// ── build_inventory_response ──

TEST_CASE("build_inventory_response format", "[ai_protocol]") {
    std::array<int, TOTAL_RESOURCES> inv = {10, 0, 0, 0, 0, 0, 0};
    std::string r = build_inventory_response(inv);
    REQUIRE(r == "[food 10, linemate 0, deraumere 0, sibur 0, mendiane 0, phiras 0, thystame 0]");
}

TEST_CASE("build_inventory_response all zeros", "[ai_protocol]") {
    std::array<int, TOTAL_RESOURCES> inv = {0, 0, 0, 0, 0, 0, 0};
    std::string r = build_inventory_response(inv);
    REQUIRE(r == "[food 0, linemate 0, deraumere 0, sibur 0, mendiane 0, phiras 0, thystame 0]");
}

// ── build_look_response ──

TEST_CASE("build_look_response with empty tiles", "[ai_protocol]") {
    Tile t1, t2;
    std::vector<Tile*> tiles = {&t1, &t2};
    std::string r = build_look_response(tiles);
    // No drones on tiles, so no "player" prefix
    REQUIRE(r == "[, ]");
}

TEST_CASE("build_look_response with resources on tiles", "[ai_protocol]") {
    Tile t1, t2;
    t1[ResourceType::FOOD] = 2;
    t2[ResourceType::LINEMATE] = 1;
    std::vector<Tile*> tiles = {&t1, &t2};
    std::string r = build_look_response(tiles);
    REQUIRE(r == "[food food, linemate]");
}
