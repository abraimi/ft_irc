#include "Channel.hpp"
#include <stdexcept>

Channel::Channel(const std::string &name)
	: _name(name), _userLimit(0), _inviteOnly(false),
	  _topicOperatorOnly(false), _hasUserLimit(false) {}

Channel::~Channel() {}

bool Channel::contains(const std::vector<Client *> &list, const Client *client) {
	for (std::size_t i = 0; i < list.size(); ++i) {
		if (list[i] == client) {
			return true;
		}
	}
	return false;
}

const std::string &Channel::getName() const { return _name; }
const std::string &Channel::getTopic() const { return _topic; }
void Channel::setTopic(const std::string &topic) { _topic = topic; }
std::size_t Channel::getMemberCount() const { return _members.size(); }
const std::vector<Client *> &Channel::getMembers() const { return _members; }
bool Channel::hasMember(const Client *client) const { return contains(_members, client); }


bool Channel::addMember(Client *client) {
	if (!client || hasMember(client)) {
		return false;
	}
	_members.push_back(client);
	return true;
}


bool Channel::removeMember(Client *client) {
	for (std::vector<Client *>::iterator it = _members.begin();
		it != _members.end(); ++it) {
		if (*it == client) {
			_members.erase(it);
			removeOperator(client);
			revokeInvite(client);
			return true;
		}
	}
	return false;
}

bool Channel::isOperator(const Client *client) const { return contains(_operators, client); }

bool Channel::addOperator(Client *client) {
	if (!hasMember(client) || isOperator(client)) {
		return false;
	}
	_operators.push_back(client);
	return true;
}


bool Channel::removeOperator(Client *client) {
	for (std::vector<Client *>::iterator it = _operators.begin();
		it != _operators.end(); ++it) {
		if (*it == client) {
			_operators.erase(it);
			return true;
		}
	}
	return false;
}

Client *Channel::promoteFirstMemberIfOperatorless() {
	if (!_operators.empty() || _members.empty())
		return NULL;
	_operators.push_back(_members[0]);
	return _members[0];
}

bool Channel::isInvited(const Client *client) const { return contains(_invited, client); }

bool Channel::invite(Client *client) {
	if (!client || isInvited(client)) {
		return false;
	}
	_invited.push_back(client);
	return true;
}


bool Channel::revokeInvite(Client *client) {
	for (std::vector<Client *>::iterator it = _invited.begin();
		it != _invited.end(); ++it) {
		if (*it == client) {
			_invited.erase(it);
			return true;
		}
	}
	return false;
}

bool Channel::isInviteOnly() const { return _inviteOnly; }
void Channel::setInviteOnly(bool enabled) { _inviteOnly = enabled; }
bool Channel::isTopicOperatorOnly() const { return _topicOperatorOnly; }
void Channel::setTopicOperatorOnly(bool enabled) { _topicOperatorOnly = enabled; }
bool Channel::hasKey() const { return !_key.empty(); }
const std::string &Channel::getKey() const { return _key; }
void Channel::setKey(const std::string &key) { _key = key; }
void Channel::clearKey() { _key.clear(); }
bool Channel::hasUserLimit() const { return _hasUserLimit; }
std::size_t Channel::getUserLimit() const { return _userLimit; }
void Channel::setUserLimit(std::size_t limit) {
	if (limit == 0) {
		throw std::runtime_error("Channel user limit must be greater than zero");
	}
	_userLimit = limit;
	_hasUserLimit = true;
}
void Channel::clearUserLimit() { _userLimit = 0; _hasUserLimit = false; }

std::string Channel::getChannelModes() const {
	std::string modes = "+";
	if (_inviteOnly) {
		modes += 'i';
	}
	if (_topicOperatorOnly) {
		modes += 't';
	}
	if (hasKey()) {
		modes += 'k';
	}
	if (_hasUserLimit) {
		modes += 'l';
	}
	return modes;
}
