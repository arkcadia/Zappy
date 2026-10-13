#include "Bodyguard.hpp"
#include "../../../debug.hpp"

void Bodyguard::_doGuard(Agent &agent)
{
    while (agent.pollBroadcast()) {}

    Dir target = _outwardDir();
    int diff = (static_cast<int>(target) - static_cast<int>(agent.dir()) + 4) % 4;
    if (diff == 1)      _turnRight(agent);
    else if (diff == 2) _turnAround(agent);
    else if (diff == 3) _turnLeft(agent);

    auto tiles = _parseLook(_sendAndRead(agent, "Look"));
    if (!tiles.empty()) {
        int playerCount = 0;
        for (const auto &item : tiles[0])
            if (item == "player") ++playerCount;

        if (playerCount > 1)
            debug_logf("[Agent {}] Bodyguard: {} players on tile (watching)", agent.ID(), playerCount);
    }
}
