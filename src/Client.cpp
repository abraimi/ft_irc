#include "Client.hpp"

Client::Client(int fd)
	: _fd(fd), _buff(), _nickname(), _username(), _realname(), _sendBuff(),
	  _isAuth(false), _isRegistered(false), _hostname("localhost") {}

Client::~Client() {}

int Client::getFd() const { return _fd; }
std::string &Client::getBuff() { return _buff; }

void Client::setNickname(const std::string &nickname) { _nickname = nickname; }
std::string Client::getNickname() const { return _nickname; }

void Client::setUserName(const std::string &username) { _username = username; }
std::string Client::getUsername() const { return _username; }

void Client::setRealname(const std::string &realname) { _realname = realname; }
std::string Client::getRealname() const { return _realname; }

void Client::setisAuth(bool isAuth) { _isAuth = isAuth; }
bool Client::isAuth() const { return _isAuth; }

void Client::setRegister(bool isRegistered) { _isRegistered = isRegistered; }
bool Client::isRegistered() const { return _isRegistered; }

std::string &Client::getSendBuff() { return _sendBuff; }

void Client::setHostname(const std::string &hostname) { _hostname = hostname; }
std::string Client::getHostname() const { return _hostname; }
