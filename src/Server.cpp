#include "../include/Server.hpp"
#include <sys/poll.h>
#include <sys/socket.h>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>

Server::Server(int port, const std::string &password) : _port(port), _server_fd(-1),_password(password)
{}
Server::~Server() {}

static void fatal_error(int server_fd, const std::string &except)
{
	close(server_fd);
	throw std::runtime_error(except);
}
void Server::setUpSocket() {

	int opt = 1;
	struct sockaddr_in address = {};
	struct pollfd	   server_poll_fd;

	_server_fd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET address family (ipv4), SOCK_STREAM communication type, 0 means the default communication protocol (tcp/ip)
	if(_server_fd == -1)
		throw std::runtime_error("socket() failed");
	// std::cout << _server_fd << std::endl;
	if(fcntl(_server_fd, F_SETFL, O_NONBLOCK) == -1) // control the fd and make it NON BLOCKING so it wont hand waiting for one conection (man fcntl)
		fatal_error(_server_fd, "fcntl() failed");
	
	if(setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR ,&opt, sizeof(opt)) == -1) // function used to allow the reuse of os tcp fds after it interruption
		fatal_error(_server_fd, "setsockopt failed");

	address.sin_family = AF_INET;
	address.sin_port = htons(_port);
	address.sin_addr.s_addr = htonl(INADDR_ANY); //all local IPv4 interfaces

	if(bind(_server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
		fatal_error(_server_fd, "bind() failed");
	
	if(listen(_server_fd, 10) == -1) // 10 is the pending connection requests the os can queue while my program hasnt accepeted them yet !not how many clients can connect
		fatal_error(_server_fd, "listen() failed");
	
	server_poll_fd.fd = _server_fd;
	server_poll_fd.events = POLLIN; // ask os whem _server_fd FD has something available to read. server and client side (new connection waiting or client sending data)
	server_poll_fd.revents = 0; // result (the event that occured on this fd after POLL() returned)

	_pollfds.push_back(server_poll_fd); // vector class because eventually we will have multiple clients that will use POLL()

	std::cout << "Server Socket created and boud to port " << _port << std::endl;
}

void Server::run()
{
	char buffer[4096];

	while (true)
	{
		int ret = poll(&_pollfds[0], _pollfds.size(), -1); //index 0 is the server socket, -1 means "wait until something hapens"
		if (ret == -1)
			throw std::runtime_error("poll() failed");
		size_t i = 0;
		while (i < _pollfds.size())
		{
			if(_pollfds[i].events == 0) { // nothing happened check next fd
				i++;
				continue;
			}
			if(_pollfds[i].revents && POLLIN) { // bitmask check if POLLIN flaged revents
			
				if(_pollfds[i].fd == _server_fd) { // a new client trying to connect
					int client_fd = accept(_server_fd, NULL, NULL); //create another fd for the clients while server fd stays alive
					if(client_fd == -1) {
						i++;
						continue; //accept failed move to next fd
					}
					if(fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) { //make the server client NONEBLOKING, later we will have recv(client_fd) and we want it none blocking
						close(client_fd);
						i++;
						continue;
					}
					struct pollfd client_poll_fd; //to tell poll to aslo watch for this new client
					client_poll_fd.fd = client_fd;
					client_poll_fd.events = POLLIN;
					client_poll_fd.revents = 0;

					_pollfds.push_back(client_poll_fd); // add the client to our vector
					std::cout << "New client connected fd " << client_fd << std::endl;
					i++;
				}
				else { // dealing with an existing client aka not the server fd
					int client_fd = _pollfds[i].fd;
					int b_r = recv(client_fd, buffer, sizeof(buffer) - 1, 0); // give me data the client sent (man recv)
					if (b_r <= 0) { // 0 means the client disconnected clean, -1 error
						close(client_fd);
						_pollfds.erase(_pollfds.begin() + i);
						std::cout << "client disconnected fd " << client_fd << std::endl;
						continue;
					}
					buffer[b_r] = '\0';
					std::cout << "client " << client_fd << " sent: " << buffer << std::endl;
					i++;
				}
			}
			else
				i++;
		}
		
	}
	
}

void Server::start()
{
	setUpSocket();
	run();
}
