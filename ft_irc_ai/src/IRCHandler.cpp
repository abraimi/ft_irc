#include "../include/IRCHandler.hpp"
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

IRCHandler::IRCHandler(const std::string &password, std::vector<Client *> &clients, std::vector<Channel *> &channels) : _password(password), _clients(clients), _channels(channels)
{}

std::vector<std::string> IRCHandler::split(const std::string &s, char delim)
{
    std::vector<std::string> parts;
    std::string current;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == delim)
        {
            parts.push_back(current);
            current = "";
        }
        else
            current += s[i];
    }
    parts.push_back(current);
    return parts;
}

std::string IRCHandler::clientPrefix(Client *client)
{
    return ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " ";
}

Client *IRCHandler::getClientByNick(const std::string &nickname)
{
    for (size_t i = 0; i < _clients.size(); i++)
        if (_clients[i]->getNickname() == nickname)
            return _clients[i];
    return NULL;
}

Channel *IRCHandler::getChannelByName(const std::string &name)
{
    for (size_t i = 0; i < _channels.size(); i++)
        if (_channels[i]->getName() == name)
            return _channels[i];
    return NULL;
}

bool IRCHandler::isChannelOperator(Channel *channel, Client *client)
{
    return channel->isOperator(client);
}

void IRCHandler::sendToChannel(Channel *channel, const std::string &msg, Client *except)
{
    std::vector<Client *> &members = channel->getMembers();
    for (size_t i = 0; i < members.size(); i++)
        if (members[i] != except)
            members[i]->getSendBuff().append(msg);
}

Command IRCHandler::parseCommand(const std::string &line)
{
    Command command;
    std::string s = line;

    if (!s.empty() && s[0] == ':') // skip ":prefix" if present
    {
        size_t sp = s.find(' ');
        if (sp == std::string::npos)
            return command;
        s = s.substr(sp + 1);
    }

    size_t sp = s.find(' ');
    if (sp == std::string::npos)
    {
        command.name = s;
        for (size_t i = 0; i < command.name.size(); i++)
            command.name[i] = toupper(command.name[i]);
        return command;
    }
    command.name = s.substr(0, sp);
    for (size_t i = 0; i < command.name.size(); i++)
        command.name[i] = toupper(command.name[i]);
    s = s.substr(sp + 1);

    while (!s.empty())
    {
        if (s[0] == ':') // trailing argument takes everything
        {
            command.args.push_back(s.substr(1));
            break;
        }
        sp = s.find(' ');
        if (sp == std::string::npos)
        {
            command.args.push_back(s);
            break;
        }
        command.args.push_back(s.substr(0, sp));
        s = s.substr(sp + 1);
        while (!s.empty() && s[0] == ' ')
            s = s.substr(1);
    }
    return command;
}

void IRCHandler::checkRegistration(Client *client)
{
    if(client->isRegistered())
        return;
    if(!client->isAuth())
        return;
    if(client->getUsername().empty())
        return;
    if(client->getNickname().empty())
        return;
    client->setRegister(true);
    client->getSendBuff().append(":server 001 " + client->getNickname() + " :Welcome to the ft_irc_ai network " + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + "\r\n");
}

void IRCHandler::handle(Client *client, const Command &command)
{
    if(command.name == "PASS")
        handlePass(client, command);
    else if(command.name == "NICK")
        handleNick(client, command);
    else if(command.name == "USER")
        handleUser(client, command);
    else if (command.name == "PRIVMSG")
        handlePrivmsg(client, command);
    else if (command.name == "JOIN")
        handleJoin(client, command);
    else if (command.name == "PART") //alo
        handlePart(client, command);
    else if (command.name == "QUIT")
        handleQuit(client, command);
    else if (command.name == "KICK")
        handleKick(client, command);
    else if (command.name == "INVITE")
        handleInvite(client, command);
    else if (command.name == "TOPIC")
        handleTopic(client, command);
    else if (command.name == "MODE")
        handleMode(client, command);
    else if (command.name == "PING")
        handlePing(client, command);
    else
        client->getSendBuff().append(":server 421 " + client->getNickname() + " " + command.name + " :Unknown command 😛\r\n");
}

void IRCHandler::handlePass(Client *client, const Command &command)
{
    if(command.args.size() != 1)
    {
        client->getSendBuff().append(":server 461 * PASS: Not enough paramaters 😛\r\n");
        return;
    }
    if(client->isAuth())
    {
        client->getSendBuff().append(":server 462 * PASS: You may not register 😛\r\n");
        return;
    }
    if(command.args[0] != _password)
    {
        client->getSendBuff().append(":server 464 * PASS: nice try diddy -- Incorrect Password 😛\r\n");
        return;
    }
    client->setisAuth(true);
    checkRegistration(client);
    std::cout << "Client authenticated!" << std::endl;
}

