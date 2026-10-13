#include "Bodyguard.hpp"
#include "../../core/Agent.hpp"
#include "../../../debug.hpp"

void Bodyguard::_doNavigate(Agent &agent)
{
    if (!_calibrated) {
        if (_beaconCalibrate(agent)) {
            debug_logf("[Agent {}] Bodyguard: calibrated at (0,0)", agent.ID());
            _calibrated = true;
            _sendAndRead(agent, "Fork");
            debug_logf("[Agent {}] Bodyguard: forked before heading to post", agent.ID());
        }
        return;
    }

    auto [tx, ty] = _postCoords(agent);
    if (_navigateTo(agent, tx, ty)) {
        debug_logf("[Agent {}] Bodyguard: reached post {} at ({},{})", agent.ID(), _claimedPos, tx, ty);
        _state = State::GUARD;
    }
}
