#include "gui_protocol_handler.hpp"
#include "world.hpp"
#include <array>
#include <sstream>
#include <vector>

static bool parseProtocolId(const std::string &token, int &id)
{
    if (token.empty())
        return false;
    std::string value = token.front() == '#' ? token.substr(1) : token;
    try {
        std::size_t parsed = 0;
        id = std::stoi(value, &parsed);
        return parsed == value.size();
    } catch (...) {
        return false;
    }
}

static std::string readRest(std::istringstream &iss)
{
    std::string text;

    std::getline(iss, text);
    if (!text.empty() && text.front() == ' ')
        text.erase(0, 1);
    return text;
}

static const char *resourceName(int index)
{
    static const char *names[] = {
        "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame",
    };

    if (index < 0 || index >= 7)
        return "unknown";
    return names[index];
}

static void pushLog(GuiProtocolContext &context, const std::string &text)
{
    if (context.pushLog)
        context.pushLog(text);
}

static void sendLine(GuiProtocolContext &context, const std::string &line)
{
    if (context.sendLine)
        context.sendLine(line);
}

static void updateMapStats(GuiProtocolContext &context)
{
    if (context.updateMapStats)
        context.updateMapStats();
}

static void addAnimation(GuiProtocolContext &context, GuiProtocolAnimation animation)
{
    if (context.addAnimation)
        context.addAnimation(animation);
}

