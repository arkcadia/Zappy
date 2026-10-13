#include "Server.hpp"
#include "AIProtocol.hpp"
#include "GUIProtocol.hpp"

#include <exception>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

void display_help()
{
    std::cout << "Usage: ./zappy_server -p port -x width -y height -n name1 name2... -c clientsNb -f freq\n"
              << "  -p port     Listening port (1-65535)\n"
              << "  -x width    World width \n"
              << "  -y height   World height\n"
              << "  -n names    Team names (at least one, space-separated)\n"
              << "  -c clients  Max clients per team\n"
              << "  -f freq     Game frequency in Hz (default: 100)"
              << std::endl;
}

Server::Server(const std::vector<std::string>& args)
{
    std::set<std::string> seen_flags;

    auto check_duplicate = [&](const std::string& flag) {
        if (!seen_flags.insert(flag).second)
            throw std::invalid_argument("Duplicate flag: " + flag);
    };

    for (size_t i = 0; i < args.size(); i++) {
        const std::string arg = args[i];
        if (arg == "-h" || arg == "--help") {
            check_duplicate("help");
            display_help();
            return;
        }
        else if ((arg == "-p" || arg == "--port") && i + 1 < args.size()) {
            check_duplicate("port");
            port = static_cast<unsigned short>(std::stoi(args[++i]));
            if (port <= 0)
                throw std::invalid_argument(arg);
        }
        else if ((arg == "-x" || arg == "--width") && i + 1 < args.size()) {
            check_duplicate("width");
            size.x = std::stoi(args[++i]);
            if (size.x == 0)
                throw std::invalid_argument(arg);
        }
        else if ((arg == "-y" || arg == "--height") && i + 1 < args.size()) {
            check_duplicate("height");
            size.y = std::stoi(args[++i]);
            if (size.y == 0)
                throw std::invalid_argument(arg);
        }
        else if ((arg == "-c" || arg == "--clients") && i + 1 < args.size()) {
            check_duplicate("clients");
            starting_slots = std::stoi(args[++i]);
            if (starting_slots == 0)
                throw std::invalid_argument(arg);
        }
        else if ((arg == "-f" || arg == "--freq") && i + 1 < args.size()) {
            check_duplicate("freq");
            frequence = std::stoi(args[++i]);
            if (frequence == 0)
                throw std::invalid_argument(arg);
        }
        else if ((arg == "-n" || arg == "--names") && i + 1 < args.size()) {
            check_duplicate("names");
            for (; i + 1 < args.size() && args[i + 1][0] != '-'; i++) {
                for (const auto& existing_name:team_names)
                    if (existing_name == args[i + 1])
                        throw std::invalid_argument(arg);
                team_names.push_back(args[i + 1]);
            }
        }
        else {
            throw std::invalid_argument(arg);
        }
    }

    static const std::vector<std::string> required = {
        "port", "width", "height", "clients", "names"
    };

    for (const auto& flag : required) {
        if (seen_flags.find(flag) == seen_flags.end())
            throw std::invalid_argument("Missing required flag: --" + flag);
    }

    done = true;
}

Server::~Server()
{
    delete world;
}

void Server::createWorld()
{
    world = new World(size, frequence);

    for (const auto& team : team_names)
        world->createTeam(team, starting_slots);
}

void Server::init_network()
{
    listen_fd = sys::socket_tcp();
    sys::setsockopt_reuseaddr(listen_fd);
    sys::bind_tcp(listen_fd, port);
    sys::listen_tcp(listen_fd);
    sys::set_nonblocking(listen_fd);

    debug_logf("[Server] Listening on port {}", port);
}

void Server::handle_new_connection()
{
    static constexpr std::size_t MAX_CLIENTS = 1024;
    if (clients.size() >= MAX_CLIENTS) {
        int tmp = sys::accept_tcp(listen_fd);
        if (tmp >= 0) sys::close_fd(tmp);
        return;
    }

    int cfd = sys::accept_tcp(listen_fd);
    if (cfd < 0) return;

    sys::set_nonblocking(cfd);
    Client client(cfd);

    client.send("WELCOME");
    clients.push_back(client);

    debug_logf("[Server] New client connected on fd {}", cfd);
}

