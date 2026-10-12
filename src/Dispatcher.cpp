#include "Dispatcher.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "Message.hpp"
#include "MessageException.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <map>
#include <stdexcept>
#include <sstream>
#include <vector>

static void broadcast(Server &server, const std::vector<Client *> &members,
		const std::string &line) {
	for (std::size_t i = 0; i < members.size(); ++i)
		server.sendToClient(members[i], line);
}

static bool parsePositive(const std::string &text, std::size_t &value) {
	static const std::size_t MAX_CHANNEL_USER_LIMIT = 1000000;
	if (text.empty())
		return false;
	std::size_t result = 0;
	for (std::string::size_type i = 0; i < text.size(); ++i) {
		if (text[i] < '0' || text[i] > '9')
			return false;
		const std::size_t digit = static_cast<std::size_t>(text[i] - '0');
		if (result > (MAX_CHANNEL_USER_LIMIT - digit) / 10)
			return false;
		result = result * 10 + digit;
	}
	if (result == 0)
		return false;
	value = result;
	return true;
}

struct ModeOperation {
	char flag;
	bool enabled;
	std::string parameter;

	ModeOperation(char modeFlag, bool isEnabled, const std::string &value)
		: flag(modeFlag), enabled(isEnabled), parameter(value) {}
};

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
	const std::string identity = _client.getNickname() + "!" + _client.getUsername()
		+ "@" + _client.getHostname();
	_server.sendToClient(&_client, Utils::composeNumericReply(_client, 001, "",
		"Welcome to the ft_irc Network " + identity));
	_server.sendToClient(&_client, Utils::composeNumericReply(_client, 002, "",
		"Your host is ft_irc, running version 1.0"));
	_server.sendToClient(&_client, Utils::composeNumericReply(_client, 003, "",
		"This server was created for the 42 ft_irc project"));
	_server.sendToClient(&_client, Utils::composeNumericReply(_client, 004,
		"ft_irc 1.0 itkol"));
}

bool Dispatcher::isNicknameInUse(const std::string &nickname) const {
	const std::vector<Client *> &clients = _server.getClients();
	for (std::size_t i = 0; i < clients.size(); ++i)
		if (clients[i] != &_client && !clients[i]->getNickname().empty()
			&& Utils::equalsIgnoreCase(clients[i]->getNickname(), nickname))
			return true;
	return false;
}

Client *Dispatcher::findClientByNickname(const std::string &nickname) const {
	const std::vector<Client *> &clients = _server.getClients();
	for (std::size_t i = 0; i < clients.size(); ++i)
		if (!clients[i]->getNickname().empty()
			&& Utils::equalsIgnoreCase(clients[i]->getNickname(), nickname))
			return clients[i];
	return NULL;
}

void Dispatcher::handlePass() {
	if (_client.isAuth() || _client.isRegistered())
		throw MessageException(ERR_ALREADYREGISTRED, &_client);
	const std::vector<std::string> &args = _message.getArgs();
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
		const std::string change = Utils::composeClientReply(_client, "NICK", "", nickname);
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
	const std::vector<std::string> &args = _message.getArgs();
	if (! _client.getUsername().empty())
		throw MessageException(ERR_ALREADYREGISTRED, &_client);
	_client.setUserName(args[0]);
	_client.setRealname(args[3]);
	tryCompleteRegistration();
}

void Dispatcher::handleJoin() {
	if (!_client.isRegistered())
		throw MessageException(ERR_NOTREGISTERED, &_client);
	const std::vector<std::string> &args = _message.getArgs();
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
	const std::string join = Utils::composeClientReply(_client, "JOIN", "", channel->getName());
	broadcast(_server, channel->getMembers(), join);
	if (channel->getTopic().empty())
		_server.sendToClient(&_client, Utils::composeNumericReply(_client, 331,
			channel->getName(), "No topic is set"));
	else
		_server.sendToClient(&_client, Utils::composeNumericReply(_client, 332,
			channel->getName(), channel->getTopic()));
	std::string names;
	const std::vector<Client *> &members = channel->getMembers();
	for (std::size_t i = 0; i < members.size(); ++i) {
		if (i)
			names += " ";
		if (channel->isOperator(members[i]))
			names += "@";
		names += members[i]->getNickname();
	}
	_server.sendToClient(&_client, Utils::composeNumericReply(_client, 353,
		"= " + channel->getName(), names));
	_server.sendToClient(&_client, Utils::composeNumericReply(_client, 366,
		channel->getName(), "End of /NAMES list."));
}