bool handleGuiProtocolMessage(const std::string &message, GuiProtocolContext &context)
{
    std::istringstream iss(message);
    std::string cmd;

    if (!(iss >> cmd))
        return false;
    if (cmd == "WELCOME")
        return true;

    if (cmd == "msz") {
        int w = 0;
        int h = 0;

        if (!(iss >> w >> h))
            return false;
        if (w > 0 && h > 0 && !context.mapReceived && context.initializeWorld) {
            context.initializeWorld(static_cast<uint16_t>(w), static_cast<uint16_t>(h));
            sendLine(context, "tna\n");
        }
    } else if (cmd == "bct") {
        if (!context.mapReceived || !context.world)
            return false;
        int x = 0;
        int y = 0;
        std::array<int, 7> counts = {};

        if (!(iss >> x >> y))
            return false;
        for (int i = 0; i < 7; i++) {
            if (!(iss >> counts[i]))
                return false;
            if (counts[i] < 0)
                counts[i] = 0;
        }
        if (x >= 0 && x < context.gridWidth && y >= 0 && y < context.gridHeight) {
            for (int i = 0; i < 7; i++)
                context.world->setResource(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                                           static_cast<ResourceType>(i),
                                           static_cast<uint16_t>(counts[i]));
        }
        updateMapStats(context);
    } else if (cmd == "sgt") {
        int t = 0;

        if (!(iss >> t))
            return false;
        pushLog(context, "TICK FREQ " + std::to_string(t));
    } else if (cmd == "sst") {
        int t = 0;

        if (!(iss >> t))
            return false;
        pushLog(context, "TICK FREQ " + std::to_string(t));
    } else if (cmd == "seg") {
        std::string team;

        iss >> team;
        pushLog(context, team.empty() ? "GAME OVER" : "WINNER " + team);
    } else if (cmd == "tna") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string team;

        if (!(iss >> team))
            return false;
        context.world->addTeam(team);
    } else if (cmd == "pnw") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string idToken;
        int id = -1;
        int x = 0;
        int y = 0;
        int orientation = 0;
        int level = 1;
        std::string team;

        if (!(iss >> idToken >> x >> y >> orientation >> level) || !parseProtocolId(idToken, id))
            return false;
        iss >> team;
        context.world->upsertPlayer(id, x, y, orientation, level, team);
        addAnimation(context, {GuiProtocolAnimationType::Spawn, static_cast<float>(x),
                               static_cast<float>(y), team, -1, true, "JOIN", id, 1.0f});
        pushLog(context, "PLAYER #" + std::to_string(id) + " JOINED");
        sendLine(context, "pin #" + std::to_string(id) + "\n");
    } else if (cmd == "ppo") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string idToken;
        int id = -1;
        int x = 0;
        int y = 0;
        int orientation = 0;

        if (!(iss >> idToken >> x >> y >> orientation) || !parseProtocolId(idToken, id))
            return false;
        const Player *previous = context.world->findPlayer(id);
        bool moved = previous && (previous->x != x || previous->y != y);
        std::string team = previous ? previous->teamName : "";
        context.world->updatePlayerPosition(id, x, y, orientation, context.time);
        if (moved)
            addAnimation(context, {GuiProtocolAnimationType::Move, static_cast<float>(x),
                                   static_cast<float>(y), team, -1, true, "", id, 0.28f});
    } else if (cmd == "plv") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string idToken;
        int id = -1;
        int level = 1;

        if (!(iss >> idToken >> level) || !parseProtocolId(idToken, id))
            return false;
        context.world->updatePlayerLevel(id, level);
        context.world->setPlayerEvent(id, "level " + std::to_string(level));
        if (const Player *player = context.world->findPlayer(id)) {
            addAnimation(context, {GuiProtocolAnimationType::LevelUp, static_cast<float>(player->x),
                                   static_cast<float>(player->y), player->teamName, -1, true,
                                   "L" + std::to_string(level), id, 1.1f});
        }
    } else if (cmd == "pin") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string idToken;
        int id = -1;
        int x = 0;
        int y = 0;
        ResourceCount inventory;

        if (!(iss >> idToken >> x >> y) || !parseProtocolId(idToken, id))
            return false;
        for (int i = 0; i < 7; ++i) {
            int count = 0;
            if (!(iss >> count))
                return false;
            if (count < 0)
                count = 0;
            inventory.set(static_cast<ResourceType>(i), static_cast<uint16_t>(count));
        }
        context.world->updatePlayerInventory(id, x, y, inventory);
    } else if (cmd == "pex") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string idToken;
        int id = -1;

        if (iss >> idToken && parseProtocolId(idToken, id)) {
            context.world->setPlayerEvent(id, "ejected");
            if (const Player *player = context.world->findPlayer(id)) {
                addAnimation(context, {GuiProtocolAnimationType::Eject, static_cast<float>(player->x),
                                       static_cast<float>(player->y), player->teamName, -1, true,
                                       "EJECT", id, 0.8f});
            }
            pushLog(context, "PLAYER #" + std::to_string(id) + " EJECTED");
        }
    } else if (cmd == "pbc") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string idToken;
        int id = -1;

        if (!(iss >> idToken) || !parseProtocolId(idToken, id))
            return false;
        std::string text = readRest(iss);
        context.world->setPlayerMessage(id, text);
        context.world->setPlayerEvent(id, "broadcast");
        if (const Player *player = context.world->findPlayer(id)) {
            addAnimation(context, {GuiProtocolAnimationType::Broadcast,
                                   static_cast<float>(player->x), static_cast<float>(player->y),
                                   player->teamName, -1, true, "MSG", id, 1.0f});
        }
        pushLog(context, "BROADCAST #" + std::to_string(id));
    } else if (cmd == "pic") {
        if (!context.mapReceived || !context.world)
            return false;
        int x = 0;
        int y = 0;
        int level = 0;
        std::vector<int> playerIds;

        if (!(iss >> x >> y >> level))
            return false;
        std::string idToken;
        while (iss >> idToken) {
            int id = -1;
            if (parseProtocolId(idToken, id))
                playerIds.push_back(id);
        }
        context.world->startIncantation(x, y, level, playerIds);
        addAnimation(context, {GuiProtocolAnimationType::IncantationStart, static_cast<float>(x),
                               static_cast<float>(y), "", -1, true, "RITUAL", -1, 1.2f});
        pushLog(context, "INCANTATION L" + std::to_string(level) + " START");
    } else if (cmd == "pie") {
        if (!context.mapReceived || !context.world)
            return false;
        int x = 0;
        int y = 0;
        int result = 0;

        if (!(iss >> x >> y >> result))
            return false;
        context.world->finishIncantation(x, y, result != 0);
        addAnimation(context, {GuiProtocolAnimationType::IncantationEnd, static_cast<float>(x),
                               static_cast<float>(y), "", -1, result != 0,
                               result != 0 ? "SUCCESS" : "FAIL", -1, 1.2f});
        pushLog(context, result != 0 ? "INCANTATION SUCCESS" : "INCANTATION FAILED");
    } else if (cmd == "pfk") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string idToken;
        int id = -1;

        if (iss >> idToken && parseProtocolId(idToken, id)) {
            context.world->setPlayerEvent(id, "fork");
            if (const Player *player = context.world->findPlayer(id)) {
                addAnimation(context, {GuiProtocolAnimationType::Fork, static_cast<float>(player->x),
                                       static_cast<float>(player->y), player->teamName, -1, true,
                                       "FORK", id, 0.9f});
            }
            pushLog(context, "PLAYER #" + std::to_string(id) + " FORK");
        }
    } else if (cmd == "pdr" || cmd == "pgt") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string idToken;
        int id = -1;
        int resource = -1;

        if (!(iss >> idToken >> resource) || !parseProtocolId(idToken, id) ||
            resource < 0 || resource >= 7)
            return false;
        int inventoryDelta = (cmd == "pgt") ? 1 : -1;
        int tileDelta = -inventoryDelta;
        context.world->adjustPlayerResource(id, static_cast<ResourceType>(resource), inventoryDelta);
        if (const Player *player = context.world->findPlayer(id)) {
            context.world->adjustTileResource(player->x, player->y,
                                             static_cast<ResourceType>(resource), tileDelta);
            addAnimation(context, {cmd == "pgt" ? GuiProtocolAnimationType::Pickup
                                                : GuiProtocolAnimationType::Drop,
                                   static_cast<float>(player->x), static_cast<float>(player->y),
                                   player->teamName, resource, true,
                                   cmd == "pgt" ? "+" : "-", id, 0.75f});
        }
        context.world->setPlayerEvent(id, std::string(cmd == "pgt" ? "took " : "dropped ") +
                                              resourceName(resource));
    } else if (cmd == "pdi") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string idToken;
        int id = -1;

        if (iss >> idToken && parseProtocolId(idToken, id)) {
            if (const Player *player = context.world->findPlayer(id)) {
                addAnimation(context, {GuiProtocolAnimationType::Death, static_cast<float>(player->x),
                                       static_cast<float>(player->y), player->teamName, -1, false,
                                       "DEAD", id, 1.0f});
            }
            context.world->removePlayer(id);
            pushLog(context, "PLAYER #" + std::to_string(id) + " DIED");
        }
    } else if (cmd == "enw") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string eggToken;
        std::string parentToken;
        int eggId = -1;
        int parentId = -1;
        int x = 0;
        int y = 0;

        if (!(iss >> eggToken >> parentToken >> x >> y) ||
            !parseProtocolId(eggToken, eggId) || !parseProtocolId(parentToken, parentId))
            return false;
        context.world->addEgg(eggId, parentId, x, y);
        addAnimation(context, {GuiProtocolAnimationType::EggLaid, static_cast<float>(x),
                               static_cast<float>(y), "", -1, true, "EGG", parentId, 0.9f});
        pushLog(context, "EGG #" + std::to_string(eggId) + " LAID");
    } else if (cmd == "ebo") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string eggToken;
        int eggId = -1;

        if (iss >> eggToken && parseProtocolId(eggToken, eggId)) {
            if (const auto &eggs = context.world->getEggs(); eggs.find(eggId) != eggs.end()) {
                const Egg &egg = eggs.at(eggId);
                addAnimation(context, {GuiProtocolAnimationType::EggHatch,
                                       static_cast<float>(egg.x), static_cast<float>(egg.y),
                                       egg.teamName, -1, true, "HATCH", egg.parentId, 1.0f});
            }
            context.world->removeEgg(eggId);
            pushLog(context, "EGG #" + std::to_string(eggId) + " HATCHED");
        }
    } else if (cmd == "edi") {
        if (!context.mapReceived || !context.world)
            return false;
        std::string eggToken;
        int eggId = -1;

        if (iss >> eggToken && parseProtocolId(eggToken, eggId)) {
            if (const auto &eggs = context.world->getEggs(); eggs.find(eggId) != eggs.end()) {
                const Egg &egg = eggs.at(eggId);
                addAnimation(context, {GuiProtocolAnimationType::EggDeath,
                                       static_cast<float>(egg.x), static_cast<float>(egg.y),
                                       egg.teamName, -1, false, "EGG", egg.parentId, 0.9f});
            }
            context.world->removeEgg(eggId);
            pushLog(context, "EGG #" + std::to_string(eggId) + " DIED");
        }
    } else if (cmd == "smg") {
        std::string text = readRest(iss);

        if (!text.empty())
            pushLog(context, "SERVER " + text);
    } else if (cmd == "suc") {
        pushLog(context, "SERVER UNKNOWN COMMAND");
    } else if (cmd == "sbp") {
        pushLog(context, "SERVER BAD PARAMETER");
    } else {
        return false;
    }
    return true;
}