void Server::handle_client_io()
{
    for (size_t i = 0; i < poll_fds_list.size(); i++) {
        if (poll_fds_list[i].fd == listen_fd) continue;

        int fd = poll_fds_list[i].fd;
        auto it = std::find_if(clients.begin(), clients.end(), [fd](const Client& c) { return c.fd == fd; });
        if (it == clients.end()) continue;

        if (poll_fds_list[i].revents & POLLIN) {
            if (!it->do_read()) {
                it->dead = true;
                continue;
            }

            while (auto line = it->next_line()) {
                if (*line == "GRAPHIC") {
                    it->type = ClientType::GUI;
                    it->handshake = HandshakeState::DONE;

                    it->send(GuiProtocol::send_msz(size.x, size.y));

                    for (auto& c : clients) {
                        if (!c.is_ai())
                            continue;
                        auto drone = world->getDrone(c.drone_id);
                        it->send(GuiProtocol::send_pnw(
                            drone->id, drone->getPos().x,
                            drone->getPos().y, drone->getDir(),
                            drone->getLevel(), c.team_name)
                        );
                    }

                    // Prime this newly-connected GUI with the full map once;
                    // later changes are streamed incrementally by syncGuiState().
                    for (int x = 0; x < size.x; x++) {
                        for (int y = 0; y < size.y; y++) {
                            const Point p(x, y);
                            it->send(GuiProtocol::send_bct(x, y, (*world)[p].getResources()));
                        }
                    }

                    debug_logf("[Server] Graphical client authenticated on fd {}", it->fd);
                }
                else if (it->handshake == HandshakeState::WAIT_TEAM_NAME) {
                    Team* team = world->getTeam(*line);
                    if (!team || team->slots() <= 0) {
                        it->send("ko");
                        it->dead = true;
                    } else {
                        Drone* new_drone = team->createDrone();
                        if (new_drone) {
                            it->team_name = *line;
                            it->type = ClientType::AI;
                            it->handshake = HandshakeState::DONE;
                            it->drone_id = new_drone->id;

                            it->send(std::to_string(team->slots()));
                            it->send(std::to_string(size.x) + " " + std::to_string(size.y));

                            int eid = next_egg_id - 1;
                            if (eid >= 0) {
                                it->egg_id = eid;
                                notifyGui(GuiProtocol::send_ebo(eid));
                            }

                            notifyGui(GuiProtocol::send_pnw(
                                new_drone->id, new_drone->getPos().x, new_drone->getPos().y,
                                new_drone->getDir(), new_drone->getLevel(), it->team_name
                            ));
                            last_drone_state[new_drone->id] = {new_drone->getPos(), new_drone->getDir()};
                        } else {
                            it->send("ko");
                            it->dead = true;
                        }
                    }
                } else if (it->is_gui() && it->handshake == HandshakeState::DONE) {
                    GuiProtocol::handleGuiCommand(*it, *line, *world, frequence,
                        [&](int t) { frequence = t; });
                } else if (it->cmd_queue.size() < MAX_CMD_QUEUE) {
                    it->cmd_queue.push_back(*line);
                }
            }
        }

        if (poll_fds_list[i].revents & POLLOUT) {
            if (it->has_pending_write()) {
                if (!it->do_write()) {
                    it->dead = true;
                }
            }
        }
    }
}

void Server::syncGuiState()
{
    for (int x = 0; x < size.x; x++) {
        for (int y = 0; y < size.y; y++) {
            Tile& tile = (*world)[Point(x, y)];
            if (!tile.isDirty())
                continue;
            notifyGui(GuiProtocol::send_bct(x, y, tile.getResources()));
            tile.clearDirty();
        }
    }

    for (const auto& [id, drone] : world->getDrones()) {
        std::pair<Point, Direction> state{drone->getPos(), drone->getDir()};
        auto it = last_drone_state.find(id);

        if (it != last_drone_state.end() && it->second.first == state.first
            && it->second.second == state.second)
            continue;

        notifyGui(GuiProtocol::send_ppo(id, state.first.x, state.first.y, state.second));
        last_drone_state[id] = state;
    }
}

