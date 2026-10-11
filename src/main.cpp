#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

   try {
    int port = std::atoi(argv[1]);

    Server server(port, argv[2]);
    server.start();
   } catch (std::runtime_error& x) {
    std::cerr << x.what() << std::endl;
   } catch (...) {
    
   }

    return 0;
}