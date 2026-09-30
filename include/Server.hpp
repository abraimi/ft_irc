#pragma once
#include <string>
#include <vector>
#include <poll.h>
class Server {
	private:
		int			_port; 	//the port that my server will listen on 6667 for example
		int			_server_fd; //our listening socket/fd
		std::string _password;
		std::vector<struct pollfd> _pollfds;
	public:
		Server(int port, const std::string &password);
		~Server();
		
		void start();
		void setUpSocket();
		void run();
};