void IRCHandler::handleNick(Client *client, const Command &command)
{
    if(command.args.size() != 1 || command.args[0].empty())
    {
        client->getSendBuff().append(":server 431 * NICK: No nickname given 😛\r\n");
        return;
    }
    std::string nickname = command.args[0];

    for (size_t i = 0; i < nickname.size(); i++)
    {
        if (nickname[i] == ' ' || nickname[i] == '!' || nickname[i] == '@' || nickname[i] == '#')
        {
            client->getSendBuff().append(":server 432 * NICK " + nickname + " :Erroneus nickname 😛\r\n");
            return;
        }
    }

    for(size_t i = 0; i < _clients.size(); i++)
    {
        if(_clients[i] != client && _clients[i]->getNickname() == nickname)
        {
            client->getSendBuff().append(":server 433 * NICK " + nickname +" :Nickname is already in use 😔\r\n");
            return;
        }
    }

    std::string oldNick = client->getNickname();
    client->setNickname(nickname);

    if (client->isRegistered() && !oldNick.empty())
    {
        std::string msg = ":" + oldNick + "!" + client->getUsername() + "@" + client->getHostname() + " NICK :" + nickname + "\r\n";
        for (size_t i = 0; i < _channels.size(); i++)
        {
            Channel *ch = _channels[i];
            if (ch->isMember(client))
            {
                sendToChannel(ch, msg, client);
            }
        }
        client->getSendBuff().append(msg);
    }
    checkRegistration(client);
}

void IRCHandler::handleUser(Client *client, const Command &command)
{
    if(command.args.size() < 4)
    {
        client->getSendBuff().append(":server 461 * USER: Not enough paramaters 😛\r\n");
        return;
    }
    if(!client->getUsername().empty())
    {
        client->getSendBuff().append(":server 462 * USER: you may not register (no username) 😛\r\n");
        return;
    }
    client->setUserName(command.args[0]);
    checkRegistration(client);
}

void IRCHandler::handlePrivmsg(Client *client, const Command &command)
{
    if (!client->isRegistered())
    {
        client->getSendBuff().append(":server 451 * :You have not registered 😛\r\n");
        return;
    }
    if (command.args.size() < 2)
    {
        client->getSendBuff().append(":server 412 " + client->getNickname() + " :No text to send 😛\r\n");
        return;
    }
    std::string target = command.args[0];
    std::string text = command.args[1];
    std::string line = clientPrefix(client) + "PRIVMSG " + target + " :" + text + "\r\n";

    if (target[0] == '#' || target[0] == '&')
    {
        Channel *channel = getChannelByName(target);
        if (channel == NULL)
        {
            client->getSendBuff().append(":server 404 " + client->getNickname() + " " + target + " :Cannot send to channel 😛\r\n");
            return;
        }
        if (!channel->isMember(client))
        {
            client->getSendBuff().append(":server 404 " + client->getNickname() + " " + target + " :Cannot send to channel 😛\r\n");
            return;
        }
        sendToChannel(channel, line, client);
        return;
    }

    Client *dest = getClientByNick(target);
    if (dest == NULL)
    {
        client->getSendBuff().append(":server 401 " + client->getNickname() + " " + target + " :No such nick/channel 😔\r\n");
        return;
    }
    dest->getSendBuff().append(line);
}

