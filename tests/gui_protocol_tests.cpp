#include "../src/gui/graphical/gui_protocol_handler.hpp"
#include "../src/gui/graphical/world.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

struct TestFailure {
    std::string message;
};

static void expect(bool condition, const std::string &message)
{
    if (!condition)
        throw TestFailure{message};
}

static void expectEq(int actual, int expected, const std::string &message)
{
    if (actual != expected)
        throw TestFailure{message + " (got " + std::to_string(actual) + ", expected " +
                          std::to_string(expected) + ")"};
}

static void expectEq(const std::string &actual, const std::string &expected,
                     const std::string &message)
{
    if (actual != expected)
        throw TestFailure{message + " (got '" + actual + "', expected '" + expected + "')" };
}

struct ProtocolHarness {
    std::unique_ptr<World> world = std::make_unique<World>(5, 5);
    bool mapReceived = true;
    uint16_t width = 5;
    uint16_t height = 5;
    float time = 10.0f;
    std::vector<std::string> sentLines;
    std::vector<std::string> logs;
    std::vector<GuiProtocolAnimation> animations;
    int statsUpdates = 0;

    bool apply(const std::string &line)
    {
        GuiProtocolContext context;

        context.world = world.get();
        context.mapReceived = mapReceived;
        context.gridWidth = width;
        context.gridHeight = height;
        context.time = time;
        context.initializeWorld = [&](uint16_t w, uint16_t h) {
            world = std::make_unique<World>(w, h);
            mapReceived = true;
            width = w;
            height = h;
            context.world = world.get();
            context.mapReceived = mapReceived;
            context.gridWidth = width;
            context.gridHeight = height;
        };
        context.sendLine = [&](const std::string &line) { sentLines.push_back(line); };
        context.pushLog = [&](const std::string &text) { logs.push_back(text); };
        context.addAnimation = [&](const GuiProtocolAnimation &animation) {
            animations.push_back(animation);
        };
        context.updateMapStats = [&]() { ++statsUpdates; };
        return handleGuiProtocolMessage(line, context);
    }
};

static void test_map_initialization_and_tile_sync()
{
    ProtocolHarness h;

    h.world.reset();
    h.mapReceived = false;
    h.width = 0;
    h.height = 0;
    expect(h.apply("WELCOME"), "WELCOME should be accepted");
    expect(h.apply("msz 4 3"), "msz should be accepted");
    expect(h.world != nullptr, "msz should initialize the world");
    expectEq(h.world->getWidth(), 4, "msz should set width");
    expectEq(h.world->getHeight(), 3, "msz should set height");
    expectEq(h.sentLines.size(), 1, "msz should request team names");
    expectEq(h.sentLines[0], "tna\n", "msz should send tna request");

    expect(h.apply("bct 2 1 5 -2 1 0 3 0 7"), "valid bct should be accepted");
    const Tile &tile = h.world->getTile(2, 1);
    expectEq(tile.resources.get(ResourceType::Food), 5, "bct should set food");
    expectEq(tile.resources.get(ResourceType::Linemate), 0, "bct should clamp negative values");
    expectEq(tile.resources.get(ResourceType::Deraumere), 1, "bct should set minerals");
    expectEq(tile.resources.get(ResourceType::Thystame), 7, "bct should set thystame");
    expectEq(h.statsUpdates, 1, "bct should refresh map stats");

    expect(h.apply("bct 99 99 1 1 1 1 1 1 1"), "out-of-bounds bct should be tolerated");
    expectEq(h.statsUpdates, 2, "out-of-bounds bct still refreshes stats like production code");
}

static void test_new_player_team_and_duplicates()
{
    ProtocolHarness h;

    expect(h.apply("tna team1"), "tna should be accepted");
    expect(h.apply("pnw #7 2 3 4 2 team1"), "pnw should be accepted");
    const Player *player = h.world->findPlayer(7);
    expect(player != nullptr, "pnw should create player");
    expectEq(player->x, 2, "pnw should set x");
    expectEq(player->y, 3, "pnw should set y");
    expectEq(player->orientation, 4, "pnw should set orientation");
    expectEq(player->level, 2, "pnw should set level");
    expectEq(player->teamName, "team1", "pnw should set team");
    expectEq(h.world->getTeams().size(), 1, "team should be stored once");
    expectEq(h.sentLines.back(), "pin #7\n", "pnw should request inventory");
    expect(h.animations.back().type == GuiProtocolAnimationType::Spawn,
           "pnw should add spawn animation");

    expect(h.apply("pnw #7 4 1 2 5 team1"), "duplicate pnw should update existing player");
    player = h.world->findPlayer(7);
    expectEq(h.world->getPlayerCount(), 1, "duplicate pnw should not duplicate player");
    expectEq(player->x, 4, "duplicate pnw should update x");
    expectEq(player->level, 5, "duplicate pnw should update level");
    expectEq(h.world->getTeams().size(), 1, "duplicate team should not duplicate team");
}

