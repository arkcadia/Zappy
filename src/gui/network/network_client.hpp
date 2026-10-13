#pragma once
#include <cstdint>
#include <string>

/**
 * @file network_client.hpp
 * @brief Non-blocking TCP client for the Zappy server protocol.
 *
 * NetworkClient manages a single TCP socket connection to a Zappy server.
 * All I/O is non-blocking; readLine() returns an empty string immediately
 * when no complete line is available rather than blocking the render loop.
 */

namespace Network {

/**
 * @brief Non-blocking TCP client for the Zappy server.
 *
 * ### Connection lifecycle
 * 1. Construct with hostname and port.
 * 2. Call connect() — this is the only blocking call (synchronous DNS + connect).
 * 3. Poll readLine() each frame to receive server messages.
 * 4. Call disconnect() or let the destructor close the socket.
 *
 * ### Non-blocking I/O
 * After a successful connect(), the socket is put into O_NONBLOCK mode.
 * readLine() uses recv() with EAGAIN/EWOULDBLOCK handling: it returns an
 * empty string when no data is ready rather than sleeping.
 *
 * ### Internal buffer
 * Received bytes accumulate in m_buffer. readLine() returns the first
 * complete newline-terminated line and leaves any remaining bytes in the
 * buffer for the next call. CR+LF line endings are handled by stripping
 * a trailing '\r' before returning.
 */
class NetworkClient {
  public:
    /**
     * @brief Constructs the client with target host and port.
     *
     * Does NOT open the socket — call connect() separately.
     *
     * @param hostname Server hostname or dotted-decimal IP address.
     * @param port     Server TCP port [1-65535].
     */
    NetworkClient(const std::string &hostname, uint16_t port);

    /**
     * @brief Disconnects and closes the socket if still open.
     */
    ~NetworkClient();

    /**
     * @brief Resolves the hostname and opens a TCP connection to the server.
     *
     * Blocking: performs a synchronous DNS lookup (gethostbyname) and
     * connect() syscall. On success the socket is switched to non-blocking
     * mode. On failure, sets an error string retrievable via getError().
     *
     * Has no effect if the socket is already open.
     */
    void connect();

    /**
     * @brief Returns true when the socket is open and connected.
     * @return `true` if the socket file descriptor is valid (>= 0).
     */
    bool isConnected() const { return m_socket >= 0; }

    /**
     * @brief Closes the socket and marks the client as disconnected.
     *
     * Safe to call multiple times; subsequent calls are no-ops.
     */
    void disconnect();

    /**
     * @brief Reads and returns the next complete line from the server.
     *
     * Consumes bytes from the internal receive buffer until a newline is
     * found. If no complete line is available, reads from the socket in
     * 256-byte chunks until EAGAIN/EWOULDBLOCK is returned.
     *
     * @return The next line with the trailing newline removed, or an empty
     *         string when no complete line is available yet.
     */
    std::string readLine();

    /**
     * @brief Returns true if at least one byte is available to read.
     *
     * Uses MSG_PEEK so the byte is not consumed. This is a lightweight
     * check; prefer readLine() for actual data consumption.
     *
     * @return `true` when recv(MSG_PEEK) returns > 0.
     */
    bool hasData() const;

    /**
     * @brief Returns the last error message set by a failed operation.
     * @return Human-readable error description, or an empty string if no error.
     */
    const std::string &getError() const { return m_error; }

    /**
     * @brief Clears the stored error message.
     */
    void clearError() { m_error.clear(); }

    /**
     * @brief Sends a line of text to the server.
     *
     * Does not append a newline — the caller is responsible for including
     * any required line terminator in @p data.
     *
     * @param data Raw bytes to send.
     */
    void sendLine(const std::string &data);

  private:
    std::string m_hostname; ///< Target server hostname or IP.
    uint16_t m_port;        ///< Target server TCP port.
    int m_socket;           ///< File descriptor; -1 when disconnected.
    std::string m_error;    ///< Last error message; empty when no error.
    std::string m_buffer;   ///< Partial-line receive buffer.

    /**
     * @brief Stores @p message in m_error and prints it to stderr.
     * @param message Human-readable error description.
     */
    void setError(const std::string &message);

    /**
     * @brief Switches the socket to O_NONBLOCK mode via fcntl(F_SETFL).
     *
     * Called automatically by connect() after a successful connection.
     * Sets an error and returns if the fcntl calls fail.
     */
    void makeNonBlocking();
};

} // namespace Network
