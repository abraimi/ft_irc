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
        std::string _hostname;

    public:
        Client(int fd);
        ~Client();

        int getFd() const;
        std::string &getBuff();
        
        void setNickname(const std::string &nickname);
        std::string getNickname() const;

        void setUserName(const std::string &username);
        std::string getUsername() const;
        void setRealname(const std::string &realname);
        std::string getRealname() const;

        void setisAuth(bool isAuth);
        bool isAuth() const;

        void setRegister(bool isRegistered);
        bool isRegistered() const;

        void setHostname(const std::string &hostname);
        std::string getHostname() const;

        std::string &getSendBuff();
};
