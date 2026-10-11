#pragma once

#include "Command.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <vector>

class IRCHandler {
    private:
        std::string _password;
        std::vector<Client *> &_clients;
        std::vector<Channel *> &_channels;

        void handlePass(Client *client, const Command &command); // password check
        void handleNick(Client *client, const Command &command); // set nickname
        void handleUser(Client *client, const Command &command); // set user args are username, mode, unused, real name
        void handlePrivmsg(Client *client, const Command &command);
        void handleJoin(Client *client, const Command &command);
        void handlePart(Client *client, const Command &command);
        void handleQuit(Client *client, const Command &command);
        void handleKick(Client *client, const Command &command);
        void handleInvite(Client *client, const Command &command);
        void handleTopic(Client *client, const Command &command);
        void handleMode(Client *client, const Command &command);
        void handlePing(Client *client, const Command &command);
        void checkRegistration(Client *client); // auth is true / nickname and username not empty

        Channel *getChannelByName(const std::string &name);
        Client  *getClientByNick(const std::string &nickname);
        bool     isChannelOperator(Channel *channel, Client *client);
        std::string clientPrefix(Client *client);
        void sendToChannel(Channel *channel, const std::string &msg, Client *except);
        std::vector<std::string> split(const std::string &s, char delim);

    public:
        IRCHandler(const std::string &password, std::vector<Client *> &clients, std::vector<Channel *> &channels);
        void handle(Client *client, const Command &command);
        static Command parseCommand(const std::string &line);
};