void Dispatcher::handlePrivmsg() {
	if (!_client.isRegistered())
		throw MessageException(ERR_NOTREGISTERED, &_client);
	const std::vector<std::string> &args = _message.getArgs();
	const std::string &target = args[0];
	const std::string line = Utils::composeClientReply(_client, "PRIVMSG", target, args[1]);
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
	const std::vector<std::string> &args = _message.getArgs();
	Channel *channel = _server.getChannelByName(args[0]);
	if (!channel)
		throw MessageException(ERR_NOSUCHCHANNEL, &_client, args[0]);
	if (!channel->hasMember(&_client))
		throw MessageException(ERR_NOTONCHANNEL, &_client, args[0]);
	if (!channel->isOperator(&_client))
		throw MessageException(ERR_CHANOPRIVSNEEDED, &_client, args[0]);
	Client *target = findClientByNickname(args[1]);
	if (!target || !channel->hasMember(target))
		throw MessageException(ERR_USERNOTINCHANNEL, &_client, args[1] + " " + args[0]);
	const std::string reason = args.size() > 2 ? args[2] : _client.getNickname();
	const std::string line = Utils::composeClientReply(_client, "KICK",
		args[0] + " " + target->getNickname(), reason);
	broadcast(_server, channel->getMembers(), line);
	_server.removeChannelMember(channel, target);
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
	const std::vector<std::string> &args = _message.getArgs();
	Channel *channel = _server.getChannelByName(args[1]);
	if (!channel)
		throw MessageException(ERR_NOSUCHCHANNEL, &_client, args[1]);
	if (!channel->hasMember(&_client))
		throw MessageException(ERR_NOTONCHANNEL, &_client, args[1]);
	Client *target = findClientByNickname(args[0]);
	if (!target)
		throw MessageException(ERR_NOSUCHNICK, &_client, args[0]);
	if (channel->hasMember(target))
		throw MessageException(ERR_USERONCHANNEL, &_client, args[0] + " " + args[1]);
	if (!channel->isOperator(&_client))
		throw MessageException(ERR_CHANOPRIVSNEEDED, &_client, args[1]);
	channel->invite(target);
	_server.sendToClient(&_client, Utils::composeNumericReply(_client, 341,
		target->getNickname() + " " + channel->getName()));
	_server.sendToClient(target, Utils::composeClientReply(_client, "INVITE",
		target->getNickname(), channel->getName()));
}

void Dispatcher::handleTopic() {
	if (!_client.isRegistered())
		throw MessageException(ERR_NOTREGISTERED, &_client);
	const std::vector<std::string> &args = _message.getArgs();
	Channel *channel = _server.getChannelByName(args[0]);
	if (!channel)
		throw MessageException(ERR_NOSUCHCHANNEL, &_client, args[0]);
	if (!channel->hasMember(&_client))
		throw MessageException(ERR_NOTONCHANNEL, &_client, args[0]);
	if (args.size() == 1) {
		if (channel->getTopic().empty())
			_server.sendToClient(&_client, Utils::composeNumericReply(_client, 331,
				channel->getName(), "No topic is set"));
		else
			_server.sendToClient(&_client, Utils::composeNumericReply(_client, 332,
				channel->getName(), channel->getTopic()));
		return;
	}
	if (channel->isTopicOperatorOnly() && !channel->isOperator(&_client))
		throw MessageException(ERR_CHANOPRIVSNEEDED, &_client, args[0]);
	channel->setTopic(args[1]);
	broadcast(_server, channel->getMembers(), Utils::composeClientReply(_client,
		"TOPIC", channel->getName(), args[1]));
}

void Dispatcher::applyInviteOnlyMode(Channel &channel, bool enabled,
		const std::string &parameter, std::string &parameters) {
	(void)parameter;
	(void)parameters;
	channel.setInviteOnly(enabled);
}

void Dispatcher::applyTopicOnlyMode(Channel &channel, bool enabled,
		const std::string &parameter, std::string &parameters) {
	(void)parameter;
	(void)parameters;
	channel.setTopicOperatorOnly(enabled);
}

void Dispatcher::applyKeyMode(Channel &channel, bool enabled,
		const std::string &parameter, std::string &parameters) {
	if (enabled)
		channel.setKey(parameter);
	else
		channel.clearKey();
	parameters += " " + parameter;
}

