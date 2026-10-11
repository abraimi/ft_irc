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
	PASS, // 0: PASS <password>
	USER, // 1: USER <username> <hostname> <servername> :<realname>
	NICK, // 2: NICK <nickname>
	JOIN, // 3: JOIN #channel <key>
	PRIVMSG, // 4:  PRIVMSG <target>/#channel :<text>

	// Operator specific commands
	KICK, // 5
	INVITE, // 6
	TOPIC, // 7
	MODE // 8
};


class Message {
	private:
		Client						*client;

		Authority_t					authority;
		Command_t					cmdType;
		std::string					commandName;
		std::vector<std::string>	args;
		bool						valid;		// false => nothing to execute (CmdType stays UNKNOWN)

		std::vector<std::string> 	tokens;

		void		tokenize(std::string& line, std::string::iterator& it);
		std::string	sanitize(const std::string& line);

		Command_t	commandType(const std::string& command);

	public:
		Message(Client *client);
		~Message();

		bool	parseNextMessage();
		void	parseMessage(const std::string &line);
		void	validateArgs();

		/* Getters */
		Authority_t					getAuthority() const;
		Command_t					getType() const;
		const std::string			&getCommandName() const;
		std::vector<std::string>	getArgs() const;
		bool						getValid() const;
};

#endif // MESSAGE_HPP
