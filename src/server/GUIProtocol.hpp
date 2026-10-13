#pragma once

// ─────────────────────────────────────────────────────────────────────────────
//  guiProtocol.hpp  —  Zappy GUI server↔client protocol helpers
//
//  All message formatters (server→client) and command parsers (client→server)
//  for the graphical-client protocol are collected here.
//
//  Protocol reference
//  ──────────────────
//  Symbol table
//    X / Y   horizontal / vertical position or size
//    q0..q6  resource quantities  (0=food 1=linemate 2=deraumere 3=sibur
//                                  4=mendiane 5=phiras 6=thystame)
//    n       player number
//    O       orientation  1=N 2=E 3=S 4=W
//    L       player or incantation level
//    e       egg number
//    T       time unit
//    N       team name
//    R       incantation result  (0=fail 1=success)
//    M       broadcast message
//    i       resource index (0..6)
//
//  Server → Client (all send functions return a formatted std::string)
//    msz X Y              — map size
//    bct X Y q0..q6       — tile content
//    mct                  — all tiles (triggers one bct per tile)
//    tna N                — team name (one per team)
//    pnw #n X Y O L N     — new player connection
//    ppo #n X Y O         — player position
//    plv #n L             — player level
//    pin #n X Y q0..q6    — player inventory
//    pex #n               — player expulsion
//    pbc #n M             — player broadcast
//    pic X Y L #n…        — incantation start
//    pie X Y R            — incantation end
//    pfk #n               — egg laying
//    pdr #n i             — resource drop
//    pgt #n i             — resource pickup
//    pdi #n               — player death
//    enw #e #n X Y        — egg laid
//    ebo #e               — egg hatched (player connected for egg)
//    edi #e               — egg death
//    sgt T                — current time unit
//    sst T                — time unit changed
//    seg N                — end of game (winning team)
//    smg M                — server message
//    suc                  — unknown command
//    sbp                  — bad command parameters
//
//  Client → Server (parseGuiCommand dispatches these)
//    msz          — request map size
//    bct X Y      — request tile content
//    mct          — request all tiles
//    tna          — request all team names
//    ppo #n       — request player position
//    plv #n       — request player level
//    pin #n       — request player inventory
//    sgt          — request time unit
//    sst T        — set time unit
// ─────────────────────────────────────────────────────────────────────────────

#include <array>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Client.hpp"
#include "../map/World.hpp"
#include "../map/Drone.hpp"
#include "../map/Team.hpp"
#include "../map/Vector.hpp"

// ─── orientation encoding ────────────────────────────────────────────────────

/// Converts our internal Direction enum to the protocol orientation integer.
/// Protocol: 1=N 2=E 3=S 4=W  (cardinal directions only; diagonals map to
/// the nearest cardinal so the GUI always receives a valid value).
inline int direction_to_orientation(Direction d)
{
    switch (d) {
        case Direction::NORTH:      return 1;
        case Direction::NORTH_EAST: return 2;   // nearest: East
        case Direction::EAST:       return 2;
        case Direction::SOUTH_EAST: return 3;   // nearest: South
        case Direction::SOUTH:      return 3;
        case Direction::SOUTH_WEST: return 4;   // nearest: West
        case Direction::WEST:       return 4;
        case Direction::NORTH_WEST: return 1;   // nearest: North
        default:                    return 1;
    }
}

// ─── resource formatting helper ──────────────────────────────────────────────

/// Appends " q0 q1 … q6" to a stream from a raw int array (server-side Tile).
inline void append_resources(std::ostringstream& os,
                              const std::array<int, TOTAL_RESOURCES>& res)
{
    for (int i = 0; i < TOTAL_RESOURCES; ++i)
        os << ' ' << res[i];
}

// ═══════════════════════════════════════════════════════════════════════════════
//  GuiProtocol  —  stateless namespace with one function per protocol message
// ═══════════════════════════════════════════════════════════════════════════════

