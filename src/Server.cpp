#include "Server.hpp"
#include "Dispatcher.hpp"
#include "Message.hpp"
#include "MessageException.hpp"
#include "Utils.hpp"
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

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
	for (std::size_t i = 0; i < _clients.size(); ++i) {
		if (_clients[i]->getFd() != fd)
			continue;
		Client *client = _clients[i];
		for (std::size_t j = 0; j < _channels.size();) {
			_channels[j]->removeMember(client);
			_channels[j]->revokeInvite(client);
			if (_channels[j]->getMemberCount() == 0) {
				delete _channels[j];
				_channels.erase(_channels.begin() + j);
			} else {
				++j;
			}
		}
		delete client;
		_clients.erase(_clients.begin() + i);
		return;
	}
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
		_channels.push_back(channel);
	}
	return channel;
}

void Server::sendToClient(Client *client, const std::string &msg) {
	if (!client || msg.empty())
		return;
	client->getSendBuff().append(msg);
	enableClientWrite(client);
}

void Server::processMessage(Client *client, const std::string &line) {
	Message message(client);
	try {
		message.parseMessage(line);
		if (!message.getValid())
			return;
		message.validateArgs();
		Dispatcher dispatcher(*this, *client, message);
		dispatcher.dispatch();
	} catch (const MessageException &error) {
		sendToClient(client, error.what());
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
		|| listen(_server_fd, 10) == -1) {
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
	_pollfds.push_back(clientPoll);
	_clients.push_back(new Client(fd));
}

bool Server::handleClientRead(std::size_t index) {
	int fd = _pollfds[index].fd;
	char buffer[4096];
	ssize_t count = recv(fd, buffer, sizeof(buffer), 0);
	if (count == 0) {
		close(fd);
		removeClient(fd);
		_pollfds.erase(_pollfds.begin() + index);
		return false;
	}
	if (count < 0)
		return true;
	Client *client = getClientByFd(fd);
	if (!client)
		return true;
	std::string &input = client->getBuff();
	input.append(buffer, static_cast<std::size_t>(count));
	std::string::size_type end = input.find("\r\n");
	while (end != std::string::npos) {
		std::string line = input.substr(0, end);
		input.erase(0, end + 2);
		if (line.size() <= 510)
			processMessage(client, line);
		else
			sendToClient(client, ":ft_irc 417 " + (client->getNickname().empty() ? "*" : client->getNickname()) + " :Input line was too long\r\n");
		if (getClientByFd(fd) != client)
			return false;
		end = input.find("\r\n");
	}
	if (input.size() > 512) {
		input.clear();
		sendToClient(client, ":ft_irc 417 " + (client->getNickname().empty() ? "*" : client->getNickname()) + " :Input line was too long\r\n");
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
		close(fd);
		removeClient(fd);
		_pollfds.erase(_pollfds.begin() + index);
		return false;
	}
	if (output.empty())
		_pollfds[index].events &= static_cast<short>(~POLLOUT);
	return true;
}

void Server::run() {
	while (true) {
		int ready = poll(&_pollfds[0], _pollfds.size(), -1);
		if (ready < 0) {
			if (errno == EINTR)
				continue;
			throw std::runtime_error("poll() failed");
		}
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
				close(fd);
				removeClient(fd);
				_pollfds.erase(_pollfds.begin() + i);
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
	}
}

void Server::start() {
	signal(SIGPIPE, SIG_IGN);
	setUpSocket();
	run();
}
