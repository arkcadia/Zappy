#include "Connection.hpp"
#include "../../debug.hpp"
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netdb.h>

Connection::Connection(const std::string &host, int port)
    : _host(host), _port(port), _fd(-1) {}

Connection::~Connection()
{
    if (_fd != -1)
        close(_fd);
}

void Connection::connect()
{
    debug_logf("[Connection] connecting to {}:{}", _host, _port);

    struct addrinfo hints{}, *res;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(_host.c_str(), std::to_string(_port).c_str(), &hints, &res) != 0) {
        debug_logf("[Connection] getaddrinfo failed for {}:{}", _host, _port);
        throw std::runtime_error("getaddrinfo failed");
    }

    _fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (_fd < 0 || ::connect(_fd, res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res);
        debug_logf("[Connection] connect() failed to {}:{}", _host, _port);
        throw std::runtime_error("connection failed");
    }
    freeaddrinfo(res);

    int flags = fcntl(_fd, F_GETFL, 0);
    fcntl(_fd, F_SETFL, flags | O_NONBLOCK);

    debug_logf("[Connection] connected to {}:{} (fd={})", _host, _port, _fd);
}

void Connection::_fill()
{
    struct pollfd pfd{_fd, POLLIN, 0};
    if (poll(&pfd, 1, -1) <= 0) {
        debug_logf("[Connection fd={}] poll(POLLIN) failed", _fd);
        throw std::runtime_error("poll POLLIN failed");
    }

    char    buf[4096];
    ssize_t n = read(_fd, buf, sizeof(buf));
    if (n <= 0) {
        debug_logf("[Connection fd={}] connection closed by peer", _fd);
        throw std::runtime_error("connection closed");
    }
    verbose_logf("[Connection fd={}] read {} bytes", _fd, n);
    _readBuf.append(buf, static_cast<size_t>(n));
}

std::string Connection::readLine()
{
    while (true) {
        auto pos = _readBuf.find('\n');
        if (pos != std::string::npos) {
            std::string line = _readBuf.substr(0, pos);
            _readBuf.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            verbose_logf("[Connection fd={}] << {}", _fd, line);
            return line;
        }
        _fill();
    }
}

void Connection::_flush()
{
    while (!_writeBuf.empty()) {
        struct pollfd pfd{_fd, POLLOUT, 0};
        if (poll(&pfd, 1, -1) <= 0) {
            debug_logf("[Connection fd={}] poll(POLLOUT) failed", _fd);
            throw std::runtime_error("poll POLLOUT failed");
        }

        ssize_t n = write(_fd, _writeBuf.c_str(), _writeBuf.size());
        if (n <= 0) {
            debug_logf("[Connection fd={}] write() failed", _fd);
            throw std::runtime_error("write failed");
        }
        _writeBuf.erase(0, static_cast<size_t>(n));
    }
}

void Connection::sendLine(const std::string &cmd)
{
    verbose_logf("[Connection fd={}] >> {}", _fd, cmd);
    _writeBuf += cmd + "\n";
    _flush();
}