static void test_position_orientation_and_unknown_position()
{
    ProtocolHarness h;

    h.apply("pnw #7 2 3 1 1 team1");
    h.animations.clear();
    expect(h.apply("ppo #7 4 1 2"), "ppo should be accepted");
    const Player *player = h.world->findPlayer(7);
    expectEq(player->x, 4, "ppo should update x");
    expectEq(player->y, 1, "ppo should update y");
    expectEq(player->orientation, 2, "ppo should update orientation");
    expect(player->hasMotion, "known moving player should keep motion data");
    expectEq(h.animations.size(), 1, "movement should add animation");
    expect(h.animations[0].type == GuiProtocolAnimationType::Move,
           "movement animation should be recorded");

    expect(h.apply("ppo #99 1 1 3"), "ppo for unknown player should be accepted");
    player = h.world->findPlayer(99);
    expect(player != nullptr, "ppo for unknown player currently creates a placeholder");
    expectEq(player->orientation, 3, "unknown ppo should set orientation");
    expect(!player->hasMotion, "unknown ppo should not animate from an old position");
}

static void test_level_inventory_and_resource_events()
{
    ProtocolHarness h;

    h.apply("pnw #7 2 3 1 1 team1");
    h.apply("bct 2 3 4 0 0 0 0 0 0");
    expect(h.apply("plv #7 4"), "plv should be accepted");
    const Player *player = h.world->findPlayer(7);
    expectEq(player->level, 4, "plv should update level");
    expectEq(player->lastEvent, "level 4", "plv should update last event");

    expect(h.apply("pin #7 2 3 9 1 2 3 4 5 6"), "pin should be accepted");
    player = h.world->findPlayer(7);
    expect(player->hasInventory, "pin should mark inventory available");
    expectEq(player->inventory.get(ResourceType::Food), 9, "pin should set food");
    expectEq(player->inventory.get(ResourceType::Thystame), 6, "pin should set thystame");

    expect(h.apply("pgt #7 0"), "pgt should be accepted");
    player = h.world->findPlayer(7);
    expectEq(player->inventory.get(ResourceType::Food), 10, "pgt should increment inventory");
    expectEq(h.world->getTile(2, 3).resources.get(ResourceType::Food), 3,
             "pgt should decrement tile resource");
    expectEq(player->lastEvent, "took food", "pgt should update event");

    expect(h.apply("pdr #7 0"), "pdr should be accepted");
    player = h.world->findPlayer(7);
    expectEq(player->inventory.get(ResourceType::Food), 9, "pdr should decrement inventory");
    expectEq(h.world->getTile(2, 3).resources.get(ResourceType::Food), 4,
             "pdr should increment tile resource");
    expectEq(player->lastEvent, "dropped food", "pdr should update event");
}

static void test_broadcast_death_and_unknown_ids()
{
    ProtocolHarness h;

    h.apply("pnw #7 2 3 1 1 team1");
    expect(h.apply("pbc #7 hello team"), "pbc should be accepted");
    const Player *player = h.world->findPlayer(7);
    expectEq(player->lastMessage, "hello team", "pbc should set message");
    expectEq(player->lastEvent, "broadcast", "pbc should set broadcast event");
    expect(h.animations.back().type == GuiProtocolAnimationType::Broadcast,
           "pbc should add broadcast animation");

    expect(h.apply("pbc #7 newer message"), "duplicate pbc should be accepted");
    player = h.world->findPlayer(7);
    expectEq(player->lastMessage, "newer message", "duplicate pbc should replace message");
    expectEq(h.world->getPlayerCount(), 1, "duplicate pbc should not duplicate player");

    expect(h.apply("pbc #42 ignored"), "unknown pbc should be tolerated");
    expect(h.world->findPlayer(42) == nullptr, "unknown pbc should not create player");

    expect(h.apply("pdi #7"), "pdi should be accepted");
    expect(h.world->findPlayer(7) == nullptr, "pdi should remove player");
    expect(h.animations.back().type == GuiProtocolAnimationType::Death,
           "pdi should add death animation for known player");

    expect(h.apply("pdi #42"), "unknown pdi should be tolerated");
    expectEq(h.world->getPlayerCount(), 0, "unknown pdi should not create player");
}

