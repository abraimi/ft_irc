#include "../include/Server.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ft_irc_ai <port> <password>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);

    Server server(port, argv[2]);
    server.start();

    return 0;
}
