#pragma once

#include <arpa/inet.h>
#include <ctime>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace sys {

[[noreturn]] inline void fail(const std::string& call)
{
    throw std::runtime_error(call + ": " + std::strerror(errno));
}

inline int socket_tcp()
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        fail("socket");
    return fd;
}

inline void setsockopt_reuseaddr(int fd)
{
    int opt = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        fail("setsockopt(SO_REUSEADDR)");
}

inline void bind_tcp(int fd, unsigned short port)
{
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        fail("bind");
}

inline void listen_tcp(int fd, int backlog = 128)
{
    if (::listen(fd, backlog) < 0)
        fail("listen");
}

inline int accept_tcp(int fd)
{
    sockaddr_in addr{};
    socklen_t   len = sizeof(addr);
    int cfd = ::accept(fd, reinterpret_cast<sockaddr*>(&addr), &len);
    if (cfd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;
        fail("accept");
    }
    return cfd;
}

inline void set_nonblocking(int fd)
{
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        fail("fcntl(F_GETFL)");
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        fail("fcntl(F_SETFL, O_NONBLOCK)");
}

inline ssize_t read_fd(int fd, void* buf, std::size_t len)
{
    ssize_t n = ::read(fd, buf, len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;
        if (errno == ECONNRESET)
            return 0;
        fail("read");
    }
    return n;
}

inline ssize_t write_fd(int fd, const void* buf, std::size_t len)
{
    ssize_t n = ::write(fd, buf, len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EPIPE)
            return -1;
        fail("write");
    }
    return n;
}

inline void close_fd(int fd)
{
    if (::close(fd) < 0)
        fail("close");
}

inline int poll_fds(std::vector<pollfd>& fds, int timeout_ms)
{
    int n = ::poll(fds.data(), static_cast<nfds_t>(fds.size()), timeout_ms);
    if (n < 0) {
        if (errno == EINTR)
            return 0;   // interrupted by signal, treat as timeout
        fail("poll");
    }
    return n;
}

inline void nanosleep_ns(long ns)
{
    timespec ts{ .tv_sec = ns / 1'000'000'000L, .tv_nsec = ns % 1'000'000'000L };
    ::nanosleep(&ts, nullptr);
}

} // namespace sys
