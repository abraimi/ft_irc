#include "../include/Client.hpp"

Client::Client(int fd) : _fd(fd), _buff(""), _nickname(""), _username(""), _sendBuff(""), _isAuth(false), _isRegistered(false), _hostname("localhost") {}
Client::~Client() {}

int Client::getFd() const
{
    return _fd;
}

std::string &Client::getBuff()
{
    return _buff;
}

void Client::setNickname(const std::string &nickname)
{
    _nickname = nickname;
}

std::string Client::getNickname() const
{
    return _nickname;
}

void Client::setUserName(const std::string &username)
{
    _username = username;
}

std::string Client::getUsername() const
{
    return _username;
}

void Client::setisAuth(bool isAuth)
{
    _isAuth = isAuth;
}

bool Client::isAuth() const
{
    return _isAuth;
}

void Client::setRegister(bool isRegistered)
{
    _isRegistered = isRegistered;
}

bool Client::isRegistered() const
{
    return _isRegistered;
}

std::string &Client::getSendBuff()
{
    return _sendBuff;
}

void Client::setHostname(const std::string &hostname)
{
    _hostname = hostname;
}

std::string Client::getHostname() const
{
    return _hostname;
}

void Client::addChannel(const std::string &name)
{
    _channels.push_back(name);
}

void Client::removeChannel(const std::string &name)
{
    for (size_t i = 0; i < _channels.size(); i++)
    {
        if (_channels[i] == name)
        {
            _channels.erase(_channels.begin() + i);
            return;
        }
    }
}

std::vector<std::string> &Client::getChannels()
{
    return _channels;
}