void IRCHandler::handleJoin(Client *client, const Command &command)
{
    if (!client->isRegistered())
    {
        client->getSendBuff().append(":server 451 * :You have not registered 😛\r\n");
        return;
    }
    if (command.args.empty())
    {
        client->getSendBuff().append(":server 461 " + client->getNickname() + " JOIN :Not enough parameters 😛\r\n");
        return;
    }

    std::vector<std::string> names = split(command.args[0], ',');
    std::vector<std::string> keys;
    if (command.args.size() > 1)
        keys = split(command.args[1], ',');

    for (size_t i = 0; i < names.size(); i++)
    {
        std::string name = names[i];
        std::string key = (i < keys.size()) ? keys[i] : "";

        if (name.empty() || name[0] != '#')
        {
            client->getSendBuff().append(":server 476 " + client->getNickname() + " " + name + " :Bad channel mask 😛\r\n");
            continue;
        }

        Channel *channel = getChannelByName(name);
        bool newChannel = (channel == NULL);
        if (newChannel)
        {
            channel = new Channel(name);
            _channels.push_back(channel);
        }
        else
        {
            if (channel->isInviteOnly() && !channel->isInvited(client->getNickname()))
            {
                client->getSendBuff().append(":server 473 " + client->getNickname() + " " + name + " :Cannot join channel (+i) 😛\r\n");
                continue;
            }
            if (channel->hasPassword() && channel->getPassword() != key)
            {
                client->getSendBuff().append(":server 475 " + client->getNickname() + " " + name + " :Cannot join channel (+k) 😛\r\n");
                continue;
            }
            if (channel->hasUserLimit() && channel->getMembers().size() >= channel->getUserLimit())
            {
                client->getSendBuff().append(":server 471 " + client->getNickname() + " " + name + " :Cannot join channel (+l) 😛\r\n");
                continue;
            }
        }

        if (channel->isMember(client))
            continue;

        channel->addMember(client);
        if (newChannel)
        {
            channel->addOperator(client); // first member becomes operator
            channel->setTopicRestricted(true);
            if (!key.empty())
                channel->setPassword(key);
        }
        else
        {
            channel->removeInvite(client->getNickname());
        }
        client->addChannel(name);

        std::string joinLine = clientPrefix(client) + "JOIN :" + name + "\r\n";
        std::vector<Client *> &members = channel->getMembers();
        for (size_t j = 0; j < members.size(); j++)
            members[j]->getSendBuff().append(joinLine);

        if (channel->getTopic().empty())
            client->getSendBuff().append(":server 331 " + client->getNickname() + " " + name + " :No topic is set\r\n");
        else
            client->getSendBuff().append(":server 332 " + client->getNickname() + " " + name + " :" + channel->getTopic() + "\r\n");

        std::string namesLine = ":server 353 " + client->getNickname() + " = " + name + " :";
        for (size_t j = 0; j < members.size(); j++)
        {
            if (channel->isOperator(members[j]))
                namesLine += "@";
            namesLine += members[j]->getNickname() + " ";
        }
        namesLine += "\r\n";
        client->getSendBuff().append(namesLine);
        client->getSendBuff().append(":server 366 " + client->getNickname() + " " + name + " :End of /NAMES list\r\n");
    }
}

void IRCHandler::handlePart(Client *client, const Command &command)
{
    if (!client->isRegistered())
    {
        client->getSendBuff().append(":server 451 * :You have not registered 😛\r\n");
        return;
    }
    if (command.args.empty())
    {
        client->getSendBuff().append(":server 461 " + client->getNickname() + " PART :Not enough parameters 😛\r\n");
        return;
    }

    std::string reason = "";
    if (command.args.size() > 1)
        reason = command.args[1];

    std::vector<std::string> names = split(command.args[0], ',');
    for (size_t i = 0; i < names.size(); i++)
    {
        Channel *channel = getChannelByName(names[i]);
        if (channel == NULL)
        {
            client->getSendBuff().append(":server 403 " + client->getNickname() + " " + names[i] + " :No such channel 😛\r\n");
            continue;
        }
        if (!channel->isMember(client))
        {
            client->getSendBuff().append(":server 442 " + client->getNickname() + " " + names[i] + " :You're not on that channel 😛\r\n");
            continue;
        }

        std::string line = clientPrefix(client) + "PART " + names[i] + (reason.empty() ? "\r\n" : " :" + reason + "\r\n");
        sendToChannel(channel, line, NULL);
        channel->removeMember(client);
        client->removeChannel(names[i]);
        if (channel->isEmpty())
        {
            for (size_t j = 0; j < _channels.size(); j++)
            {
                if (_channels[j] == channel)
                {
                    delete channel;
                    _channels.erase(_channels.begin() + j);
                    break;
                }
            }
        }
    }
}

void IRCHandler::handleQuit(Client *client, const Command &command)
{
    std::string reason = "Client Quit";
    if (!command.args.empty())
        reason = command.args[0];
    std::string line = clientPrefix(client) + "QUIT :" + reason + "\r\n";

    std::vector<std::string> channels = client->getChannels();
    for (size_t i = 0; i < channels.size(); i++)
    {
        Channel *channel = getChannelByName(channels[i]);
        if (channel != NULL)
            sendToChannel(channel, line, client);
    }
}

