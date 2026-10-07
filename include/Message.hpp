#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "Client.hpp"
#include <string>
#include <vector>

enum Authority_t {
	OPERATOR,
	REGULAR
};

enum Command_t {
	UNKNOWN = -1, // no (valid) command in the parsed line

	// General commands
	PASS, // PASS <password>
	USER, // USER <username> <hostname> <servername> :<realname>
	NICK, // NICK <nickname>
	JOIN, // JOIN #channel <key>
	PRIVMSG, // PRIVMSG <target>/#channel :<text>

	// Operator specific commands
	KICK,
	INVITE,
	TOPIC,
	MODE
};


class Message {
	private:
		Client						*client;

		Authority_t					authority;
		Command_t					cmdType;
		std::vector<std::string>	args;
		bool						valid;		// false => nothing to execute (CmdType stays UNKNOWN)

		std::vector<std::string> 	tokens;

		void		tokenize(std::string& line, std::string::iterator& it);
		std::string	sanitize(const std::string& line);

		Command_t	commandType(const std::string& command);

	public:
		Message(Client *client);
		~Message();

		void	parseMessage(const std::string &line);
		void	validateArgs();

		/* Getters */
		Authority_t					getAuthority() const;
		Command_t					getType() const;
		std::vector<std::string>	getArgs() const;
		bool						getValid() const;
};

#endif // MESSAGE_HPP
