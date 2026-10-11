#include "Dispatcher.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "Message.hpp"
#include "MessageException.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include <map>
#include <sstream>
#include <vector>

namespace {

std::string nickOf(const Client &client) {
	return client.getNickname().empty() ? "*" : client.getNickname();
}

std::string prefixOf(const Client &client) {
	return ":" + client.getNickname() + "!" + client.getUsername() + "@"
		+ client.getHostname();
}

void broadcast(Server &server, const std::vector<Client *> &members,
		const std::string &line) {
	for (std::size_t i = 0; i < members.size(); ++i)
		server.sendToClient(members[i], line);
}

bool parsePositive(const std::string &text, std::size_t &value) {
	if (text.empty())
		return false;
	std::size_t result = 0;
	for (std::string::size_type i = 0; i < text.size(); ++i) {
		if (text[i] < '0' || text[i] > '9')
			return false;
		if (result > 100000)
			return false;
		result = result * 10 + static_cast<std::size_t>(text[i] - '0');
		if (result > 1000000)
			return false;
	}
	if (result == 0)
		return false;
	value = result;
	return true;
}

}

Dispatcher::Dispatcher(Server &server, Client &client, Message &msg)
	: _server(server), _client(client), _message(msg) {}

Dispatcher::~Dispatcher() {}

bool Dispatcher::verifyPassword(const std::string &password) const {
	return password == _server._password;
}

void Dispatcher::tryCompleteRegistration() {
	if (_client.isRegistered() || !_client.isAuth()
		|| _client.getNickname().empty() || _client.getUsername().empty())
		return;
	_client.setRegister(true);
	std::ostringstream welcome;
	welcome << ":ft_irc 001 " << _client.getNickname()
		<< " :Welcome to the ft_irc Network " << _client.getNickname()
		<< "!" << _client.getUsername() << "@" << _client.getHostname() << "\r\n";
	_server.sendToClient(&_client, welcome.str());
	std::ostringstream info;
	info << ":ft_irc 002 " << _client.getNickname()
		<< " :Your host is ft_irc, running version 1.0\r\n"
		<< ":ft_irc 003 " << _client.getNickname()
		<< " :This server was created for the 42 ft_irc project\r\n"
		<< ":ft_irc 004 " << _client.getNickname()
		<< " ft_irc 1.0 itkol :itkol\r\n";
	_server.sendToClient(&_client, info.str());
}

bool Dispatcher::isNicknameInUse(const std::string &nickname) const {
	const std::vector<Client *> &clients = _server.getClients();
	for (std::size_t i = 0; i < clients.size(); ++i)
		if (clients[i] != &_client && !clients[i]->getNickname().empty()
			&& Utils::equalsIgnoreCase(clients[i]->getNickname(), nickname))
			return true;
	return false;
}

void Dispatcher::handlePass() {
	if (_client.isAuth() || _client.isRegistered())
		throw MessageException(ERR_ALREADYREGISTRED, &_client);
	const std::vector<std::string> args = _message.getArgs();
	if (!verifyPassword(args[0]))
		throw MessageException(ERR_PASSWDMISMATCH, &_client);
	_client.setisAuth(true);
	tryCompleteRegistration();
}

void Dispatcher::handleNick() {
	const std::string nickname = _message.getArgs()[0];
	if (isNicknameInUse(nickname))
		throw MessageException(ERR_NICKNAMEINUSE, &_client, nickname);
	const std::string oldNickname = _client.getNickname();
	if (!oldNickname.empty() && oldNickname != nickname) {
		const std::string change = prefixOf(_client) + " NICK :" + nickname + "\r\n";
		std::vector<Client *> recipients;
		recipients.push_back(&_client);
		for (std::size_t i = 0; i < _server._channels.size(); ++i) {
			if (!_server._channels[i]->hasMember(&_client))
				continue;
			const std::vector<Client *> &members = _server._channels[i]->getMembers();
			for (std::size_t j = 0; j < members.size(); ++j) {
				bool found = false;
				for (std::size_t k = 0; k < recipients.size(); ++k)
					if (recipients[k] == members[j])
						found = true;
				if (!found)
					recipients.push_back(members[j]);
			}
		}
		broadcast(_server, recipients, change);
	}
	_client.setNickname(nickname);
	tryCompleteRegistration();
}