void IRCHandler::handleKick(Client *client, const Command &command)
{
    if (!client->isRegistered())
    {
        client->getSendBuff().append(":server 451 * :You have not registered 😛\r\n");
        return;
    }
    if (command.args.size() < 2)
    {
        client->getSendBuff().append(":server 461 " + client->getNickname() + " KICK :Not enough parameters 😛\r\n");
        return;
    }

    Channel *channel = getChannelByName(command.args[0]);
    if (channel == NULL)
    {
        client->getSendBuff().append(":server 403 " + client->getNickname() + " " + command.args[0] + " :No such channel 😛\r\n");
        return;
    }
    if (!channel->isMember(client))
    {
        client->getSendBuff().append(":server 442 " + client->getNickname() + " " + command.args[0] + " :You're not on that channel 😛\r\n");
        return;
    }
    if (!isChannelOperator(channel, client))
    {
        client->getSendBuff().append(":server 482 " + client->getNickname() + " " + command.args[0] + " :You're not channel operator 😛\r\n");
        return;
    }

    std::string reason = command.args.size() > 2 ? command.args[2] : "Kicked";
    std::vector<std::string> nicks = split(command.args[1], ',');
    for (size_t i = 0; i < nicks.size(); i++)
    {
        Client *target = getClientByNick(nicks[i]);
        if (target == NULL || !channel->isMember(target))
        {
            client->getSendBuff().append(":server 441 " + client->getNickname() + " " + nicks[i] + " :They aren't on that channel 😛\r\n");
            continue;
        }
        std::string line = clientPrefix(client) + "KICK " + command.args[0] + " " + target->getNickname() + " :" + reason + "\r\n";
        sendToChannel(channel, line, NULL);
        channel->removeMember(target);
        target->removeChannel(command.args[0]);
    }
}

void IRCHandler::handleInvite(Client *client, const Command &command)
{
    if (!client->isRegistered())
    {
        client->getSendBuff().append(":server 451 * :You have not registered 😛\r\n");
        return;
    }
    if (command.args.size() < 2)
    {
        client->getSendBuff().append(":server 461 " + client->getNickname() + " INVITE :Not enough parameters 😛\r\n");
        return;
    }

    Channel *channel = getChannelByName(command.args[1]);
    if (channel == NULL)
    {
        client->getSendBuff().append(":server 403 " + client->getNickname() + " " + command.args[1] + " :No such channel 😛\r\n");
        return;
    }
    if (!channel->isMember(client))
    {
        client->getSendBuff().append(":server 442 " + client->getNickname() + " " + command.args[1] + " :You're not on that channel 😛\r\n");
        return;
    }
    if (channel->isInviteOnly() && !isChannelOperator(channel, client))
    {
        client->getSendBuff().append(":server 482 " + client->getNickname() + " " + command.args[1] + " :You're not channel operator 😛\r\n");
        return;
    }

    Client *target = getClientByNick(command.args[0]);
    if (target == NULL)
    {
        client->getSendBuff().append(":server 401 " + client->getNickname() + " " + command.args[0] + " :No such nick/channel 😔\r\n");
        return;
    }
    if (channel->isMember(target))
    {
        client->getSendBuff().append(":server 443 " + client->getNickname() + " " + target->getNickname() + " " + channel->getName() + " :is already on channel 😛\r\n");
        return;
    }

    channel->addInvite(target->getNickname());
    target->getSendBuff().append(clientPrefix(client) + "INVITE " + target->getNickname() + " :" + channel->getName() + "\r\n");
    client->getSendBuff().append(":server 341 " + client->getNickname() + " " + target->getNickname() + " " + channel->getName() + "\r\n");
}

void IRCHandler::handleTopic(Client *client, const Command &command)
{
    if (!client->isRegistered())
    {
        client->getSendBuff().append(":server 451 * :You have not registered 😛\r\n");
        return;
    }
    if (command.args.empty())
    {
        client->getSendBuff().append(":server 461 " + client->getNickname() + " TOPIC :Not enough parameters 😛\r\n");
        return;
    }

    Channel *channel = getChannelByName(command.args[0]);
    if (channel == NULL)
    {
        client->getSendBuff().append(":server 403 " + client->getNickname() + " " + command.args[0] + " :No such channel 😛\r\n");
        return;
    }
    if (!channel->isMember(client))
    {
        client->getSendBuff().append(":server 442 " + client->getNickname() + " " + command.args[0] + " :You're not on that channel 😛\r\n");
        return;
    }

    if (command.args.size() == 1)
    {
        if (channel->getTopic().empty())
            client->getSendBuff().append(":server 331 " + client->getNickname() + " " + channel->getName() + " :No topic is set\r\n");
        else
            client->getSendBuff().append(":server 332 " + client->getNickname() + " " + channel->getName() + " :" + channel->getTopic() + "\r\n");
        return;
    }

    if (channel->isTopicRestricted() && !isChannelOperator(channel, client))
    {
        client->getSendBuff().append(":server 482 " + client->getNickname() + " " + channel->getName() + " :You're not channel operator 😛\r\n");
        return;
    }

    channel->setTopic(command.args[1]);
    std::string line = clientPrefix(client) + "TOPIC " + channel->getName() + " :" + command.args[1] + "\r\n";
    sendToChannel(channel, line, NULL);
}

