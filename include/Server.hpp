#pragma once

#include "../include/Client.hpp"
#include <string>
#include <vector>
#include <poll.h>

class Server {
	private:
		friend class Dispatcher;

		int							_port; 	//the port that my server will listen on 6667 for example
		int							_server_fd; //our listening socket/fd
		std::string 				_password;
		std::vector<struct pollfd> 	_pollfds; 
		std::vector<Client *>		_clients;
	public:
		Server(int port, const std::string &password);
		~Server();
		
		Client	*getClientByFd(int fd);
		void 	removeClient(int fd);
		const std::vector<Client *>& getClients() const;
		void 	processMessage(Client *client, const std::string &msg);
		void 	sendToClient(Client *client, const std::string &msg); //!enable POLLOUT

		void start();
		void setUpSocket();
		void run();
};