void Dispatcher::handleUser() {
	if (_client.isRegistered())
		throw MessageException(ERR_ALREADYREGISTRED, &_client);
	const std::vector<std::string> args = _message.getArgs();
	if (! _client.getUsername().empty())
		throw MessageException(ERR_ALREADYREGISTRED, &_client);
	_client.setUserName(args[0]);
	_client.setRealname(args[3]);
	tryCompleteRegistration();
}

void Dispatcher::handleJoin() {
	if (!_client.isRegistered())
		throw MessageException(ERR_NOTREGISTERED, &_client);
	const std::vector<std::string> args = _message.getArgs();
	bool created = false;
	Channel *channel = _server.getOrCreateChannel(args[0], created);
	if (channel->hasMember(&_client))
		return;
	if (channel->hasKey() && (args.size() < 2 || args[1] != channel->getKey()))
		throw MessageException(ERR_BADCHANNELKEY, &_client, args[0]);
	if (channel->isInviteOnly() && !channel->isInvited(&_client))
		throw MessageException(ERR_INVITEONLYCHAN, &_client, args[0]);
	if (channel->hasUserLimit() && channel->getMemberCount() >= channel->getUserLimit())
		throw MessageException(ERR_CHANNELISFULL, &_client, args[0]);
	channel->addMember(&_client);
	if (created)
		channel->addOperator(&_client);
	else
		channel->revokeInvite(&_client);
	const std::string join = prefixOf(_client) + " JOIN :" + channel->getName() + "\r\n";
	broadcast(_server, channel->getMembers(), join);
	std::ostringstream reply;
	if (channel->getTopic().empty())
		reply << ":ft_irc 331 " << nickOf(_client) << " " << channel->getName()
			<< " :No topic is set\r\n";
	else
		reply << ":ft_irc 332 " << nickOf(_client) << " " << channel->getName()
			<< " :" << channel->getTopic() << "\r\n";
	reply << ":ft_irc 353 " << nickOf(_client) << " = " << channel->getName() << " :";
	const std::vector<Client *> &members = channel->getMembers();
	for (std::size_t i = 0; i < members.size(); ++i) {
		if (i)
			reply << " ";
		if (channel->isOperator(members[i]))
			reply << "@";
		reply << members[i]->getNickname();
	}
	reply << "\r\n:ft_irc 366 " << nickOf(_client) << " " << channel->getName()
		<< " :End of /NAMES list.\r\n";
	_server.sendToClient(&_client, reply.str());
}

void Dispatcher::handlePrivmsg() {
	if (!_client.isRegistered())
		throw MessageException(ERR_NOTREGISTERED, &_client);
	const std::vector<std::string> args = _message.getArgs();
	const std::string &target = args[0];
	const std::string line = prefixOf(_client) + " PRIVMSG " + target + " :"
		+ args[1] + "\r\n";
	if (!target.empty() && (target[0] == '#' || target[0] == '&')) {
		Channel *channel = _server.getChannelByName(target);
		if (!channel)
			throw MessageException(ERR_NOSUCHCHANNEL, &_client, target);
		if (!channel->hasMember(&_client))
			throw MessageException(ERR_CANNOTSENDTOCHAN, &_client, target);
		const std::vector<Client *> &members = channel->getMembers();
		for (std::size_t i = 0; i < members.size(); ++i)
			if (members[i] != &_client)
				_server.sendToClient(members[i], line);
		return;
	}
	const std::vector<Client *> &clients = _server.getClients();
	for (std::size_t i = 0; i < clients.size(); ++i) {
		if (!clients[i]->getNickname().empty()
			&& Utils::equalsIgnoreCase(clients[i]->getNickname(), target)) {
			_server.sendToClient(clients[i], line);
			return;
		}
	}
	throw MessageException(ERR_NOSUCHNICK, &_client, target);
}

