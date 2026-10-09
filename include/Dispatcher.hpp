#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include "Message.hpp"
#include <map>
class Server;
class Client;
class Channel;
class Message;

class Dispatcher {
	private:
		Server&	_server;
		Client&	_client;
		Channel*	_channel;
		Message&	_message;

		typedef void (Dispatcher::*Handler)();
		typedef std::map<Command_t, Handler> HandlerMap;

		bool	isPasswordValid(const std::string &password) const;
		void	tryCompleteRegistration();
		static bool sameNickname(const std::string &left, const std::string &right);
		bool	isNicknameInUse(const std::string &nickname) const;
		void	handlePass();
		void	handleNick();
		void	handleUser();
		void	handleUnknown();

	public:
		Dispatcher(Server& server, Client& client, Channel* channel, Message& msg);
		~Dispatcher();

		void	dispatch();
};

#endif // DISPATCHER_HPP
