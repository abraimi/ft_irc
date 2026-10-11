#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include "Message.hpp"
#include <map>
#include <string>
class Server;
class Client;
class Message;

class Dispatcher {
	private:
		Server&		_server;
		Client&		_client;
		Message&	_message;

		typedef void (Dispatcher::*Handler)();
		typedef std::map<Command_t, Handler> HandlerMap;

		bool		verifyPassword(const std::string &password) const;
		void		tryCompleteRegistration();
		bool		isNicknameInUse(const std::string &nickname) const;

		void	handlePass();
		void	handleNick();
		void	handleUser();
		void	handleJoin();
		void	handlePrivmsg();
		void	handleKick();
		void	handleInvite();
		void	handleTopic();
		void	handleMode();
		void	handleCap();
		void	handlePing();
		void	handleUnknown();
		void	handleBot();
		double	calculate(const std::string &input);


	public:
		Dispatcher(Server& server, Client& client, Message& msg);
		~Dispatcher();

		void	dispatch();
};

#endif // DISPATCHER_HPP
