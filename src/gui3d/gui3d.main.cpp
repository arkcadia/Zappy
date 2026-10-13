#include "graphical/Graphical3d.hpp"
#include "../gui/network/network_client.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>

struct Args {
    std::string hostname;
    uint16_t    port = 0;
};

static void printUsage(const char *prog)
{
    std::cerr << "Usage: " << prog << " -p port -h machine\n"
              << "  -p port    Port serveur (1-65535)\n"
              << "  -h machine Hostname ou IP\n"
              << "Exemple: " << prog << " -p 4242 -h localhost\n";
}

static Args parseArgs(int argc, char *argv[])
{
    if (argc != 5) { 
        printUsage(argv[0]); 
        throw std::runtime_error("Arguments invalides"); 
    }

    Args args;
    
    for (int i = 1; i < argc; i += 2) {
        std::string flag  = argv[i];
        std::string value = argv[i + 1];

        if (flag == "-p") {
            int v = std::stoi(value);
            if (v < 1 || v > 65535) throw std::out_of_range("Port hors plage");
            args.port = static_cast<uint16_t>(v);
        } else if (flag == "-h") {
            args.hostname = value;
        } else {
            printUsage(argv[0]);
            throw std::runtime_error("Flag inconnu: " + flag);
        }
    }
    if (args.hostname.empty()) 
        throw std::runtime_error("Manque -h machine");
    if (args.port == 0)        
        throw std::runtime_error("Manque -p port");
    return args;
}


int main(int argc, char *argv[])
{
    try {
        Args args = parseArgs(argc, argv);

        std::cout << "Zappy GUI 3D\n"
                  << "Connexion vers " << args.hostname << ":" << args.port << "\n";

        Network::NetworkClient client(args.hostname, args.port);
        client.connect();

        if (!client.isConnected()) {
            std::cerr << "Erreur: " << client.getError() << "\n";
            return 1;
        }

        std::cout << "Connecté !\n";
        client.sendLine("GRAPHIC\n");

        Graphical3D gui(client);
        gui.run();

        client.disconnect();
        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Erreur fatale: " << e.what() << "\n";
        return 1;
    }
}