void IRCHandler::handleMode(Client *client, const Command &command)
{
    if (!client->isRegistered())
    {
        client->getSendBuff().append(":server 451 * :You have not registered 😛\r\n");
        return;
    }
    if (command.args.empty())
    {
        client->getSendBuff().append(":server 461 " + client->getNickname() + " MODE :Not enough parameters 😛\r\n");
        return;
    }

    Channel *channel = getChannelByName(command.args[0]);
    if (channel == NULL)
    {
        client->getSendBuff().append(":server 403 " + client->getNickname() + " " + command.args[0] + " :No such channel 😛\r\n");
        return;
    }

    if (command.args.size() == 1)
    {
        std::string modes = "+";
        std::string params = "";
        if (channel->isInviteOnly())
            modes += "i";
        if (channel->isTopicRestricted())
            modes += "t";
        if (channel->hasPassword())
        {
            modes += "k";
            params += " " + channel->getPassword();
        }
        if (channel->hasUserLimit())
        {
            modes += "l";
            std::ostringstream limit;
            limit << channel->getUserLimit();
            params += " " + limit.str();
        }
        client->getSendBuff().append(":server 324 " + client->getNickname() + " " + channel->getName() + " " + modes + params + "\r\n");
        return;
    }

    if (!channel->isMember(client))
    {
        client->getSendBuff().append(":server 442 " + client->getNickname() + " " + channel->getName() + " :You're not on that channel 😛\r\n");
        return;
    }
    if (!isChannelOperator(channel, client))
    {
        client->getSendBuff().append(":server 482 " + client->getNickname() + " " + channel->getName() + " :You're not channel operator 😛\r\n");
        return;
    }

    std::string modeString = command.args[1];
    std::string applied = "";
    char sign = '+';
    size_t paramIndex = 2;

    for (size_t i = 0; i < modeString.size(); i++)
    {
        char c = modeString[i];
        if (c == '+' || c == '-')
        {
            sign = c;
            if (applied.empty() || (applied[applied.size() - 1] != '+' && applied[applied.size() - 1] != '-'))
                applied += c;
            else if (applied[applied.size() - 1] != c)
                applied += c;
            continue;
        }
        switch (c)
        {
            case 'i':
                channel->setInviteOnly(sign == '+');
                applied += c;
                break;
            case 't':
                channel->setTopicRestricted(sign == '+');
                applied += c;
                break;
            case 'k':
                if (sign == '+')
                {
                    if (paramIndex < command.args.size())
                    {
                        channel->setPassword(command.args[paramIndex++]);
                        applied += c;
                    }
                }
                else
                {
                    channel->clearPassword();
                    applied += c;
                }
                break;
            case 'l':
                if (sign == '+')
                {
                    if (paramIndex < command.args.size())
                    {
                        channel->setUserLimit(static_cast<size_t>(std::atoi(command.args[paramIndex++].c_str())));
                        applied += c;
                    }
                }
                else
                {
                    channel->clearUserLimit();
                    applied += c;
                }
                break;
            case 'o':
                if (paramIndex < command.args.size())
                {
                    Client *target = getClientByNick(command.args[paramIndex++]);
                    if (target != NULL && channel->isMember(target))
                    {
                        if (sign == '+')
                            channel->addOperator(target);
                        else
                            channel->removeOperator(target);
                        applied += c;
                    }
                    else
                    {
                        client->getSendBuff().append(":server 441 " + client->getNickname() + " " + command.args[paramIndex - 1] + " :They aren't on that channel 😛\r\n");
                    }
                }
                break;
            default:
                client->getSendBuff().append(":server 501 " + client->getNickname() + " " + c + " :Unknown mode flag 😛\r\n");
                break;
        }
    }

    if (!applied.empty())
        sendToChannel(channel, clientPrefix(client) + "MODE " + channel->getName() + " " + applied + "\r\n", NULL);
}

void IRCHandler::handlePing(Client *client, const Command &command)
{
    std::string token = command.args.empty() ? "ft_irc_ai" : command.args[0];
    client->getSendBuff().append(":server PONG server :" + token + "\r\n");
}
