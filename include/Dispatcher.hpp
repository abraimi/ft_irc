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

		void	handleUnknown();

	public:
		Dispatcher(Server& server, Client& client, Channel* channel, Message& msg);
		~Dispatcher();

		void	dispatch();
};

#endif // DISPATCHER_HPP
