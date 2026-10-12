#include "Server.hpp"
#include "Client.hpp"
#include "Dispatcher.hpp"
#include "Message.hpp"
#include "MessageException.hpp"
#include "Utils.hpp"
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <new>
#include <stdexcept>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static const std::size_t MAX_IRC_LINE_LENGTH = 512;
static const std::size_t IRC_LINE_END_LENGTH = 2;
static const std::size_t MAX_IRC_CONTENT_LENGTH =
	MAX_IRC_LINE_LENGTH - IRC_LINE_END_LENGTH;
static const std::size_t MAX_CLIENT_OUTPUT = 64 * 1024;
static const std::size_t RECV_BUFFER_SIZE = 4096;
static const int LISTEN_BACKLOG = 10;
static const int POLL_TIMEOUT_MILLISECONDS = 1000;
static volatile sig_atomic_t shutdownRequested = 0;

static void requestShutdown(int signalNumber) {
	(void)signalNumber;
	shutdownRequested = 1;
}

static std::string limitIrcLine(const std::string &message) {
	if (message.size() <= MAX_IRC_LINE_LENGTH)
		return message;
	if (message.size() < IRC_LINE_END_LENGTH
		|| message.compare(message.size() - IRC_LINE_END_LENGTH,
			IRC_LINE_END_LENGTH, "\r\n") != 0)
		return message.substr(0, MAX_IRC_LINE_LENGTH - IRC_LINE_END_LENGTH) + "\r\n";

	const std::string::size_type trailingStart = message.find(" :");
	if (trailingStart != std::string::npos
		&& trailingStart + 2 <= MAX_IRC_CONTENT_LENGTH) {
		const std::size_t trailingLength =
			MAX_IRC_CONTENT_LENGTH - trailingStart - 2;
		return message.substr(0, trailingStart + 2)
			+ message.substr(trailingStart + 2, trailingLength) + "\r\n";
	}
	return message.substr(0, MAX_IRC_CONTENT_LENGTH) + "\r\n";
}

Server::Server(int port, const std::string &password)
	: _port(port), _server_fd(-1), _password(password), _clients(), _channels(), _pollfds() {}

Server::~Server() {
	for (std::size_t i = 0; i < _clients.size(); ++i) {
		close(_clients[i]->getFd());
		delete _clients[i];
	}
	for (std::size_t i = 0; i < _channels.size(); ++i)
		delete _channels[i];
	if (_server_fd != -1)
		close(_server_fd);
}

void Server::removeClient(int fd) {
	Client *client = NULL;
	for (std::size_t i = 0; i < _clients.size(); ++i) {
		if (_clients[i]->getFd() != fd)
			continue;
		client = _clients[i];
		close(fd);
		for (std::size_t j = 0; j < _channels.size();) {
			Channel *channel = _channels[j];
			removeChannelMember(channel, client);
			channel->revokeInvite(client);
			if (channel->getMemberCount() == 0) {
				delete channel;
				_channels.erase(_channels.begin() + j);
			} else {
				++j;
			}
		}
		delete client;
		_clients.erase(_clients.begin() + i);
		break;
	}
	for (std::size_t i = 0; i < _pollfds.size(); ++i)
		if (_pollfds[i].fd == fd) {
			_pollfds.erase(_pollfds.begin() + i);
			break;
		}
}

Client *Server::removeChannelMember(Channel *channel, Client *client) {
	const bool wasOperator = channel->isOperator(client);
	if (!channel->removeMember(client) || !wasOperator)
		return NULL;
	Client *promoted = NULL;
	try {
		promoted = channel->promoteFirstMemberIfOperatorless();
	} catch (const std::bad_alloc &) {
		return NULL;
	}
	if (!promoted)
		return NULL;
	try {
		const std::string mode = Utils::composeServerReply("MODE",
			channel->getName() + " +o " + promoted->getNickname());
		const std::vector<Client *> &members = channel->getMembers();
		for (std::size_t i = 0; i < members.size(); ++i)
			sendToClient(members[i], mode);
	} catch (const std::bad_alloc &) {
		return promoted;
	}
	return promoted;
}

Client *Server::getClientByFd(int fd) {
	for (std::size_t i = 0; i < _clients.size(); ++i)
		if (_clients[i]->getFd() == fd)
			return _clients[i];
	return NULL;
}

const std::vector<Client *> &Server::getClients() const { return _clients; }

Channel *Server::getChannelByName(const std::string &name) const {
	for (std::size_t i = 0; i < _channels.size(); ++i)
		if (Utils::equalsIgnoreCase(_channels[i]->getName(), name))
			return _channels[i];
	return NULL;
}

Channel *Server::getOrCreateChannel(const std::string &name, bool &created) {
	Channel *channel = getChannelByName(name);
	created = channel == NULL;
	if (created) {
		channel = new Channel(name);
		try {
			_channels.push_back(channel);
		} catch (...) {
			delete channel;
			throw;
		}
	}
	return channel;
}

void Server::sendToClient(Client *client, const std::string &msg) {
	if (!client || msg.empty())
		return;
	if (client->shouldDisconnect())
		return;
	try {
		const std::string line = limitIrcLine(msg);
		std::string &output = client->getSendBuff();
		if (line.size() > MAX_CLIENT_OUTPUT - output.size()) {
			client->markForDisconnect();
			return;
		}
		output.append(line);
	} catch (const std::bad_alloc &) {
		client->markForDisconnect();
		return;
	}
	enableClientWrite(client);
}


void Server::processMessage(Client *client, const std::string &line) {
	try {
		Message message(client);
		message.parseMessage(line);
		if (!message.getValid())
			return;
		Dispatcher dispatcher(*this, *client, message);
		dispatcher.dispatch();
	} catch (const MessageException &error) {
		sendToClient(client, error.what());
	} catch (const std::bad_alloc &) {
		client->markForDisconnect();
	}
}

