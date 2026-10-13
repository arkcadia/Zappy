#include "Bodyguard.hpp"

void Bodyguard::tick(Agent &agent)
{
    switch (_state) {
    case State::NAVIGATE: _doNavigate(agent); break;
    case State::GUARD:    _doGuard(agent);    break;
    }
}

std::pair<int,int> Bodyguard::_postCoords(const Agent &agent) const
{
    switch (_claimedPos) {
    case 0: return {1,              0              };
    case 1: return {agent.mapX()-1, 0              };
    case 2: return {0,              1              };
    case 3: return {0,              agent.mapY()-1 };
    default: return {0, 0};
    }
}

Dir Bodyguard::_outwardDir() const
{
    switch (_claimedPos) {
    case 0: return Dir::East;
    case 1: return Dir::West;
    case 2: return Dir::South;
    case 3: return Dir::North;
    default: return Dir::North;
    }
}
