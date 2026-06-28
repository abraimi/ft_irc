#include "Server.hpp"
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

Server::Server() : port(6969), password("password") {};

Server::Server(int port, std::string password) : password(password) {
	if (port < 0 || port > 65535) {
		throw std::runtime_error("Port is out of range!");
	}
}

Server::~Server() {};

void	Server::initSocket() {
	int		optval = 1;

	this->serverSock = socket(AF_INET, SOCK_STREAM, 0);

	if (this->serverSock < 0)
		throw std::runtime_error("Socket Initiation Failed!");
	if (setsockopt(this->serverSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
		throw std::runtime_error("Couldn't set the socket options!");
	if (fcntl(this->serverSock, F_SETFL, O_NONBLOCK) < 0) {
		throw std::runtime_error("Couldn't set NON-BLOCK mode!");
	} 
}

void	Server::bindSocket() {
	sockaddr_in serverAddr;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(this->port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(this->serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
		throw std::runtime_error("Couldn't bind port to the server!");
	}
}
