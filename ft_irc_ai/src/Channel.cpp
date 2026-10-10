#include "../include/Channel.hpp"

Channel::Channel(const std::string &name) : _name(name), _topic(""), _password(""), _inviteOnly(false), _topicRestricted(false), _userLimit(0), _hasUserLimit(false) {}
Channel::~Channel() {}

std::string Channel::getName() const
{
    return _name;
}

void Channel::setTopic(const std::string &topic)
{
    _topic = topic;
}

std::string Channel::getTopic() const
{
    return _topic;
}

void Channel::setPassword(const std::string &password)
{
    _password = password;
}

std::string Channel::getPassword() const
{
    return _password;
}

bool Channel::hasPassword() const
{
    return !_password.empty();
}

void Channel::clearPassword()
{
    _password = "";
}

void Channel::addMember(Client *client)
{
    _members.push_back(client);
}

void Channel::removeMember(Client *client)
{
    for (size_t i = 0; i < _members.size(); i++)
    {
        if (_members[i] == client)
        {
            _members.erase(_members.begin() + i);
            break;
        }
    }
    for (size_t i = 0; i < _operators.size(); i++)
    {
        if (_operators[i] == client)
        {
            _operators.erase(_operators.begin() + i);
            break;
        }
    }
}

bool Channel::isMember(Client *client) const
{
    for (size_t i = 0; i < _members.size(); i++)
        if (_members[i] == client)
            return true;
    return false;
}

std::vector<Client *> &Channel::getMembers()
{
    return _members;
}

bool Channel::isEmpty() const
{
    return _members.empty();
}

void Channel::addOperator(Client *client)
{
    if (!isOperator(client))
        _operators.push_back(client);
}

void Channel::removeOperator(Client *client)
{
    for (size_t i = 0; i < _operators.size(); i++)
    {
        if (_operators[i] == client)
        {
            _operators.erase(_operators.begin() + i);
            return;
        }
    }
}

bool Channel::isOperator(Client *client) const
{
    for (size_t i = 0; i < _operators.size(); i++)
        if (_operators[i] == client)
            return true;
    return false;
}

void Channel::addInvite(const std::string &nickname)
{
    _inviteList.push_back(nickname);
}

void Channel::removeInvite(const std::string &nickname)
{
    for (size_t i = 0; i < _inviteList.size(); i++)
    {
        if (_inviteList[i] == nickname)
        {
            _inviteList.erase(_inviteList.begin() + i);
            return;
        }
    }
}

bool Channel::isInvited(const std::string &nickname) const
{
    for (size_t i = 0; i < _inviteList.size(); i++)
        if (_inviteList[i] == nickname)
            return true;
    return false;
}

void Channel::setInviteOnly(bool value)
{
    _inviteOnly = value;
}

bool Channel::isInviteOnly() const
{
    return _inviteOnly;
}

void Channel::setTopicRestricted(bool value)
{
    _topicRestricted = value;
}

bool Channel::isTopicRestricted() const
{
    return _topicRestricted;
}

void Channel::setUserLimit(size_t limit)
{
    _userLimit = limit;
    _hasUserLimit = true;
}

size_t Channel::getUserLimit() const
{
    return _userLimit;
}

bool Channel::hasUserLimit() const
{
    return _hasUserLimit;
}

void Channel::clearUserLimit()
{
    _hasUserLimit = false;
    _userLimit = 0;
}