void Dispatcher::handleKick() {
	if (!_client.isRegistered())
		throw MessageException(ERR_NOTREGISTERED, &_client);
	const std::vector<std::string> args = _message.getArgs();
	Channel *channel = _server.getChannelByName(args[0]);
	if (!channel)
		throw MessageException(ERR_NOSUCHCHANNEL, &_client, args[0]);
	if (!channel->hasMember(&_client))
		throw MessageException(ERR_NOTONCHANNEL, &_client, args[0]);
	if (!channel->isOperator(&_client))
		throw MessageException(ERR_CHANOPRIVSNEEDED, &_client, args[0]);
	Client *target = NULL;
	const std::vector<Client *> &clients = _server.getClients();
	for (std::size_t i = 0; i < clients.size(); ++i)
		if (Utils::equalsIgnoreCase(clients[i]->getNickname(), args[1]))
			target = clients[i];
	if (!target || !channel->hasMember(target))
		throw MessageException(ERR_USERNOTINCHANNEL, &_client, args[1] + " " + args[0]);
	const std::string reason = args.size() > 2 ? args[2] : _client.getNickname();
	const std::string line = prefixOf(_client) + " KICK " + args[0] + " "
		+ target->getNickname() + " :" + reason + "\r\n";
	broadcast(_server, channel->getMembers(), line);
	channel->removeMember(target);
	if (channel->getMemberCount() == 0) {
		for (std::size_t i = 0; i < _server._channels.size(); ++i)
			if (_server._channels[i] == channel) {
				delete channel;
				_server._channels.erase(_server._channels.begin() + i);
				break;
			}
	}
}

void Dispatcher::handleInvite() {
	if (!_client.isRegistered())
		throw MessageException(ERR_NOTREGISTERED, &_client);
	const std::vector<std::string> args = _message.getArgs();
	Channel *channel = _server.getChannelByName(args[1]);
	if (!channel)
		throw MessageException(ERR_NOSUCHCHANNEL, &_client, args[1]);
	if (!channel->hasMember(&_client))
		throw MessageException(ERR_NOTONCHANNEL, &_client, args[1]);
	Client *target = NULL;
	const std::vector<Client *> &clients = _server.getClients();
	for (std::size_t i = 0; i < clients.size(); ++i)
		if (Utils::equalsIgnoreCase(clients[i]->getNickname(), args[0]))
			target = clients[i];
	if (!target)
		throw MessageException(ERR_NOSUCHNICK, &_client, args[0]);
	if (channel->hasMember(target))
		throw MessageException(ERR_USERONCHANNEL, &_client, args[0] + " " + args[1]);
	if (!channel->isOperator(&_client))
		throw MessageException(ERR_CHANOPRIVSNEEDED, &_client, args[1]);
	channel->invite(target);
	std::ostringstream reply;
	reply << ":ft_irc 341 " << nickOf(_client) << " " << target->getNickname()
		<< " " << channel->getName() << "\r\n";
	_server.sendToClient(&_client, reply.str());
	_server.sendToClient(target, prefixOf(_client) + " INVITE "
		+ target->getNickname() + " :" + channel->getName() + "\r\n");
}

void Dispatcher::handleTopic() {
	if (!_client.isRegistered())
		throw MessageException(ERR_NOTREGISTERED, &_client);
	const std::vector<std::string> args = _message.getArgs();
	Channel *channel = _server.getChannelByName(args[0]);
	if (!channel)
		throw MessageException(ERR_NOSUCHCHANNEL, &_client, args[0]);
	if (!channel->hasMember(&_client))
		throw MessageException(ERR_NOTONCHANNEL, &_client, args[0]);
	if (args.size() == 1) {
		std::ostringstream reply;
		if (channel->getTopic().empty())
			reply << ":ft_irc 331 " << nickOf(_client) << " " << channel->getName()
				<< " :No topic is set\r\n";
		else
			reply << ":ft_irc 332 " << nickOf(_client) << " " << channel->getName()
				<< " :" << channel->getTopic() << "\r\n";
		_server.sendToClient(&_client, reply.str());
		return;
	}
	if (channel->isTopicOperatorOnly() && !channel->isOperator(&_client))
		throw MessageException(ERR_CHANOPRIVSNEEDED, &_client, args[0]);
	channel->setTopic(args[1]);
	broadcast(_server, channel->getMembers(), prefixOf(_client) + " TOPIC "
		+ channel->getName() + " :" + args[1] + "\r\n");
}

