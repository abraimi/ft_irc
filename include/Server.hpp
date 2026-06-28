#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

class Server {
	private:
		int			port;
		std::string	password;
		int			serverSock;

		/* Sockets */
		void	initSocket();

	public:
		Server();
		Server(int port, std::string password);
		~Server();

		void	run();
		void	stop();
};

#endif // SERVER_HPP