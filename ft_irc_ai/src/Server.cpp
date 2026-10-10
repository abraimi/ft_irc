#include "../include/Server.hpp"
#include "Command.hpp"
#include <cstddef>
#include <sys/poll.h>
#include <sys/socket.h>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>


Server::Server(int port, const std::string &password) : _port(port), _server_fd(-1), _password(password), \
	_clients() , _channels(), _pollfds(), _irchandler(password, _clients, _channels)
{}
Server::~Server() {
	for(size_t i = 0; i < _clients.size(); i++)
	{
		close(_clients[i]->getFd());
		delete _clients[i];
	}
	for(size_t i = 0; i < _channels.size(); i++)
		delete _channels[i];
	if(_server_fd != -1)
		close(_server_fd); //close listenin sock
}

void Server::removeClient(int fd)
{
	// detach the client from every channel it was in
	for(size_t i = 0; i < _channels.size(); i++)
	{
		Channel *channel = _channels[i];
		std::vector<Client *> &members = channel->getMembers();
		for(size_t j = 0; j < members.size(); j++)
		{
			if(members[j]->getFd() == fd)
			{
				channel->removeMember(members[j]);
				break;
			}
		}
		if(channel->isEmpty())
		{
			delete channel;
			_channels.erase(_channels.begin() + i);
			i--;
		}
	}
	for(size_t i = 0; i < _clients.size(); i++)
	{
		if(_clients[i]->getFd() == fd)
		{
			delete _clients[i];
			_clients.erase(_clients.begin() + i); // remove pointer form the vector
			return;
		}
	}
}

static void fatal_error(int server_fd, const std::string &except)
{
	close(server_fd);
	throw std::runtime_error(except);
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

void Server::processMessage(Client *client, const std::string &msg) //connection point with the IRC parser
{
	Command command = IRCHandler::parseCommand(msg);
	if(command.name.empty())
		return;

	_irchandler.handle(client, command);

	if(command.name == "QUIT")
	{
		int fd = client->getFd();
		close(fd);
		removeClient(fd);
		for(size_t i = 0; i < _pollfds.size(); i++)
		{
			if(_pollfds[i].fd == fd)
			{
				_pollfds.erase(_pollfds.begin() + i);
				break;
			}
		}
		std::cout << "Client quit fd " << fd << std::endl;
		return;
	}
	
	//keep this after a finished parsing for POLLOUT 
	if (!client->getSendBuff().empty())
        enableClientWrite(client);
}

void Server::setUpSocket() {

	int opt = 1;
	struct sockaddr_in address = {};
	struct pollfd	   server_poll_fd;

	_server_fd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET address family (ipv4), SOCK_STREAM communication type, 0 means the default communication protocol (tcp/ip)
	if(_server_fd == -1)
		throw std::runtime_error("socket() failed");
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

void Server::handleNewConn()
{
	int client_fd = accept(_server_fd, NULL, NULL); //create another fd for the clients while server fd stays alive
	if(client_fd == -1)
		return;
	if(fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) { //make the server client NONEBLOKING, later we will have recv(client_fd) and we want it none blocking
		close(client_fd);
		return;
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

bool Server::handleClientRead(size_t index)
{
	char buffer[4096];
	int client_fd = _pollfds[index].fd;
	int b_r = recv(client_fd, buffer, sizeof(buffer), 0); // give me data the client sent (man recv)
	if (b_r <= 0) { // 0 means the client disconnected clean, -1 error
		close(client_fd);
		removeClient(client_fd);
		_pollfds.erase(_pollfds.begin() + index);
		std::cout << "client disconnected fd " << client_fd << std::endl;
		return false;
	}
	Client *client = getClientByFd(client_fd);
	if (client == NULL)
		return true;
	client->getBuff().append(buffer, b_r); // keep appending data to buffer in case of interrupted msgs co^mma^nd

	//temporary line extraction
	std::string &client_buffer = client->getBuff();
	size_t pos = client_buffer.find("\r\n");
	while(pos != std::string::npos)
	{
		std::string msg = client_buffer.substr(0, pos);
		client_buffer.erase(0, pos + 2); // remove it before process
		pos = client_buffer.find("\r\n"); //get next pos
		processMessage(client, msg); // eventually we will need to store this so it become the connection point to the server
		if (getClientByFd(client_fd) != client) // client was removed (e.g. QUIT)
			return false;
	}
	return true;
}

void Server::enableClientWrite(Client *client)
{
    size_t i = 0;

    while (i < _pollfds.size())
    {
        if (_pollfds[i].fd == client->getFd())
        {
            _pollfds[i].events |= POLLOUT;
            return;
        }

        i++;
    }
}

void Server::handleClientwrite(size_t index)
{
	Client *client = getClientByFd(_pollfds[index].fd);
	if (client == NULL)
		return;
	std::string &send_buff = client->getSendBuff();
	if(!send_buff.empty())
	{
		int b_s = send(client->getFd(), send_buff.c_str(), send_buff.size(), 0); ///send as much of the buffered data as possible
		if(b_s > 0)
			send_buff.erase(0, b_s); // only remove byte that were sent and keep the remaining
	}
	if(send_buff.empty())
		_pollfds[index].events &= ~POLLOUT; // nothing left to send , stop asking poll() about POLLOUT
}

void Server::run()
{
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
			if(_pollfds[i].fd != _server_fd && (_pollfds[i].revents & (POLLHUP | POLLERR | POLLNVAL))) { //bitmask check if a client socket had an error
				int client_fd = _pollfds[i].fd;
				close(client_fd);
				removeClient(client_fd);
				_pollfds.erase(_pollfds.begin() + i);
				std::cout << "Client error or disconnected fd " << client_fd << std::endl;
				continue;		
			}
			if(_pollfds[i].revents & POLLIN)
			{
				if(_pollfds[i].fd == _server_fd)
					handleNewConn();
				else 
					if(!handleClientRead(i))
						continue;
			}
			if(i < _pollfds.size() && (_pollfds[i].revents & POLLOUT))
				handleClientwrite(i);
			i++;
		}	
	}
}




void Server::start()
{
	setUpSocket();
	run();
}