void Dispatcher::applyLimitMode(Channel &channel, bool enabled,
		const std::string &parameter, std::string &parameters) {
	if (!enabled) {
		channel.clearUserLimit();
		return;
	}
	std::size_t limit = 0;
	if (!parsePositive(parameter, limit))
		throw MessageException(ERR_INVALIDMODEPARAM, &_client, "l");
	channel.setUserLimit(limit);
	parameters += " " + parameter;
}

void Dispatcher::applyOperatorMode(Channel &channel, bool enabled,
		const std::string &parameter, std::string &parameters) {
	Client *target = findClientByNickname(parameter);
	if (enabled)
		channel.addOperator(target);
	else
		channel.removeOperator(target);
	parameters += " " + parameter;
}

void Dispatcher::handleMode() {
	if (!_client.isRegistered())
		throw MessageException(ERR_NOTREGISTERED, &_client);
	const std::vector<std::string> &args = _message.getArgs();
	Channel *channel = _server.getChannelByName(args[0]);
	if (!channel && Utils::equalsIgnoreCase(args[0], _client.getNickname())) {
		if (args.size() == 1)
			_server.sendToClient(&_client, Utils::composeNumericReply(_client, 221, "+"));
		else
			_server.sendToClient(&_client, Utils::composeNumericReply(_client, 501,
				"", "Unknown MODE flag"));
		return;
	}
	if (!channel)
		throw MessageException(ERR_NOSUCHCHANNEL, &_client, args[0]);
	if (args.size() == 1) {
		sendChannelModes(*channel);
		return;
	}
	if (!channel->hasMember(&_client))
		throw MessageException(ERR_NOTONCHANNEL, &_client, args[0]);
	if (!channel->isOperator(&_client))
		throw MessageException(ERR_CHANOPRIVSNEEDED, &_client, args[0]);
	applyChannelModes(*channel, args);
}

void Dispatcher::sendChannelModes(Channel &channel) {
	std::string parameters = channel.getName() + " " + channel.getChannelModes();
	if (channel.hasKey())
		parameters += " " + channel.getKey();
	if (channel.hasUserLimit()) {
		std::ostringstream limit;
		limit << channel.getUserLimit();
		parameters += " " + limit.str();
	}
	_server.sendToClient(&_client, Utils::composeNumericReply(_client, 324, parameters));
}

void Dispatcher::applyChannelModes(Channel &channel,
		const std::vector<std::string> &args) {
	const std::string &mode = args[1];
	typedef std::pair<char, ModeHandler> ModeEntry;
	typedef std::map<char, ModeHandler> ModeHandlerMap;
	static const ModeEntry modeEntries[] = {
		ModeEntry('i', &Dispatcher::applyInviteOnlyMode),
		ModeEntry('t', &Dispatcher::applyTopicOnlyMode),
		ModeEntry('k', &Dispatcher::applyKeyMode),
		ModeEntry('l', &Dispatcher::applyLimitMode),
		ModeEntry('o', &Dispatcher::applyOperatorMode)
	};
	static const ModeHandlerMap modeHandlers(modeEntries,
		modeEntries + sizeof(modeEntries) / sizeof(modeEntries[0]));
	std::vector<ModeOperation> operations;
	char sign = '+';
	std::size_t parameterIndex = 2;
	std::string applied;
	for (std::string::size_type i = 0; i < mode.size(); ++i) {
		if (mode[i] == '+' || mode[i] == '-') {
			sign = mode[i];
			applied += sign;
			continue;
		}
		ModeHandlerMap::const_iterator handler = modeHandlers.find(mode[i]);
		if (handler == modeHandlers.end())
			throw MessageException(ERR_UNKNOWNMODE, &_client, std::string(1, mode[i]));
		const bool needsParameter = mode[i] == 'k' || mode[i] == 'o'
			|| (mode[i] == 'l' && sign == '+');
		std::string parameter;
		if (needsParameter) {
			if (parameterIndex >= args.size())
				throw MessageException(ERR_INVALIDMODEPARAM, &_client, args[1]);
			parameter = args[parameterIndex++];
		}
		if (mode[i] == 'k' && parameter.empty())
			throw MessageException(ERR_INVALIDMODEPARAM, &_client, "k");
		if (mode[i] == 'k' && sign == '+' && channel.hasKey())
			throw MessageException(ERR_KEYSET, &_client, channel.getName());
		if (mode[i] == 'l' && sign == '+') {
			std::size_t limit = 0;
			if (!parsePositive(parameter, limit))
				throw MessageException(ERR_INVALIDMODEPARAM, &_client, "l");
		}
		if (mode[i] == 'o') {
			Client *target = findClientByNickname(parameter);
			if (!target)
				throw MessageException(ERR_NOSUCHNICK, &_client, parameter);
			if (!channel.hasMember(target))
				throw MessageException(ERR_USERNOTINCHANNEL, &_client, parameter + " " + args[0]);
		}
		operations.push_back(ModeOperation(mode[i], sign == '+', parameter));
		applied += mode[i];
	}
	std::string params;
	for (std::size_t i = 0; i < operations.size(); ++i) {
		ModeHandlerMap::const_iterator handler = modeHandlers.find(operations[i].flag);
		(this->*(handler->second))(channel, operations[i].enabled,
			operations[i].parameter, params);
	}
	broadcast(_server, channel.getMembers(), Utils::composeClientReply(_client,
		"MODE", channel.getName() + " " + applied + params));
}

