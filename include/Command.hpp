#ifndef COMMAND_HPP
#define COMMAND_HPP

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


class Command {
	private:
		Client						*client;

		Authority_t					authority;
		Command_t					type;
		std::vector<std::string>	args;
		bool						valid;		// false => nothing to execute (CmdType stays UNKNOWN)

		std::vector<std::string> 	tokens;

		void		tokenize(std::string& line, std::string::iterator& it);
		std::string	sanitize(const std::string& line);

		Command_t	commandType(const std::string& command);

	public:
		Command(Client *client);
		~Command();

		void	parseCommand(const std::string &line);
		void	validateArgs();

		/* Getters */
		Authority_t					getAuthority() const;
		Command_t					getType() const;
		std::vector<std::string>	getArgs() const;
		bool						getValid() const;
};

#endif // COMMAND_HPP
