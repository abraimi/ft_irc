#pragma once

#include "../include/Client.hpp"
#include "../include/IRCHandler.hpp"
#include "../include/Channel.hpp"

#include <cstddef>
#include <string>
#include <vector>
#include <poll.h>

class Server {
	private:
		int							_port; 	//the port that my server will listen on 6667 for example
		int							_server_fd; //our listening socket/fd
		std::string 				_password;

		std::vector<Client *>		_clients; //connected clients
		std::vector<Channel *>		_channels; //existing channels
		std::vector<struct pollfd> 	_pollfds; //fds watched by poll()
		IRCHandler 					_irchandler;

		void 	removeClient(int fd); 
		Client	*getClientByFd(int fd);
		void 	processMessage(Client *client, const std::string &msg); //temporary parser connection point
		void 	sendToClient(Client *client, const std::string &msg); // add data to client buff

		//todo
		void handleNewConn(); //accept new client 
		bool handleClientRead(size_t index); // receive client data
		void handleClientwrite(size_t index); // send buff data
		void enableClientWrite(Client *client);
	public:
		Server(int port, const std::string &password);
		~Server();
		

		void start();
		void setUpSocket(); //create/bind/listen on server side
		void run();
};
