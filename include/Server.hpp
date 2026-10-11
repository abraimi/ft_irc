#pragma once

#include "../include/Client.hpp"
#include "../include/IRCHandler.hpp"

#include <cstddef>
#include <string>
#include <vector>
#include <poll.h>

class Channel;

class Server {
	private:
		friend class Dispatcher;

		int							_port; 	//the port that my server will listen on 6667 for example
		int							_server_fd; //our listening socket/fd
		std::string 				_password;

		std::vector<Client *>		_clients; //connected clients
		std::vector<struct pollfd> 	_pollfds; //fds watched by poll()
		IRCHandler 					_irchandler;

		void 	removeClient(int fd); 
		Client	*getClientByFd(int fd);
		void 	processMessage(Client *client, const std::string &msg); // temporary parser connection point
		void 	sendToClient(Client *client, const std::string &msg); // add data to client buff

		//todo
		void handleNewConn(); //accept new client 
		bool handleClientRead(size_t index); // receive client data
		void handleClientwrite(size_t index); // send buff data
		void enableClientWrite(Client *client);
	public:
		Server(int port, const std::string &password);
		~Server();
		
		Client	*getClientByFd(int fd);
		void 	removeClient(int fd);
		const std::vector<Client *>& getClients() const;
		Channel	*getChannelByName(const std::string &name) const;
		Channel	*getOrCreateChannel(const std::string &name, bool &created);
		void 	processMessage(Client *client, const std::string &msg);
		void 	sendToClient(Client *client, const std::string &msg); //!enable POLLOUT

		void start();
		void setUpSocket(); //create/bind/listen on server side
		void run();
};