void Server::setUpSocket() {
	struct sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	int opt = 1;
	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_server_fd == -1)
		throw std::runtime_error("socket() failed");
	if (fcntl(_server_fd, F_SETFL, O_NONBLOCK) == -1
		|| setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		close(_server_fd);
		_server_fd = -1;
		throw std::runtime_error("Unable to configure listening socket");
	}
	address.sin_family = AF_INET;
	address.sin_port = htons(static_cast<unsigned short>(_port));
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(_server_fd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) == -1
		|| listen(_server_fd, LISTEN_BACKLOG) == -1) {
		close(_server_fd);
		_server_fd = -1;
		throw std::runtime_error("Unable to bind/listen on requested port");
	}
	struct pollfd listener;
	listener.fd = _server_fd;
	listener.events = POLLIN;
	listener.revents = 0;
	_pollfds.push_back(listener);
	std::cout << "IRC server listening on port " << _port << std::endl;
}

void Server::handleNewConn() {
	int fd = accept(_server_fd, NULL, NULL);
	if (fd < 0)
		return;
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		close(fd);
		return;
	}
	struct pollfd clientPoll;
	clientPoll.fd = fd;
	clientPoll.events = POLLIN;
	clientPoll.revents = 0;
	Client *client = NULL;
	try {
		client = new Client(fd);
		_clients.push_back(client);
		_pollfds.push_back(clientPoll);
	} catch (...) {
		if (client && !_clients.empty() && _clients.back() == client)
			_clients.pop_back();
		delete client;
		close(fd);
		return;
	}
}

bool Server::handleClientRead(std::size_t index) {
	int fd = _pollfds[index].fd;
	char buffer[RECV_BUFFER_SIZE];
	ssize_t count = recv(fd, buffer, sizeof(buffer), 0);
	if (count == 0) {
		removeClient(fd);
		return false;
	}
	if (count < 0) // Wait for poll to report readiness again; do not inspect errno here.
		return true;
	Client *client = getClientByFd(fd);
	if (!client)
		return true;
	std::string &input = client->getBuff();
	try {
		input.append(buffer, static_cast<std::size_t>(count));
	} catch (const std::bad_alloc &) {
		client->markForDisconnect();
		return true;
	}
	std::string::size_type end = input.find('\n');
	while (end != std::string::npos) {
		std::string line = input.substr(0, end);
		input.erase(0, end + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.size() <= MAX_IRC_CONTENT_LENGTH)
			processMessage(client, line);
		else
			sendToClient(client, Utils::composeNumericReply(*client, 417, "",
				"Input line was too long"));
		if (getClientByFd(fd) != client)
			return false;
		if (client->shouldDisconnect())
			break;
		end = input.find('\n');
	}
	if (input.size() > MAX_IRC_LINE_LENGTH) {
		input.clear();
		sendToClient(client, Utils::composeNumericReply(*client, 417, "",
			"Input line was too long"));
	}
	return true;
}

void Server::enableClientWrite(Client *client) {
	for (std::size_t i = 0; i < _pollfds.size(); ++i)
		if (_pollfds[i].fd == client->getFd()) {
			_pollfds[i].events |= POLLOUT;
			return;
		}
}

bool Server::handleClientWrite(std::size_t index) {
	Client *client = getClientByFd(_pollfds[index].fd);
	if (!client)
		return true;
	std::string &output = client->getSendBuff();
	if (output.empty()) {
		_pollfds[index].events &= static_cast<short>(~POLLOUT);
		return true;
	}
	ssize_t sent = send(client->getFd(), output.data(), output.size(), 0);
	if (sent > 0)
		output.erase(0, static_cast<std::size_t>(sent));
	else if (sent == 0) {
		int fd = client->getFd();
		removeClient(fd);
		return false;
	}
	if (sent < 0) // Keep the queued bytes and let poll drive the next attempt.
		return true;
	if (output.empty())
		_pollfds[index].events &= static_cast<short>(~POLLOUT);
	return true;
}

void Server::run() {
	while (!shutdownRequested) {
		int ready = poll(&_pollfds[0], _pollfds.size(), POLL_TIMEOUT_MILLISECONDS);
		if (ready < 0) {
			if (errno == EINTR && !shutdownRequested)
				continue;
			if (shutdownRequested)
				break;
			throw std::runtime_error("poll() failed");
		}
		if (ready == 0)
			continue;
		for (std::size_t i = 0; i < _pollfds.size();) {
			short events = _pollfds[i].revents;
			if (!events) {
				++i;
				continue;
			}
			if (_pollfds[i].fd == _server_fd) {
				if (events & POLLIN)
					handleNewConn();
				++i;
				continue;
			}
			if (events & (POLLERR | POLLHUP | POLLNVAL)) {
				int fd = _pollfds[i].fd;
				removeClient(fd);
				continue;
			}
			if ((events & POLLIN) && !handleClientRead(i))
				continue;
			if (i < _pollfds.size() && (events & POLLOUT)) {
				if (!handleClientWrite(i))
					continue;
			}
			++i;
		}
		disconnectOverflowedClients();
	}
}

void Server::disconnectOverflowedClients() {
	for (std::size_t i = 0; i < _clients.size();) {
		if (!_clients[i]->shouldDisconnect()) {
			++i;
			continue;
		}
		removeClient(_clients[i]->getFd());
	}
}

void Server::start() {
	shutdownRequested = 0;
	if (signal(SIGINT, requestShutdown) == SIG_ERR
		|| signal(SIGTERM, requestShutdown) == SIG_ERR
		|| signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		throw std::runtime_error("Unable to configure signal handlers");
	setUpSocket();
	run();
}
