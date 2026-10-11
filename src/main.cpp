#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <climits>
#include <cerrno>
#include <cstring>

int main(int argc, char **argv) {
	if (argc != 3) {
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return 1;
	}
	char *end = NULL;
	errno = 0;
	long parsedPort = std::strtol(argv[1], &end, 10);
	if (errno || !end || *end || parsedPort < 1 || parsedPort > 65535) {
		std::cerr << "Invalid port" << std::endl;
		return 1;
	}
	if (std::strlen(argv[2]) == 0) {
		std::cerr << "Password must not be empty" << std::endl;
		return 1;
	}
	try {
		Server server(static_cast<int>(parsedPort), argv[2]);
		server.start();
	} catch (const std::exception &error) {
		std::cerr << error.what() << std::endl;
		return 1;
	}
	return 0;
}
