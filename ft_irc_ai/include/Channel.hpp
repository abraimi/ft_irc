#pragma once

#include "Client.hpp"
#include <cstddef>
#include <string>
#include <vector>

class Channel {
    private:
        std::string _name;
        std::string _topic;
        std::string _password; // empty means no password
        std::vector<Client *> _members;
        std::vector<Client *> _operators;
        std::vector<std::string> _inviteList;
        bool _inviteOnly;
        bool _topicRestricted;
        size_t _userLimit;       // 0 means no limit
        bool _hasUserLimit;

    public:
        Channel(const std::string &name);
        ~Channel();

        std::string getName() const;

        void setTopic(const std::string &topic);
        std::string getTopic() const;

        void setPassword(const std::string &password);
        std::string getPassword() const;
        bool hasPassword() const;
        void clearPassword();

        void addMember(Client *client);
        void removeMember(Client *client);
        bool isMember(Client *client) const;
        std::vector<Client *> &getMembers();
        bool isEmpty() const;

        void addOperator(Client *client);
        void removeOperator(Client *client);
        bool isOperator(Client *client) const;

        void addInvite(const std::string &nickname);
        void removeInvite(const std::string &nickname);
        bool isInvited(const std::string &nickname) const;

        void setInviteOnly(bool value);
        bool isInviteOnly() const;

        void setTopicRestricted(bool value);
        bool isTopicRestricted() const;

        void setUserLimit(size_t limit);
        size_t getUserLimit() const;
        bool hasUserLimit() const;
        void clearUserLimit();
};
