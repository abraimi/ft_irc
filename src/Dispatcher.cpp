#include "Dispatcher.hpp"
#include "Message.hpp"
#include "MessageException.hpp"
#include "Server.hpp"
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>

Dispatcher::Dispatcher(Server& server, Client& client, Channel* channel, Message& msg)
	: _server(server), _client(client), _channel(channel), _message(msg)
{}

Dispatcher::~Dispatcher() {}

bool Dispatcher::isPasswordValid(const std::string &password) const
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

bool Dispatcher::sameNickname(const std::string &left, const std::string &right) {
	if (left.size() != right.size())
		return false;
	for (std::string::size_type i = 0; i < left.size(); ++i) {
		unsigned char leftChar = static_cast<unsigned char>(left[i]);
		unsigned char rightChar = static_cast<unsigned char>(right[i]);
		if (std::tolower(leftChar) != std::tolower(rightChar))
			return false;
	}
	return true;
}

bool Dispatcher::isNicknameInUse(const std::string &nickname) const {
	const std::vector<Client *>& clients = _server.getClients();
	for (std::size_t i = 0; i < clients.size(); ++i)
		if (clients[i] != &_client && !clients[i]->getNickname().empty()
			&& sameNickname(clients[i]->getNickname(), nickname))
			return true;
	return false;
}

void Dispatcher::handlePass() {
	if (_client.isAuth() || _client.isRegistered())
		throw MessageException(ERR_ALREADYREGISTRED, &_client);

	std::vector<std::string> args = _message.getArgs();
	if (!isPasswordValid(args[0]))
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

void Dispatcher::handleUnknown() {
	throw MessageException(ERR_UNKNOWNCOMMAND, &_client, _message.getCommandName());
}

void Dispatcher::dispatch() {
	typedef std::pair<Command_t, Handler> Entry;

	static const Entry entries[] = {
		Entry(PASS, &Dispatcher::handlePass),
		Entry(NICK, &Dispatcher::handleNick),
		Entry(USER, &Dispatcher::handleUser),
		Entry(UNKNOWN, &Dispatcher::handleUnknown)
	};
	static const HandlerMap handlers(entries, entries + sizeof(entries) / sizeof(entries[0]));
	HandlerMap::const_iterator it =	handlers.find(_message.getType());

	std::cout << "Command Type: " << _message.getType() << std::endl;	

	if (it == handlers.end())
		throw std::runtime_error("No dispatcher handler registered for command");
	(this->*(it->second))();
}
