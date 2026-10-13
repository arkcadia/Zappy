#pragma once
#include <string>

class Connection {
public:
    Connection(const std::string &host, int port);
    ~Connection();

    void        connect();
    std::string readLine();
    void        sendLine(const std::string &cmd);

private:
    std::string _host;
    int         _port;
    int         _fd;
    std::string _readBuf;
    std::string _writeBuf;

    void _fill();
    void _flush();
};
