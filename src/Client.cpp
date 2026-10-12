#include "Client.hpp"

Client::Client(int fd)
	: _fd(fd), _buff(), _nickname(), _username(), _realname(), _sendBuff(),
	  _isAuth(false), _isRegistered(false), _shouldDisconnect(false),
	  _hostname("localhost") {}

Client::~Client() {}

int Client::getFd() const { return _fd; }
std::string &Client::getBuff() { return _buff; }

void Client::setNickname(const std::string &nickname) { _nickname = nickname; }
const std::string &Client::getNickname() const { return _nickname; }

void Client::setUserName(const std::string &username) { _username = username; }
const std::string &Client::getUsername() const { return _username; }

void Client::setRealname(const std::string &realname) { _realname = realname; }
const std::string &Client::getRealname() const { return _realname; }

void Client::setisAuth(bool isAuth) { _isAuth = isAuth; }
bool Client::isAuth() const { return _isAuth; }

void Client::setRegister(bool isRegistered) { _isRegistered = isRegistered; }
bool Client::isRegistered() const { return _isRegistered; }

void Client::markForDisconnect() { _shouldDisconnect = true; }
bool Client::shouldDisconnect() const { return _shouldDisconnect; }

std::string &Client::getSendBuff() { return _sendBuff; }

void Client::setHostname(const std::string &hostname) { _hostname = hostname; }
const std::string &Client::getHostname() const { return _hostname; }
