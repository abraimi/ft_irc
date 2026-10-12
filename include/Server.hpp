#pragma once

#include "Client.hpp"
#include "Channel.hpp"

#include <cstddef>
#include <string>
#include <vector>
#include <poll.h>

class Server {
	private:
		friend class Dispatcher;

		int							_port; 	//the port that my server will listen on 6667 for example
		int							_server_fd; //our listening socket/fd
		std::string 				_password;

		std::vector<Client *>		_clients; //connected clients
		std::vector<Channel *>		_channels;
		std::vector<struct pollfd> 	_pollfds; //fds watched by poll()

		Server(const Server &other);
		Server &operator=(const Server &other);

		void handleNewConn(); //accept new client 
		bool handleClientRead(size_t index); // receive client data
		bool handleClientWrite(size_t index); // false means fd removed
		void enableClientWrite(Client *client);
		Client *removeChannelMember(Channel *channel, Client *client);
		void disconnectOverflowedClients();
	public:
		Server(int port, const std::string &password);
		~Server();
		
		const std::vector<Client *>& getClients() const;
		Client	*getClientByFd(int fd);
		void 	removeClient(int fd);
		Channel	*getChannelByName(const std::string &name) const;
		Channel	*getOrCreateChannel(const std::string &name, bool &created);
		void 	processMessage(Client *client, const std::string &msg);
		void 	sendToClient(Client *client, const std::string &msg);

		void start();
		void setUpSocket(); //create/bind/listen on server side
		void run();
};