void Server::process_client_commands()
{
    long long current_tick = world->getTickCount();

    for (auto& client : clients) {
        if (client.dead || client.handshake != HandshakeState::DONE || client.cmd_queue.empty())
            continue;

        Drone* drone = world->getDrone(client.drone_id);
        if (drone == nullptr) {
            client.dead = true;
            continue;
        }

        if (client.cmd_finish_tick == -1) {
            std::string current_cmd = client.cmd_queue.front();

            if (current_cmd == "Incantation") {
                if (!drone->canIncantate(*world)) {
                    client.cmd_queue.pop_front();
                    client.send("ko");
                    continue;
                }
                client.send("Elevation underway");
                client.incanting = true;
                Point pos = drone->getPos();
                auto ids = world->operator[](pos).getDroneIds();
                notifyGui(GuiProtocol::send_pic(pos.x, pos.y, drone->getLevel(),
                          std::vector<int>(ids.begin(), ids.end())));
            }

            client.cmd_finish_tick = current_tick + ai_cmd_cost(current_cmd);
        }

        if (current_tick >= client.cmd_finish_tick) {
            std::string cmd = client.cmd_queue.front();
            client.cmd_queue.pop_front();
            client.cmd_finish_tick = -1;

            if (cmd == "Incantation") {
                client.incanting = false;

                std::vector<int> incanting_ids;
                for (auto& c : clients)
                    if (c.is_ai() && !c.dead && c.incanting)
                        incanting_ids.push_back(c.drone_id);
                incanting_ids.push_back(client.drone_id);

                Point pos = drone->getPos();
                int newLevel = drone->startIncantation(*world, incanting_ids);
                if (newLevel == 8)
                    winning_team = client.team_name;
                if (newLevel > 0) {
                    std::string msg = "Current level: " + std::to_string(newLevel);
                    client.send(msg);
                    notifyGui(GuiProtocol::send_plv(client.drone_id, newLevel));

                    for (auto& c : clients) {
                        if (&c == &client || c.dead || !c.is_ai())
                            continue;
                        Drone* d = world->getDrone(c.drone_id);
                        if (d && c.incanting
                            && d->getPos().x == pos.x && d->getPos().y == pos.y
                            && d->getLevel() == newLevel) {
                            c.send(msg);
                            c.incanting = false;
                            c.cmd_queue.pop_front();
                            c.cmd_finish_tick = -1;
                            notifyGui(GuiProtocol::send_plv(c.drone_id, newLevel));
                        }
                    }
                    notifyGui(GuiProtocol::send_pie(pos.x, pos.y, true));
                } else {
                    notifyGui(GuiProtocol::send_pie(pos.x, pos.y, false));
                    client.send("ko");
                    for (auto& c : clients) {
                        if (&c == &client || c.dead || !c.is_ai())
                            continue;
                        Drone* d = world->getDrone(c.drone_id);
                        if (d && c.incanting
                            && d->getPos().x == pos.x && d->getPos().y == pos.y) {
                            c.send("ko");
                            c.incanting = false;
                            c.cmd_queue.pop_front();
                            c.cmd_finish_tick = -1;
                        }
                    }
                }
            } else if (cmd == "Eject") {
                auto ejected = drone->eject(*world);
                if (ejected.empty()) {
                    client.send("ko");
                } else {
                    int ejectDir = (static_cast<int>(drone->getDir()) - 1 + 4) % 8 + 1;
                    for (int did : ejected) {
                        for (auto& c : clients) {
                            if (c.drone_id == did && c.is_ai() && !c.dead)
                                c.send("eject " + std::to_string(ejectDir));
                        }
                    }
                    client.send("ok");
                }
            } else {
                AiProtocol ai_proto(
                    [&](int sender_fd, const std::string& text) {
                        broadcast(sender_fd, text);
                    },
                    [&](Client&) {}
                );
                client.send(ai_proto.execute(client, cmd, *world, drone));
            }
        }
    }
}

