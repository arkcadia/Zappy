#include "network_client.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
namespace Network {
NetworkClient::NetworkClient(const std::string &hostname, uint16_t port)
    : m_hostname(hostname), m_port(port), m_socket(-1) {}

NetworkClient::~NetworkClient() { disconnect(); }
void NetworkClient::setError(const std::string &message) {
    m_error = message;
    std::cerr << "Network error: " << message << "\n";
}

void NetworkClient::connect() {
    if (m_socket >= 0) {
        setError("Already connected");
        return;
    }

    struct hostent *hostInfo = gethostbyname(m_hostname.c_str());
    if (!hostInfo) {
        setError("Failed to resolve hostname: " + m_hostname);
        return;
    }

    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);
    std::memcpy(&serverAddr.sin_addr, hostInfo->h_addr, static_cast<size_t>(hostInfo->h_length));

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) {
        setError("Failed to create socket");
        return;
    }

    if (::connect(m_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        setError("Failed to connect to " + m_hostname + ":" + std::to_string(m_port));
        close(m_socket);
        m_socket = -1;
        return;
    }

    makeNonBlocking();
    std::cout << "Connected to " << m_hostname << ":" << m_port << "\n";
}

void NetworkClient::makeNonBlocking() {
    int flags = fcntl(m_socket, F_GETFL, 0);
    if (flags == -1) {
        setError("Failed to get socket flags");
        return;
    }

    if (fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        setError("Failed to set non-blocking mode");
        return;
    }
}

void NetworkClient::disconnect() {
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
        std::cout << "Disconnected from server\n";
    }
}

bool NetworkClient::hasData() const {
    if (m_socket < 0)
        return false;

    char c;
    int result = static_cast<int>(recv(m_socket, &c, 1, MSG_PEEK));

    if (result < 0) {
        return false;
    }

    if (result == 0) {
        return false;
    }

    return true;
}

std::string NetworkClient::readLine() {
    if (m_socket < 0) {
        return "";
    }

    const int CHUNK_SIZE = 256;
    char buffer[CHUNK_SIZE];

    while (true) {
        size_t newlinePos = m_buffer.find('\n');
        if (newlinePos != std::string::npos) {
            std::string line = m_buffer.substr(0, newlinePos);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            m_buffer = m_buffer.substr(newlinePos + 1);
            return line;
        }

        int bytesRead = static_cast<int>(recv(m_socket, buffer, CHUNK_SIZE - 1, 0));

        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return "";
            }
            setError("Failed to read from socket");
            disconnect();
            return "";
        }

        if (bytesRead == 0) {
            setError("Server closed connection");
            disconnect();
            if (!m_buffer.empty()) {
                std::string line = m_buffer;
                m_buffer.clear();
                return line;
            }
            return "";
        }

        buffer[bytesRead] = '\0';
        m_buffer += buffer;
    }
}

void NetworkClient::sendLine(const std::string &data) {
    if (m_socket < 0)
        return;
    send(m_socket, data.c_str(), data.size(), 0);
}
} // namespace Network
