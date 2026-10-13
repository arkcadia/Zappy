#include "core/Agent.hpp"
#include "../debug.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

struct Args {
    std::string host     = "localhost";
    int         port     = -1;
    std::string teamName;
    bool        solo     = false;
};

static Args parseArgs(int argc, char **argv)
{
    Args args;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-p") == 0 && i + 1 < argc)
            args.port = std::stoi(argv[++i]);
        else if (std::strcmp(argv[i], "-n") == 0 && i + 1 < argc)
            args.teamName = argv[++i];
        else if (std::strcmp(argv[i], "-h") == 0 && i + 1 < argc)
            args.host = argv[++i];
        else if (std::strcmp(argv[i], "-s") == 0)
            args.solo = true;
    }
    if (args.port == -1 || args.teamName.empty()) {
        std::cerr << "Usage: " << argv[0] << " -p <port> -n <name> [-h <host>]\n";
        throw std::invalid_argument("missing required arguments");
    }
    return args;
}

int main(int argc, char **argv)
{
    Args args;
    try {
        args = parseArgs(argc, argv);
    } catch (const std::invalid_argument &) {
        return 1;
    }

    srand(static_cast<unsigned>(getpid()));
    try {
        Agent agent(args.host, args.port, args.teamName, args.solo);
        agent.run();
    } catch (const std::exception &e) {
        std::string_view msg = e.what();
        if (msg != "connection closed" && msg != "dead")
            std::cerr << "Error: " << msg << "\n";
        else
            debug_logf("[Agent] exiting: {}", msg);
        return 1;
    }
    return 0;
}