namespace GuiProtocol {

// ─── Server → Client senders ─────────────────────────────────────────────────

/// msz X Y\n  — map size
inline std::string send_msz(int width, int height)
{
    std::ostringstream os;
    os << "msz " << width << ' ' << height << '\n';
    return os.str();
}

/// bct X Y q0 q1 q2 q3 q4 q5 q6\n  — content of one tile
inline std::string send_bct(int x, int y,
                             const std::array<int, TOTAL_RESOURCES>& resources)
{
    std::ostringstream os;
    os << "bct " << x << ' ' << y;
    append_resources(os, resources);
    os << '\n';
    return os.str();
}

/// mct  — content of the whole map (returns a bct line per tile, concatenated)
inline std::string send_mct(const World& world)
{
    const Vector2i size = world.getSize();
    std::ostringstream os;
    for (int y = 0; y < size.y; ++y)
        for (int x = 0; x < size.x; ++x)
            os << send_bct(x, y, world[{x, y}].getResources());
    return os.str();
}

/// tna N\n  — one team name (call once per team)
inline std::string send_tna(const std::string& team_name)
{
    return "tna " + team_name + '\n';
}

/// pnw #n X Y O L N\n  — new player connected
inline std::string send_pnw(int player_id, int x, int y,
                             Direction dir, int level,
                             const std::string& team_name)
{
    std::ostringstream os;
    os << "pnw #" << player_id
       << ' ' << x << ' ' << y
       << ' ' << direction_to_orientation(dir)
       << ' ' << level
       << ' ' << team_name
       << '\n';
    return os.str();
}

/// ppo #n X Y O\n  — player position and orientation
inline std::string send_ppo(int player_id, int x, int y, Direction dir)
{
    std::ostringstream os;
    os << "ppo #" << player_id
       << ' ' << x << ' ' << y
       << ' ' << direction_to_orientation(dir)
       << '\n';
    return os.str();
}

/// plv #n L\n  — player level
inline std::string send_plv(int player_id, int level)
{
    std::ostringstream os;
    os << "plv #" << player_id << ' ' << level << '\n';
    return os.str();
}

/// pin #n X Y q0 q1 q2 q3 q4 q5 q6\n  — player inventory
inline std::string send_pin(int player_id, int x, int y,
                             const std::array<int, TOTAL_RESOURCES>& inventory)
{
    std::ostringstream os;
    os << "pin #" << player_id << ' ' << x << ' ' << y;
    append_resources(os, inventory);
    os << '\n';
    return os.str();
}

/// pex #n\n  — player expulsion (eject)
inline std::string send_pex(int player_id)
{
    return "pex #" + std::to_string(player_id) + '\n';
}

/// pbc #n M\n  — player broadcast message
inline std::string send_pbc(int player_id, const std::string& message)
{
    return "pbc #" + std::to_string(player_id) + ' ' + message + '\n';
}

/// pic X Y L #n …\n  — incantation start (first player + all participants)
inline std::string send_pic(int x, int y, int level,
                             const std::vector<int>& player_ids)
{
    std::ostringstream os;
    os << "pic " << x << ' ' << y << ' ' << level;
    for (int id : player_ids)
        os << " #" << id;
    os << '\n';
    return os.str();
}

/// pie X Y R\n  — incantation end  (R: 0=fail, 1=success)
inline std::string send_pie(int x, int y, bool success)
{
    std::ostringstream os;
    os << "pie " << x << ' ' << y << ' ' << (success ? 1 : 0) << '\n';
    return os.str();
}

/// pfk #n\n  — egg laying by player
inline std::string send_pfk(int player_id)
{
    return "pfk #" + std::to_string(player_id) + '\n';
}

/// pdr #n i\n  — player dropped resource i
inline std::string send_pdr(int player_id, int resource_index)
{
    std::ostringstream os;
    os << "pdr #" << player_id << ' ' << resource_index << '\n';
    return os.str();
}

/// pgt #n i\n  — player picked up resource i
inline std::string send_pgt(int player_id, int resource_index)
{
    std::ostringstream os;
    os << "pgt #" << player_id << ' ' << resource_index << '\n';
    return os.str();
}

/// pdi #n\n  — player death
inline std::string send_pdi(int player_id)
{
    return "pdi #" + std::to_string(player_id) + '\n';
}

/// enw #e #n X Y\n  — egg laid by player at position
inline std::string send_enw(int egg_id, int player_id, int x, int y)
{
    std::ostringstream os;
    os << "enw #" << egg_id << " #" << player_id
       << ' ' << x << ' ' << y << '\n';
    return os.str();
}

/// ebo #e\n  — a player connected through egg (egg hatched)
inline std::string send_ebo(int egg_id)
{
    return "ebo #" + std::to_string(egg_id) + '\n';
}

/// edi #e\n  — egg death (expired without hatching)
inline std::string send_edi(int egg_id)
{
    return "edi #" + std::to_string(egg_id) + '\n';
}

/// sgt T\n  — current time unit (response to sgt request)
inline std::string send_sgt(int time_unit)
{
    return "sgt " + std::to_string(time_unit) + '\n';
}

/// sst T\n  — time unit after modification (response to sst command)
inline std::string send_sst(int time_unit)
{
    return "sst " + std::to_string(time_unit) + '\n';
}

/// seg N\n  — end of game, N is the winning team name
inline std::string send_seg(const std::string& team_name)
{
    return "seg " + team_name + '\n';
}

/// smg M\n  — arbitrary server message to the GUI
inline std::string send_smg(const std::string& message)
{
    return "smg " + message + '\n';
}

/// suc\n  — unknown command received from GUI client
inline std::string send_suc()
{
    return "suc\n";
}

/// sbp\n  — bad parameters in the GUI client command
inline std::string send_sbp()
{
    return "sbp\n";
}

// ─── Client → Server parser ───────────────────────────────────────────────────

/// Tag for each recognised GUI command (returned by parseGuiCommand).
enum class GuiCmd {
    MSZ,       ///< msz          — request map size
    BCT,       ///< bct X Y      — request tile content
    MCT,       ///< mct          — request all tiles
    TNA,       ///< tna          — request all team names
    PPO,       ///< ppo #n       — request player position
    PLV,       ///< plv #n       — request player level
    PIN,       ///< pin #n       — request player inventory
    SGT,       ///< sgt          — request time unit
    SST,       ///< sst T        — set time unit
    UNKNOWN,   ///< unrecognised command  (server must reply suc\n)
    BAD_PARAM, ///< recognised command but bad arguments (server must reply sbp\n)
};

/// Parsed representation of a GUI client command.
struct GuiRequest {
    GuiCmd cmd = GuiCmd::UNKNOWN;

