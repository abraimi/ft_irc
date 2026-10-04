#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "Client.hpp"
#include <cstddef>
#include <string>
#include <vector>

class Channel {
	private:
		std::string _name;
		std::string _topic;
		std::string _key;
		std::vector<Client *> _members;
		std::vector<Client *> _operators;
		std::vector<Client *> _invited;
		std::size_t _userLimit;
		bool _inviteOnly;
		bool _topicOperatorOnly;
		bool _hasUserLimit;
		static bool contains(const std::vector<Client *> &list, const Client *client);

	public:
		explicit Channel(const std::string &name);
		const std::string &getName() const;
		const std::string &getTopic() const;
		void setTopic(const std::string &topic);
		std::size_t getMemberCount() const;
		const std::vector<Client *> &getMembers() const;
		bool hasMember(const Client *client) const;
		bool addMember(Client *client);
		bool removeMember(Client *client);
		bool isOperator(const Client *client) const;
		bool addOperator(Client *client);
		bool removeOperator(Client *client);
		bool isInvited(const Client *client) const;
		bool invite(Client *client);
		bool revokeInvite(Client *client);
		bool isInviteOnly() const;
		void setInviteOnly(bool enabled);
		bool isTopicOperatorOnly() const;
		void setTopicOperatorOnly(bool enabled);
		bool hasKey() const;
		const std::string &getKey() const;
		void setKey(const std::string &key);
		void clearKey();
		bool hasUserLimit() const;
		std::size_t getUserLimit() const;
		void setUserLimit(std::size_t limit);
		void clearUserLimit();
		std::string getChannelModes() const;
};

#endif
