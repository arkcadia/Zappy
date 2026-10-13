#include "gui.hpp"
#include "graphical/graphical.hpp"
#include "graphical/world.hpp"
#include "network/network_client.hpp"
#include <cstring>
#include <iostream>
void populateWorldWithRandomResources(World &world) {
    for (uint16_t y = 0; y < world.getHeight(); y++) {
        for (uint16_t x = 0; x < world.getWidth(); x++) {
            if ((x + y) % 3 == 0) {
                world.addResource(x, y, ResourceType::Food, 3);
            }
            if ((x + y) % 5 == 0) {
                world.addResource(x, y, ResourceType::Linemate, 1);
            }
            if ((x * y) % 7 == 0) {
                world.addResource(x, y, ResourceType::Sibur, 2);
            }
            if ((x + y + 1) % 11 == 0) {
                world.addResource(x, y, ResourceType::Deraumere, 1);
            }
        }
    }
}

static void printUsage(const char *programName) {
    std::cerr << "Usage: " << programName << " -p port -h machine\n"
              << "  -p port    Server port (1-65535)\n"
              << "  -h machine Server hostname or IP address\n"
              << "Example: " << programName << " -p 4242 -h localhost\n";
}

Gui::Gui(int argc, char *argv[]) : hostname(""), port(0) {
    if (argc != 5) {
        printUsage(argv[0]);
        throw std::runtime_error("Invalid arguments");
    }

    for (int i = 1; i < argc; i += 2) {
        std::string flag = argv[i];

        if (i + 1 >= argc) {
            printUsage(argv[0]);
            throw std::runtime_error("Missing value for flag: " + flag);
        }

        std::string value = argv[i + 1];

        if (flag == "-p") {
            try {
                int portValue = std::stoi(value);
                if (portValue < 1 || portValue > 65535) {
                    throw std::out_of_range("Port out of range");
                }
                port = static_cast<uint16_t>(portValue);
            } catch (const std::exception &e) {
                std::cerr << "Error: Invalid port '" << value << "'\n";
                throw;
            }
        } else if (flag == "-h") {
            hostname = value;
        } else {
            std::cerr << "Error: Unknown flag '" << flag << "'\n";
            printUsage(argv[0]);
            throw std::runtime_error("Unknown flag: " + flag);
        }
    }

    validate();
}

void Gui::validate() const {
    if (hostname.empty()) {
        throw std::runtime_error("Missing required argument: -h machine");
    }
    if (port == 0) {
        throw std::runtime_error("Missing required argument: -p port");
    }
}

int main(int argc, char *argv[]) {
    try {
        Gui args(argc, argv);

        std::cout << "Zappy GUI Client\n";
        std::cout << "Connecting to " << args.hostname << ":" << args.port << "\n";

        Network::NetworkClient networkClient(args.hostname, args.port);
        networkClient.connect();

        if (!networkClient.isConnected()) {
            std::cerr << "Error: " << networkClient.getError() << "\n";
            return 1;
        }

        std::cout << "Connection established!\n";
        networkClient.sendLine("GRAPHIC\n");
        Graphical Graphical(networkClient);
        Graphical.run();

        networkClient.disconnect();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}