static int broadcast_direction(const Drone& receiver, const Drone& sender, Point map_size)
{
    Point rp = receiver.getPos();
    Point sp = sender.getPos();

    // Shortest-path delta on toroidal map
    int dx = sp.x - rp.x;
    int dy = sp.y - rp.y;
    if (std::abs(dx * 2) > map_size.x) dx -= (dx > 0 ? map_size.x : -map_size.x);
    if (std::abs(dy * 2) > map_size.y) dy -= (dy > 0 ? map_size.y : -map_size.y);

    if (dx == 0 && dy == 0) return 0;

    // Rotate (dx,dy) into receiver's local frame
    // Local frame: forward = receiver's facing direction, right = 90° CW
    int lx, ly;
    switch (receiver.getDir()) {
        case Direction::NORTH: lx =  dx; ly =  dy; break;
        case Direction::EAST:  lx =  dy; ly = -dx; break;
        case Direction::SOUTH: lx = -dx; ly = -dy; break;
        case Direction::WEST:  lx = -dy; ly =  dx; break;
        default:               lx =  dx; ly =  dy; break;
    }

    // Map sign(lx), sign(ly) → direction k (1-8)
    // Grid (local frame, ly negative = forward):
    //  1(NW) 2(N) 3(NE)
    //  8(W)   0   4(E)   -- using dir numbering from spec: left=8, right=4
    //  7(SW) 6(S) 5(SE)
    int sx = (lx > 0) - (lx < 0);
    int sy = (ly > 0) - (ly < 0);

    if (sx ==  0 && sy == -1) return 2; // N
    if (sx ==  1 && sy == -1) return 3; // NE
    if (sx ==  1 && sy ==  0) return 4; // E
    if (sx ==  1 && sy ==  1) return 5; // SE
    if (sx ==  0 && sy ==  1) return 6; // S
    if (sx == -1 && sy ==  1) return 7; // SW
    if (sx == -1 && sy ==  0) return 8; // W
    if (sx == -1 && sy == -1) return 1; // NW
    return 0;
}

void Server::broadcast(int sender_fd, const std::string& text)
{
    Drone* sender_drone = nullptr;
    for (const auto& c : clients) {
        if (c.fd == sender_fd) {
            sender_drone = world->getDrone(c.drone_id);
            break;
        }
    }

    Point map_size = world->getSize();

    for (auto& c : clients) {
        if (c.type != ClientType::AI || c.handshake != HandshakeState::DONE)
            continue;
        if (c.fd == sender_fd)
            continue;

        int k = 0;
        if (sender_drone) {
            Drone* recv_drone = world->getDrone(c.drone_id);
            if (recv_drone)
                k = broadcast_direction(*recv_drone, *sender_drone, map_size);
        }
        c.send("message " + std::to_string(k) + ", " + text);
    }
}

void Server::notifyGui(const std::string& msg)
{
    for (auto& c : clients)
        if (c.is_gui() && c.handshake == HandshakeState::DONE)
            c.send(msg);
}

void Server::rebuild_poll_fds()
{
    clients.erase(std::remove_if(clients.begin(), clients.end(), [this](const Client& c) {
        if (c.dead) {
            if (c.is_ai() && c.drone_id != -1)
                world->killDrone(c.drone_id);
            sys::close_fd(c.fd);
            return true;
        }
        return false;
    }), clients.end());

    poll_fds_list.clear();

    poll_fds_list.push_back({listen_fd, POLLIN, 0});

    for (const auto& client : clients) {
        short events = POLLIN;
        if (client.has_pending_write()) {
            events |= POLLOUT;
        }
        poll_fds_list.push_back({client.fd, events, 0});
    }
}

void Server::run()
{
    init_network();

    try {

    while (winning_team.empty()) {
        rebuild_poll_fds();

        int active_fds = sys::poll_fds(poll_fds_list, 0);

        if (active_fds > 0) {
            if (poll_fds_list[0].revents & POLLIN) {
                handle_new_connection();
            }
            handle_client_io();
        }

        process_client_commands();

        world->tick();
        syncGuiState();

        for (int dead_id : world->getAndClearRecentlyDied()) {
            last_drone_state.erase(dead_id);
            for (auto& c : clients) {
                if (c.is_ai() && c.drone_id == dead_id && !c.dead) {
                    c.send("dead");
                    c.dead = true;
                    break;
                }
            }
            for (auto& c : clients) {
                if (c.is_gui() && c.handshake == HandshakeState::DONE)
                    c.send(GuiProtocol::send_pdi(dead_id));
            }
        }
        if (!winning_team.empty()) {
            for (auto& c : clients)
                if (c.is_gui())
                    c.send(GuiProtocol::send_seg(winning_team));
        }
    }
    } catch (...) {
        throw std::runtime_error("Something happened while running.");
    }

    std::cout << "Team " << winning_team << " won!" << std::endl;
}