void Dispatcher::handleMode() {
	if (!_client.isRegistered())
		throw MessageException(ERR_NOTREGISTERED, &_client);
	const std::vector<std::string> args = _message.getArgs();
	Channel *channel = _server.getChannelByName(args[0]);
	if (!channel && Utils::equalsIgnoreCase(args[0], _client.getNickname())) {
		if (args.size() == 1)
			_server.sendToClient(&_client, ":ft_irc 221 " + nickOf(_client) + " +\r\n");
		else
			_server.sendToClient(&_client, ":ft_irc 501 " + nickOf(_client)
				+ " :Unknown MODE flag\r\n");
		return;
	}
	if (!channel)
		throw MessageException(ERR_NOSUCHCHANNEL, &_client, args[0]);
	if (args.size() == 1) {
		std::ostringstream reply;
		reply << ":ft_irc 324 " << nickOf(_client) << " " << channel->getName()
			<< " " << channel->getChannelModes();
		if (channel->hasKey())
			reply << " " << channel->getKey();
		if (channel->hasUserLimit())
			reply << " " << channel->getUserLimit();
		reply << "\r\n";
		_server.sendToClient(&_client, reply.str());
		return;
	}
	if (!channel->hasMember(&_client))
		throw MessageException(ERR_NOTONCHANNEL, &_client, args[0]);
	if (!channel->isOperator(&_client))
		throw MessageException(ERR_CHANOPRIVSNEEDED, &_client, args[0]);
	const std::string &mode = args[1];
	char checkSign = '+';
	std::size_t checkParam = 2;
	const std::vector<Client *> &clients = _server.getClients();
	for (std::string::size_type i = 0; i < mode.size(); ++i) {
		if (mode[i] == '+' || mode[i] == '-') {
			checkSign = mode[i];
			continue;
		}
		if (mode[i] == 'k') {
			if (checkSign == '+' && channel->hasKey())
				throw MessageException(ERR_KEYSET, &_client, channel->getName());
			++checkParam;
		} else if (mode[i] == 'l' && checkSign == '+') {
			std::size_t limit = 0;
			if (!parsePositive(args[checkParam], limit))
				throw MessageException(ERR_INVALIDMODEPARAM, &_client, "l");
			++checkParam;
		} else if (mode[i] == 'o') {
			Client *target = NULL;
			for (std::size_t j = 0; j < clients.size(); ++j)
				if (Utils::equalsIgnoreCase(clients[j]->getNickname(), args[checkParam]))
					target = clients[j];
			if (!target)
				throw MessageException(ERR_NOSUCHNICK, &_client, args[checkParam]);
			if (!channel->hasMember(target))
				throw MessageException(ERR_USERNOTINCHANNEL, &_client, args[checkParam] + " " + args[0]);
			++checkParam;
		}
	}
	char sign = '+';
	std::size_t param = 2;
	std::string applied;
	std::string params;
	for (std::string::size_type i = 0; i < mode.size(); ++i) {
		if (mode[i] == '+' || mode[i] == '-') {
			sign = mode[i];
			if (applied.empty() || applied[applied.size() - 1] == '+' || applied[applied.size() - 1] == '-')
				applied += sign;
			else
				applied += sign;
			continue;
		}
		if (mode[i] == 'i')
			channel->setInviteOnly(sign == '+');
		else if (mode[i] == 't')
			channel->setTopicOperatorOnly(sign == '+');
		else if (mode[i] == 'k') {
			if (sign == '+') {
				channel->setKey(args[param]);
				params += " " + args[param++];
			} else {
				channel->clearKey();
				params += " " + args[param];
				++param;
			}
		} else if (mode[i] == 'l') {
			if (sign == '+') {
				std::size_t limit = 0;
				parsePositive(args[param], limit);
				channel->setUserLimit(limit);
				params += " " + args[param++];
			} else
				channel->clearUserLimit();
		} else if (mode[i] == 'o') {
			Client *target = NULL;
			for (std::size_t j = 0; j < clients.size(); ++j)
				if (Utils::equalsIgnoreCase(clients[j]->getNickname(), args[param]))
					target = clients[j];
			if (sign == '+')
				channel->addOperator(target);
			else
				channel->removeOperator(target);
			params += " " + args[param++];
		}
		applied += mode[i];
	}
	broadcast(_server, channel->getMembers(), prefixOf(_client) + " MODE "
		+ channel->getName() + " " + applied + params + "\r\n");
}

