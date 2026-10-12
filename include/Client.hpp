#pragma once

#include <string>

class Client {
    private:
        int _fd;
        std::string _buff;
        std::string _nickname;
        std::string _username;
        std::string _realname;
        std::string _sendBuff;
        bool        _isAuth; 
        bool        _isRegistered; // already connected NICK + USER = regisetered
        bool        _shouldDisconnect;
        std::string _hostname;

        Client(const Client &other);
        Client &operator=(const Client &other);

    public:
        Client(int fd);
        ~Client();

        int getFd() const;
        std::string &getBuff();
        
        void setNickname(const std::string &nickname);
        const std::string &getNickname() const;

        void setUserName(const std::string &username);
        const std::string &getUsername() const;
        void setRealname(const std::string &realname);
        const std::string &getRealname() const;

        void setisAuth(bool isAuth);
        bool isAuth() const;

        void setRegister(bool isRegistered);
        bool isRegistered() const;

        void markForDisconnect();
        bool shouldDisconnect() const;

        void setHostname(const std::string &hostname);
        const std::string &getHostname() const;

        std::string &getSendBuff();
};
