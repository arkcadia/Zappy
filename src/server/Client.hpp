#pragma once

#include <deque>
#include <string>
#include <optional>

#include "Syscall.hpp"
#include "../debug.hpp"

static constexpr int MAX_CMD_QUEUE = 10;

static constexpr std::size_t MAX_READ_BUF = 65536;

enum class ClientType {
    UNKNOWN,
    AI,
    GUI,
};

enum class HandshakeState {
    WAIT_TEAM_NAME,
    DONE,
};

struct Client {
    int              fd           = -1;
    ClientType       type         = ClientType::UNKNOWN;
    HandshakeState   handshake    = HandshakeState::WAIT_TEAM_NAME;
    bool             dead         = false;

    std::string      team_name;
    int              drone_id     = -1;
    int              player_id    = -1;
    int              egg_id       = -1;

    std::string      read_buf;
    std::string      write_buf;

    std::deque<std::string> cmd_queue;

    long long        cmd_finish_tick = -1;
    bool             incanting       = false;

    explicit Client(int fd) : fd(fd) {}

    bool do_read()
    {
        char buf[4096];
        while (true) {
            ssize_t n = sys::read_fd(fd, buf, sizeof(buf));
            if (n == 0) return false;   // EOF
            if (n <  0) break;          // EAGAIN
            if (read_buf.size() + static_cast<std::size_t>(n) > MAX_READ_BUF) {
                debug_logf("[Client {}] read buffer overflow, closing", fd);
                return false;
            }
            read_buf.append(buf, static_cast<std::size_t>(n));
        }

        verbose_logf("[Client {}] read ->{}<-", fd, read_buf);

        return true;
    }

    bool do_write()
    {
        verbose_logf("[Client {}] wrote ->{}<-", fd, write_buf);

        while (!write_buf.empty()) {
            ssize_t n = sys::write_fd(fd, write_buf.data(), write_buf.size());
            if (n < 0) break;   // EAGAIN
            if (n == 0) return false;
            write_buf.erase(0, static_cast<std::size_t>(n));
        }
        return true;
    }

    void send(const std::string& line)
    {
        static constexpr std::size_t MAX_WRITE_BUF = 1 << 20; // 1 MB
        if (write_buf.size() > MAX_WRITE_BUF) {
            dead = true;   // slow client, disconnect
            return;
        }
        write_buf += line;
        if (write_buf.empty() || write_buf.back() != '\n')
            write_buf += '\n';
    }

    std::optional<std::string> next_line()
    {
        std::size_t pos = read_buf.find('\n');
        if (pos == std::string::npos)
            return std::nullopt;

        std::string line = read_buf.substr(0, pos);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        read_buf.erase(0, pos + 1);
        return line;
    }

    bool has_pending_write() const { return !write_buf.empty(); }
    bool is_ai()  const { return type == ClientType::AI;  }
    bool is_gui() const { return type == ClientType::GUI; }
};