void Dispatcher::handleUnknown() {
	throw MessageException(ERR_UNKNOWNCOMMAND, &_client, _message.getCommandName());
}

void Dispatcher::handleCap() {
	const std::vector<std::string> args = _message.getArgs();
	if (args.empty())
		return;
	if (Utils::equalsIgnoreCase(args[0], "LS"))
		_server.sendToClient(&_client, ":ft_irc CAP " + nickOf(_client) + " LS :\r\n");
	else if (Utils::equalsIgnoreCase(args[0], "REQ")) {
		std::string requested = args.size() > 1 ? args[1] : std::string();
		_server.sendToClient(&_client, ":ft_irc CAP " + nickOf(_client) + " NAK :" + requested + "\r\n");
	}
}

void Dispatcher::handlePing() {
	const std::vector<std::string> args = _message.getArgs();
	if (args.empty())
		throw MessageException(ERR_NEEDMOREPARAMS, &_client, "PING");
	_server.sendToClient(&_client, ":ft_irc PONG ft_irc :" + args[0] + "\r\n");
}

double Dispatcher::calculate(const std::string &input)
{
    std::istringstream iss(input);
    double a;
    double b;
    char op;
    char extra;

    if (!(iss >> a >> op >> b) || (iss >> extra))
        throw std::runtime_error("Invalid expression");

    if (op == '+')
        return a + b;
    if (op == '-')
        return a - b;
    if (op == '*')
        return a * b;
    if (op == '/')
    {
        if (b == 0)
            throw std::runtime_error("Division by zero");
        return a / b;
    }

    throw std::runtime_error("Unknown operator");
}
void Dispatcher::handleBot() {
	const std::vector<std::string> args = _message.getArgs();
	if(args.empty())
		return;
	std::string result;
	try {
		std::ostringstream out;
		out << calculate(args[0]);
		result = out.str();
	} catch (...) {
		result = "error";
	}
	_server.sendToClient(&_client, "calculator responded to " + nickOf(_client) + " your result : " + result + "\r\n");
}
void Dispatcher::dispatch() {
	typedef std::pair<Command_t, Handler> Entry;
	static const Entry entries[] = {
		Entry(PASS, &Dispatcher::handlePass), Entry(NICK, &Dispatcher::handleNick),
		Entry(USER, &Dispatcher::handleUser), Entry(JOIN, &Dispatcher::handleJoin),
		Entry(PRIVMSG, &Dispatcher::handlePrivmsg), Entry(KICK, &Dispatcher::handleKick),
		Entry(INVITE, &Dispatcher::handleInvite), Entry(TOPIC, &Dispatcher::handleTopic),
		Entry(MODE, &Dispatcher::handleMode), Entry(CAP, &Dispatcher::handleCap),
		Entry(PING, &Dispatcher::handlePing), Entry(UNKNOWN, &Dispatcher::handleUnknown),
		Entry(BOT, &Dispatcher::handleBot)
	};
	static const HandlerMap handlers(entries, entries + sizeof(entries) / sizeof(entries[0]));
	HandlerMap::const_iterator it = handlers.find(_message.getType());
	if (it != handlers.end())
		(this->*(it->second))();
}
