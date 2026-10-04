#include "../include/Server.hpp"
#include "Command.hpp"
#include <cstddef>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <vector>

Server::Server(int port, const std::string &password) : _port(port), _server_fd(-1),_password(password)
{}
Server::~Server() {
	for(size_t i = 0; i < _clients.size(); i++)
	{
		close(_clients[i]->getFd());
		delete _clients[i];
	}
	if(_server_fd != -1)
		close(_server_fd);
}

void Server::removeClient(int fd)
{
	for(size_t i = 0; i < _clients.size(); i++)
	{
		if(_clients[i]->getFd() == fd)
		{
			delete _clients[i];
			_clients.erase(_clients.begin() + i); // remove the pointer form the vector
			return;
		}
	}
}

static void fatal_error(int server_fd, const std::string &except)
{
	close(server_fd);
	throw std::runtime_error(except);
}

void Server::sendToClient(Client *client, const std::string &msg)
{
	client->getSendBuff().append(msg);
	for(size_t i = 0; i < _pollfds.size(); i++)
	{
		if(_pollfds[i].fd == client->getFd()){
			_pollfds[i].events |= POLLOUT; // using OR bitmask operator because events is typically a bitmask , each bit represents a different event we are intersted in
			//POLLIN = reading // POLLOUT = writing // POLLERR = error // POLLIN |= POLLOUT = 0001 |= 0010 = POLLOUT
			return; 
		}
	}
}
Client *Server::getClientByFd(int fd)
{
	for(size_t i = 0; i < _clients.size(); i++)
	{
		if(_clients[i]->getFd() == fd)
			return _clients[i];
	}
	return NULL;
}

void Server::processMessage(Client *client, const std::string &msg) //just to check the way i retrieve msg parser need to implement it
{
	std::cout << "the complete msg from fd " << client->getFd() << ": " << msg << std::endl;
	sendToClient(client, "server received: " + msg + "\r\n");
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
			if(_pollfds[i].revents == 0) { // nothing happened check next fd
				i++;
				continue;
			}
			if(_pollfds[i].revents & POLLIN) { // bitmask check if POLLIN flaged revents
			
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
					client_poll_fd.events = POLLIN; // POLLIN = tell me when this socket is ready for me to read
					client_poll_fd.revents = 0;

					_pollfds.push_back(client_poll_fd); // add the client to our vector
					Client *client = new Client(client_fd); //free later
					_clients.push_back(client);
					std::cout << "New client connected fd " << client_fd << std::endl;
				}
				else { // dealing with an existing client aka not the server fd
					int client_fd = _pollfds[i].fd;
					int b_r = recv(client_fd, buffer, sizeof(buffer) - 1, 0); // give me data the client sent (man recv)
					if (b_r <= 0) { // 0 means the client disconnected clean, -1 error
						close(client_fd);
						removeClient(client_fd);
						_pollfds.erase(_pollfds.begin() + i);
						std::cout << "client disconnected fd " << client_fd << std::endl;
						continue;
					}
					else if (b_r > 0)
					{
						Client *client = getClientByFd(client_fd);
						client->getBuff().append(buffer, b_r); // keep appending data to buffer in case of interrupted msgs co^mma^nd
						Command	cmd;

						cmd.parseCommand(client->getBuff());

						std::cout << "Valid: " << cmd.getValid()
							<< "Cmd type: " << cmd.getType()
							<< "Authority: " << cmd.getAuthority()
							<< "ErrNum: " << cmd.getErrNum()
						<< std::endl;
						for (size_t idx = 0; idx < cmd.getArgs().size(); idx++) {
							std::cout <<
								cmd.getArgs().at(idx) << ","
							<< std::endl;
						}
						/**
							TODO: In here the execution of the command should be added 
						*/
						// std::cout << "Valid: " << cmd.valid
						// 	<< "Cmd type: " << cmd.type 
						// 	<< "Authority: " << cmd.authority
						// 	<< "ErrNum: " << cmd.errNum
						// << std::endl;
						// for (size_t idx = 0; idx < cmd.args.size(); idx++) {
						// 	std::cout <<
						// 		cmd.args.at(idx) << ","
						// 	<< std::endl;
						// }
						// client->getBuff().append(buffer, b_r); // keep appending data to buffer in case of interrupted msgs co^mma^nd
						// //!!! for testing only parser need to implement this
						// std::string &client_buffer = client->getBuff();
						// size_t pos = client_buffer.find("\r\n");
						// while(pos != std::string::npos)
						// {
						// 	std::string msg = client_buffer.substr(0, pos);
						// 	processMessage(client, msg); // eventually we will need to store this so it become the connection point to the server
						// 	client_buffer.erase(0, pos + 2); // remove it after process 
						// 	pos = client_buffer.find("\r\n"); //get next pos
						// }
					}
					// std::cout << "client " << client_fd << " sent: " << buffer << std::endl;
				}
			}
			if(_pollfds[i].revents & POLLOUT)
			{
				Client *client = getClientByFd(_pollfds[i].fd);
				if(client != NULL)
				{
					std::string &send_buff = client->getSendBuff();
					if(!send_buff.empty())
					{
						int b_s = send(client->getFd(), send_buff.c_str(), send_buff.size(), 0); ///man send() im sick of commenting
						if(b_s > 0)
							send_buff.erase(0, b_s);
					}
					if(send_buff.empty())
						_pollfds[i].events &= ~POLLOUT; // tell the os to stop POLL() to notify us about writing 
				}
			}
				i++;

		}
		
	}
	
}



void Server::start()
{
	setUpSocket();
	run();
}
