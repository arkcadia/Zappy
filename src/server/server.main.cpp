#include "Server.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "../debug.hpp"

int main(int argc, char **argv)
{
    std::vector<std::string> args;

    for (int i = 1; i < argc; i++)
        args.push_back(argv[i]);

    try {
        Server server(args);

        if (!server.done)
            return 0;

        debug_logf("{}", server);

        server.createWorld();

        server.run();

    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        display_help();
        return 84;
    }

    return 0;
}
