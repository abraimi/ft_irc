#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include "Message.hpp"
#include <map>
#include <string>
class Server;
class Client;
class Message;
class Channel;

class Dispatcher {
	private:
		Server&		_server;
		Client&		_client;
		Message&	_message;

		typedef void (Dispatcher::*Handler)();
		typedef std::map<Command_t, Handler> HandlerMap;
		typedef void (Dispatcher::*ModeHandler)(Channel &, bool,
			const std::string &, std::string &);

		Dispatcher(const Dispatcher &other);
		Dispatcher &operator=(const Dispatcher &other);

		bool		verifyPassword(const std::string &password) const;
		void		tryCompleteRegistration();
		bool		isNicknameInUse(const std::string &nickname) const;
		Client		*findClientByNickname(const std::string &nickname) const;

		void	handlePass();
		void	handleNick();
		void	handleUser();
		void	handleJoin();
		void	handlePrivmsg();
		void	handleKick();
		void	handleInvite();
		void	handleTopic();
		void	handleMode();
		void	sendChannelModes(Channel &channel);
		void	applyChannelModes(Channel &channel,
			const std::vector<std::string> &args);
		void	applyInviteOnlyMode(Channel &channel, bool enabled,
			const std::string &parameter, std::string &parameters);
		void	applyTopicOnlyMode(Channel &channel, bool enabled,
			const std::string &parameter, std::string &parameters);
		void	applyKeyMode(Channel &channel, bool enabled,
			const std::string &parameter, std::string &parameters);
		void	applyLimitMode(Channel &channel, bool enabled,
			const std::string &parameter, std::string &parameters);
		void	applyOperatorMode(Channel &channel, bool enabled,
			const std::string &parameter, std::string &parameters);
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