void Dispatcher::handleUnknown() {
	throw MessageException(ERR_UNKNOWNCOMMAND, &_client, _message.getCommandName());
}

void Dispatcher::handleCap() {
	const std::vector<std::string> &args = _message.getArgs();
	if (args.empty())
		return;
	if (Utils::equalsIgnoreCase(args[0], "LS"))
		_server.sendToClient(&_client, Utils::composeServerReply("CAP",
			(_client.getNickname().empty() ? "*" : _client.getNickname()) + " LS", ""));
	else if (Utils::equalsIgnoreCase(args[0], "REQ")) {
		std::string requested = args.size() > 1 ? args[1] : std::string();
		_server.sendToClient(&_client, Utils::composeServerReply("CAP",
			(_client.getNickname().empty() ? "*" : _client.getNickname()) + " NAK", requested));
	}
}

void Dispatcher::handlePing() {
	const std::vector<std::string> &args = _message.getArgs();
	if (args.empty())
		throw MessageException(ERR_NEEDMOREPARAMS, &_client, "PING");
	_server.sendToClient(&_client, Utils::composeServerReply("PONG", "ft_irc", args[0]));
}

static double addValues(double left, double right) { return left + right; }
static double subtractValues(double left, double right) { return left - right; }
static double multiplyValues(double left, double right) { return left * right; }

static double divideValues(double left, double right) {
	if (right == 0)
		throw std::runtime_error("Division by zero");
	return left / right;
}

double Dispatcher::calculate(const std::string &input) {
	std::istringstream iss(input);
	double left = 0;
	double right = 0;
	char op = 0;
	char extra = 0;
	if (!(iss >> left >> op >> right) || (iss >> extra))
		throw std::runtime_error("Invalid expression");
	typedef double (*Operation)(double, double);
	typedef std::pair<char, Operation> Entry;
	typedef std::map<char, Operation> OperationMap;
	static const Entry entries[] = {
		Entry('+', addValues), Entry('-', subtractValues),
		Entry('*', multiplyValues), Entry('/', divideValues)
	};
	static const OperationMap operations(entries,
		entries + sizeof(entries) / sizeof(entries[0]));
	OperationMap::const_iterator operation = operations.find(op);
	if (operation == operations.end())
		throw std::runtime_error("Unknown operator");
	return operation->second(left, right);
}
void Dispatcher::handleBot() {
	const std::vector<std::string> &args = _message.getArgs();
	if (args.empty())
		return;
	std::string result;
	try {
		std::ostringstream out;
		out << calculate(args[0]);
		result = out.str();
	} catch (const std::exception &) {
		result = "error";
	}
	_server.sendToClient(&_client, "calculator responded to " + _client.getNickname() + " your result : " + result + "\r\n");
}
void Dispatcher::dispatch() {
	if (!_client.isRegistered()) {
		typedef std::pair<Command_t, bool> RegistrationEntry;
		typedef std::map<Command_t, bool> RegistrationMap;
		static const RegistrationEntry registrationEntries[] = {
			RegistrationEntry(PASS, true), RegistrationEntry(NICK, true),
			RegistrationEntry(USER, true), RegistrationEntry(CAP, true),
			RegistrationEntry(PING, true)
		};
		static const RegistrationMap registrationCommands(registrationEntries,
			registrationEntries + sizeof(registrationEntries)
			/ sizeof(registrationEntries[0]));
		if (registrationCommands.find(_message.getType()) == registrationCommands.end())
			throw MessageException(ERR_NOTREGISTERED, &_client);
	}
	_message.validateArgs();
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
