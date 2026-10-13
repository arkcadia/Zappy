#pragma once

#include <format>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <poll.h>

#include "Client.hpp"
#include "../map/World.hpp"

class Server {
public:
    Server(const std::vector<std::string>& args);
    ~Server();

    void createWorld();
    void run();

    bool done = false;
    std::string winning_team = {};

private:
    void init_network();
    void handle_new_connection();
    void handle_client_io();
    void process_client_commands();
    void rebuild_poll_fds();
    void broadcast(int sender_fd, const std::string& text);
    void notifyGui(const std::string& msg);
    void syncGuiState();

    unsigned short port = 0;
    int listen_fd = -1;
    Vector2i size;
    std::vector<std::string> team_names;
    int starting_slots = 0;
    int frequence = 100;

    int next_egg_id = 0;

    std::vector<Client> clients;
    std::vector<pollfd> poll_fds_list;

    // Last position/orientation broadcast to GUI clients for each drone, so
    // syncGuiState() only sends a ppo when something actually changed.
    std::unordered_map<int, std::pair<Point, Direction>> last_drone_state;

    World *world = nullptr;

    friend struct std::formatter<Server>;
};

template <>
struct std::formatter<Server> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const Server& s, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(),
            "[Server] Port: {} Size: {} Teams: {} Slots: {} Freq: {}",
            s.port, s.size, s.team_names.size(), s.starting_slots, s.frequence);
    }
};

void display_help();
