#include "Dispatcher.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "MessageException.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

Dispatcher::Dispatcher(Server& server, Client& client, Channel* channel, Message& msg)
	: _server(server), _client(client), _channel(channel), _message(msg)
{}

Dispatcher::~Dispatcher() {}

bool Dispatcher::verifyPassword(const std::string &password) const
{
	return password == _server._password;
}

void Dispatcher::tryCompleteRegistration()
{
	if (_client.isRegistered() || !_client.isAuth()
		|| _client.getNickname().empty() || _client.getUsername().empty())
		return;

	_client.setRegister(true);
	std::ostringstream welcome;
	welcome << ":ft_irc 001 " << _client.getNickname()
		<< " :Welcome to the ft_irc Network " << _client.getNickname()
		<< "!" << _client.getUsername() << "@localhost\r\n";
	_server.sendToClient(&_client, welcome.str());
}

bool Dispatcher::isNicknameInUse(const std::string &nickname) const {
	const std::vector<Client *>& clients = _server.getClients();
	for (std::size_t i = 0; i < clients.size(); ++i)
		if (clients[i] != &_client && !clients[i]->getNickname().empty()
			&& Utils::equalsIgnoreCase(clients[i]->getNickname(), nickname))
			return true;
	return false;
}

void Dispatcher::handlePass() {
	if (_client.isAuth() || _client.isRegistered())
		throw MessageException(ERR_ALREADYREGISTRED, &_client);

	std::vector<std::string> args = _message.getArgs();
	if (!verifyPassword(args[0]))
		throw MessageException(ERR_PASSWDMISMATCH, &_client);

	_client.setisAuth(true);
	tryCompleteRegistration();
}

void Dispatcher::handleNick() {
	std::vector<std::string> args = _message.getArgs();
	if (isNicknameInUse(args[0]))
		throw MessageException(ERR_NICKNAMEINUSE, &_client, args[0]);

	_client.setNickname(args[0]);
	tryCompleteRegistration();
}

void Dispatcher::handleUser() {
	if (_client.isRegistered())
		throw MessageException(ERR_ALREADYREGISTRED, &_client);

	std::vector<std::string> args = _message.getArgs();
	_client.setUserName(args[0]);
	tryCompleteRegistration();
}

void Dispatcher::handleJoin() {
	if (!_client.isRegistered()) {
		throw MessageException(ERR_NOTREGISTERED, &_client);
	}

	std::vector<std::string> args = _message.getArgs();
	bool created = false;
	Channel *channel = _server.getOrCreateChannel(args[0], created);
	if (channel->hasMember(&_client)) {
		return;
	}

	if (!created) {
		if (channel->hasKey()
			&& (args.size() < 2 || args[1] != channel->getKey())) {
			throw MessageException(ERR_BADCHANNELKEY, &_client, args[0]);
		}
		if (channel->isInviteOnly() && !channel->isInvited(&_client)) {
			throw MessageException(ERR_INVITEONLYCHAN, &_client, args[0]);
		}
		if (channel->hasUserLimit()
			&& channel->getMemberCount() >= channel->getUserLimit()) {
			throw MessageException(ERR_CHANNELISFULL, &_client, args[0]);
		}
	}

	if (!channel->addMember(&_client)) {
		return;
	}
	if (created) {
		channel->addOperator(&_client);
	} else {
		channel->revokeInvite(&_client);
	}

	std::string prefix = ":" + _client.getNickname() + "!"
		+ _client.getUsername() + "@localhost";
	const std::vector<Client *>& members = channel->getMembers();
	for (std::size_t i = 0; i < members.size(); ++i) {
		_server.sendToClient(members[i], prefix + " JOIN "
			+ channel->getName() + "\r\n");
	}

	std::ostringstream reply;
	if (!channel->getTopic().empty()) {
		reply << ":ft_irc " << RPL_TOPIC << " " << _client.getNickname()
			<< " " << channel->getName() << " :" << channel->getTopic()
			<< "\r\n";
	}

	std::ostringstream names;
	for (std::size_t i = 0; i < members.size(); ++i) {
		if (i != 0) {
			names << " ";
		}
		if (channel->isOperator(members[i])) {
			names << "@";
		}
		names << members[i]->getNickname();
	}
	reply << ":ft_irc " << RPL_NAMREPLY << " " << _client.getNickname()
		<< " = " << channel->getName() << " :" << names.str() << "\r\n"
		<< ":ft_irc " << RPL_ENDOFNAMES << " " << _client.getNickname()
		<< " " << channel->getName() << " :End of /NAMES list.\r\n";
	_server.sendToClient(&_client, reply.str());
}

void Dispatcher::handlePrivmsg() {
	if (!_client.isRegistered()) {
		throw MessageException(ERR_NOTREGISTERED, &_client);
	}

	std::vector<std::string> args = _message.getArgs();
	const std::string& target = args[0];
	std::string prefix = ":" + _client.getNickname() + "!"
		+ _client.getUsername() + "@localhost PRIVMSG " + target + " :"
		+ args[1] + "\r\n";

	if (target[0] == '#' || target[0] == '&') {
		Channel *channel = _server.getChannelByName(target);
		if (!channel) {
			throw MessageException(ERR_NOSUCHCHANNEL, &_client, target);
		}
		if (!channel->hasMember(&_client)) {
			throw MessageException(ERR_CANNOTSENDTOCHAN, &_client, target);
		}

		const std::vector<Client *>& members = channel->getMembers();
		for (std::size_t i = 0; i < members.size(); ++i) {
			if (members[i] != &_client) {
				_server.sendToClient(members[i], prefix);
			}
		}
		return;
	}

	const std::vector<Client *>& clients = _server.getClients();
	for (std::size_t i = 0; i < clients.size(); ++i) {
		if (!clients[i]->getNickname().empty()
			&& Utils::equalsIgnoreCase(clients[i]->getNickname(), target)) {
			_server.sendToClient(clients[i], prefix);
			return;
		}
	}
	throw MessageException(ERR_NOSUCHNICK, &_client, target);
}

void Dispatcher::handleUnknown() {
	throw MessageException(ERR_UNKNOWNCOMMAND, &_client, _message.getCommandName());
}

void Dispatcher::dispatch() {
	typedef std::pair<Command_t, Handler> Entry;

	static const Entry entries[] = {
		Entry(PASS, &Dispatcher::handlePass),
		Entry(NICK, &Dispatcher::handleNick),
		Entry(USER, &Dispatcher::handleUser),
		Entry(JOIN, &Dispatcher::handleJoin),
		Entry(PRIVMSG, &Dispatcher::handlePrivmsg),
		Entry(UNKNOWN, &Dispatcher::handleUnknown)
	};
	static const HandlerMap handlers(entries, entries + sizeof(entries) / sizeof(entries[0]));
	HandlerMap::const_iterator it =	handlers.find(_message.getType());

	std::cout << "Command Type: " << _message.getType() << std::endl;	

	if (it == handlers.end())
		throw std::runtime_error("No dispatcher handler registered for command");
	(this->*(it->second))();
}
