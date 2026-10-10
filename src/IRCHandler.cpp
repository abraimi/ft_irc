#include "../include/IRCHandler.hpp"
#include <cstddef>
#include <iostream>
#include <vector>

IRCHandler::IRCHandler(const std::string &password, std::vector<Client *> &clients) : _password(password), _clients(clients)
{}

void IRCHandler::checkRegistration(Client *client)
{
    if(client->isRegistered())
        return;
    if(!client->isAuth())
        return;
    if(client->getUsername().empty())
        return;
    if(client->getUsername().empty())
        return;
    client->setRegister(true);
}

void IRCHandler::handle(Client *client, const Command &command)
{
    if(command.name == "PASS")
        handlePass(client, command);
    if(command.name == "NICK")
        handleNick(client, command);
    if(command.name == "USER")
        handleUser(client, command);
    if (command.name == "PRIVMSG")
        handlePrivmsg(client, command);
}

void IRCHandler::handlePass(Client *client, const Command &command)
{
    if(command.args.size() != 1)
    {
        client->getSendBuff().append(":server 461 * PASS: Not enough paramaters 😛");
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
    if(command.args.size() != 1)
    {
        client->getSendBuff().append(":server 431 * NICK: No nickname given 😛\r\n");
        return;
    }
    std::string nickname = command.args[0];

    for(size_t i = 0; i < _clients.size(); i++)
    {
        if(_clients[i] != client && _clients[i]->getNickname() == nickname)
        {
            client->getSendBuff().append(":server 433 * NICK " + nickname +" :Nickname is already in use 😔\r\n:");
            return;
        }
        client->setUserName(nickname);
        checkRegistration(client);
    }
}

void IRCHandler::handleUser(Client *client, const Command &command)
{
    if(command.args.size() != 4)
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
