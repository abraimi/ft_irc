#include "Dispatcher.hpp"
#include "Message.hpp"
#include "MessageException.hpp"
#include <stdexcept>

Dispatcher::Dispatcher(Server& server, Client& client, Channel* channel, Message& msg)
	: _server(server), _client(client), _channel(channel), _message(msg)
{}

Dispatcher::~Dispatcher() {}

void Dispatcher::handleUnknown() {
	throw MessageException(ERR_UNKNOWNCOMMAND, &_client,
		_message.getCommandName());
}

void Dispatcher::dispatch() {
	typedef std::pair<Command_t, Handler> Entry;
	static const Entry entries[] = {
		Entry(UNKNOWN, &Dispatcher::handleUnknown)
	};
	static const HandlerMap handlers(
		entries, entries + sizeof(entries) / sizeof(entries[0]));
		HandlerMap::const_iterator it =
		handlers.find(_message.getType());

	if (it == handlers.end())
		throw std::runtime_error("No dispatcher handler registered for command");
	(this->*(it->second))();
}