    // BCT / PPO / PLV / PIN
    int x          = -1; ///< X coordinate (BCT) or ignored
    int y          = -1; ///< Y coordinate (BCT) or ignored
    int player_id  = -1; ///< Player number  (PPO / PLV / PIN), stripped of '#'
    int time_unit  = -1; ///< Time unit value (SST)
};

/// Parses a single line received from a GUI client into a GuiRequest.
/// The caller should check cmd == UNKNOWN → reply suc, BAD_PARAM → reply sbp.
inline GuiRequest parseGuiCommand(const std::string& line)
{
    GuiRequest req;
    std::istringstream iss(line);
    std::string token;

    if (!(iss >> token)) {
        req.cmd = GuiCmd::UNKNOWN;
        return req;
    }

    // ── zero-argument commands ──────────────────────────────────────────────
    if (token == "msz") { req.cmd = GuiCmd::MSZ; return req; }
    if (token == "mct") { req.cmd = GuiCmd::MCT; return req; }
    if (token == "tna") { req.cmd = GuiCmd::TNA; return req; }
    if (token == "sgt") { req.cmd = GuiCmd::SGT; return req; }

    // ── bct X Y ────────────────────────────────────────────────────────────
    if (token == "bct") {
        req.cmd = GuiCmd::BCT;
        if (!(iss >> req.x >> req.y)) { req.cmd = GuiCmd::BAD_PARAM; }
        if (req.x < 0 || req.y < 0) { req.cmd = GuiCmd::BAD_PARAM; }
        return req;
    }

    // ── ppo #n ─────────────────────────────────────────────────────────────
    if (token == "ppo") {
        req.cmd = GuiCmd::PPO;
        std::string id_tok;
        if (!(iss >> id_tok)) { req.cmd = GuiCmd::BAD_PARAM; return req; }
        try {
            req.player_id = std::stoi(id_tok.front() == '#' ? id_tok.substr(1) : id_tok);
        } catch (...) { req.cmd = GuiCmd::BAD_PARAM; }
        return req;
    }

    // ── plv #n ─────────────────────────────────────────────────────────────
    if (token == "plv") {
        req.cmd = GuiCmd::PLV;
        std::string id_tok;
        if (!(iss >> id_tok)) { req.cmd = GuiCmd::BAD_PARAM; return req; }
        try {
            req.player_id = std::stoi(id_tok.front() == '#' ? id_tok.substr(1) : id_tok);
        } catch (...) { req.cmd = GuiCmd::BAD_PARAM; }
        return req;
    }

    // ── pin #n ─────────────────────────────────────────────────────────────
    if (token == "pin") {
        req.cmd = GuiCmd::PIN;
        std::string id_tok;
        if (!(iss >> id_tok)) { req.cmd = GuiCmd::BAD_PARAM; return req; }
        try {
            req.player_id = std::stoi(id_tok.front() == '#' ? id_tok.substr(1) : id_tok);
        } catch (...) { req.cmd = GuiCmd::BAD_PARAM; }
        return req;
    }

    // ── sst T ──────────────────────────────────────────────────────────────
    if (token == "sst") {
        req.cmd = GuiCmd::SST;
        if (!(iss >> req.time_unit) || req.time_unit < 1 || req.time_unit > 10000) {
            req.cmd = GuiCmd::BAD_PARAM;
        }
        return req;
    }

    req.cmd = GuiCmd::UNKNOWN;
    return req;
}

// ─── High-level dispatch helper ───────────────────────────────────────────────

/// Handles a complete GUI request end-to-end: parses the raw line, executes the
/// corresponding action via the supplied callbacks, and queues the formatted
/// reply(ies) into client.write_buf via client.send().
///
/// Parameters
/// ──────────
///  client       — the GUI Client whose write_buf should receive the reply
///  line         — raw command line received from that client (no trailing \\n)
///  world        — the live game World
///  freq         — current server frequency (time unit)
///  set_freq_cb  — called with the new T value when the client sends sst T;
///                 the server updates its own frequency and calls send_sgt back
///
/// Returns true if the command was successfully handled, false on UNKNOWN/BAD_PARAM.
inline bool handleGuiCommand(
    Client&      client,
    const std::string& line,
    World&       world,
    int          freq,
    const std::function<void(int)>& set_freq_cb)
{
    const GuiRequest req = parseGuiCommand(line);

    switch (req.cmd) {

        // ── msz ────────────────────────────────────────────────────────────
        case GuiCmd::MSZ: {
            const auto sz = world.getSize();
            client.send(send_msz(sz.x, sz.y));
            return true;
        }

        // ── bct X Y ────────────────────────────────────────────────────────
        case GuiCmd::BCT: {
            const auto sz = world.getSize();
            if (req.x < 0 || req.x >= sz.x || req.y < 0 || req.y >= sz.y) {
                client.send(send_sbp());
                return false;
            }
            client.send(send_bct(req.x, req.y,
                                 world[{req.x, req.y}].getResources()));
            return true;
        }

        // ── mct ────────────────────────────────────────────────────────────
        case GuiCmd::MCT: {
            client.send(send_mct(world));
            return true;
        }

        // ── tna ────────────────────────────────────────────────────────────
        case GuiCmd::TNA: {
            for (const auto& name : world.getTeamNames())
                client.send(send_tna(name));
            return true;
        }

        // ── ppo #n ─────────────────────────────────────────────────────────
        case GuiCmd::PPO: {
            const Drone* drone = world.getDrone(req.player_id);
            if (!drone) { client.send(send_sbp()); return false; }
            const auto pos = drone->getPos();
            client.send(send_ppo(req.player_id, pos.x, pos.y,
                                 drone->getDir()));
            return true;
        }

        // ── plv #n ─────────────────────────────────────────────────────────
        case GuiCmd::PLV: {
            const Drone* drone = world.getDrone(req.player_id);
            if (!drone) { client.send(send_sbp()); return false; }
            client.send(send_plv(req.player_id, drone->getLevel()));
            return true;
        }

        // ── pin #n ─────────────────────────────────────────────────────────
        case GuiCmd::PIN: {
            const Drone* drone = world.getDrone(req.player_id);
            if (!drone) { client.send(send_sbp()); return false; }
            const auto pos = drone->getPos();
            client.send(send_pin(req.player_id, pos.x, pos.y,
                                 drone->inventory()));
            return true;
        }

        // ── sgt ────────────────────────────────────────────────────────────
        case GuiCmd::SGT: {
            client.send(send_sgt(freq));
            return true;
        }

        // ── sst T ──────────────────────────────────────────────────────────
        case GuiCmd::SST: {
            set_freq_cb(req.time_unit);
            client.send(send_sst(req.time_unit));
            return true;
        }

        // ── error responses ────────────────────────────────────────────────
        case GuiCmd::BAD_PARAM:
            client.send(send_sbp());
            return false;

        case GuiCmd::UNKNOWN:
        default:
            client.send(send_suc());
            return false;
    }
}

/// Broadcasts a server-generated string to every connected GUI client.
/// Pass a reference to the client vector held by Server.
inline void broadcastToGui(std::vector<Client>& clients,
                            const std::string&   message)
{
    for (auto& c : clients)
        if (c.is_gui())
            c.send(message);
}

} // namespace GuiProtocol
