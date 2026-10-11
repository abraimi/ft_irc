#pragma once

#include "Command.hpp"
#include "Client.hpp"
#include <vector>


class IRCHandler {
    private:
        std::string _password;
        std::vector<Client *> &_clients;

        void handlePass(Client *client, const Command &command); // password check
        void handleNick(Client *client, const Command &command); // set nickname
        void handleUser(Client *client, const Command &command); // set user args are username, mode, unused, real name
        void handlePrivmsg(Client *client, const Command &command);
        void checkRegistration(Client *client); //auth is true / nickname and username not empty 
    public:
        IRCHandler(const std::string &password, std::vector<Client *> &clients);
        void handle(Client *client, const Command &command);
};