static void test_incantation_events()
{
    ProtocolHarness h;

    h.apply("pnw #7 0 0 1 1 team1");
    h.apply("pnw #8 1 1 2 1 team1");
    expect(h.apply("pic 3 4 2 #7 #8"), "pic should be accepted");
    expectEq(h.world->getIncantations().size(), 1, "pic should create incantation");
    const Incantation &incantation = h.world->getIncantations()[0];
    expect(incantation.active, "pic should mark incantation active");
    expectEq(incantation.x, 3, "pic should set x");
    expectEq(incantation.y, 4, "pic should set y");
    expectEq(incantation.level, 2, "pic should set level");
    expect(h.world->findPlayer(7)->inIncantation, "pic should mark participant");
    expectEq(h.world->findPlayer(7)->x, 3, "pic should sync participant x");
    expectEq(h.world->findPlayer(8)->level, 2, "pic should sync participant level");

    expect(h.apply("pie 3 4 1"), "pie success should be accepted");
    expect(!h.world->getIncantations()[0].active, "pie should finish incantation");
    expect(h.world->getIncantations()[0].success, "pie should store success");
    expect(!h.world->findPlayer(7)->inIncantation, "pie should clear participant flag");
    expectEq(h.world->findPlayer(7)->level, 3, "successful pie should elevate level");
    expectEq(h.world->findPlayer(7)->lastEvent, "incantation ok",
             "successful pie should update event");
}

static void test_eggs_and_parent_relationship()
{
    ProtocolHarness h;

    h.apply("pnw #7 2 2 1 1 team1");
    expect(h.apply("enw #5 #7 3 4"), "enw should be accepted");
    expectEq(h.world->getEggs().size(), 1, "enw should create egg");
    const Egg &egg = h.world->getEggs().at(5);
    expectEq(egg.id, 5, "enw should set egg id");
    expectEq(egg.parentId, 7, "enw should set parent id");
    expectEq(egg.x, 3, "enw should set x");
    expectEq(egg.y, 4, "enw should set y");
    expectEq(egg.teamName, "team1", "enw should derive team from parent");

    expect(h.apply("ebo #5"), "ebo should be accepted");
    expectEq(h.world->getEggs().size(), 0, "ebo should remove egg");
    expect(h.animations.back().type == GuiProtocolAnimationType::EggHatch,
           "ebo should add hatch animation");

    h.apply("enw #6 #7 1 1");
    expect(h.apply("edi #6"), "edi should be accepted");
    expectEq(h.world->getEggs().size(), 0, "edi should remove egg");
    expect(h.animations.back().type == GuiProtocolAnimationType::EggDeath,
           "edi should add egg death animation");
}

static void test_malformed_messages_are_safe()
{
    ProtocolHarness h;

    h.apply("pnw #7 2 3 1 1 team1");
    h.apply("pin #7 2 3 1 2 3 4 5 6 7");
    const Player before = *h.world->findPlayer(7);

    expect(!h.apply("pnw #bad 4 4 1 1 team1"), "bad pnw should be rejected");
    expect(!h.apply("pin #7 2 3 1 2"), "short pin should be rejected");
    expect(!h.apply("pgt #7 99"), "invalid resource event should be rejected");
    expect(!h.apply("does_not_exist #7"), "unknown command should be rejected");
    expect(!h.apply(""), "empty line should be rejected");

    const Player *after = h.world->findPlayer(7);
    expect(after != nullptr, "malformed messages should not remove existing player");
    expectEq(after->x, before.x, "malformed messages should preserve x");
    expectEq(after->inventory.get(ResourceType::Food), before.inventory.get(ResourceType::Food),
             "malformed pin should preserve inventory");
    expectEq(h.world->getPlayerCount(), 1, "malformed messages should not create players");
}

static int runTest(const std::string &name, void (*test)())
{
    try {
        test();
        std::cout << "[PASS] " << name << '\n';
        return 0;
    } catch (const TestFailure &failure) {
        std::cerr << "[FAIL] " << name << ": " << failure.message << '\n';
        return 1;
    } catch (const std::exception &error) {
        std::cerr << "[FAIL] " << name << ": unexpected exception: " << error.what() << '\n';
        return 1;
    }
}

int main()
{
    int failures = 0;

    failures += runTest("map initialization and tile sync", test_map_initialization_and_tile_sync);
    failures += runTest("new player, team, duplicates", test_new_player_team_and_duplicates);
    failures += runTest("position, orientation, unknown ppo",
                        test_position_orientation_and_unknown_position);
    failures += runTest("level, inventory, resource events",
                        test_level_inventory_and_resource_events);
    failures += runTest("broadcast, death, unknown ids", test_broadcast_death_and_unknown_ids);
    failures += runTest("incantation events", test_incantation_events);
    failures += runTest("egg and parent relationship", test_eggs_and_parent_relationship);
    failures += runTest("malformed messages are safe", test_malformed_messages_are_safe);

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "All GUI protocol tests passed\n";
    return 0;
}
