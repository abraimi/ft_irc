#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "Client.hpp"
#include <cstddef>
#include <string>
#include <vector>


/**
* @brief 
*
* @param Operator This is for channel operators, to distinguich them from regular commands.
* @param Regular These should be available for all users including channel operators.
**/
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
		std::size_t					errNum;

		std::vector<std::string> 	tokens;

		// void		tokenize(const std::string &line, std::size_t start);
		void		tokenize(std::string& line, std::string::iterator& it);
		std::string	sanitize(const std::string& line);

		Command_t	commandType(const std::string& command);

	public:
		Command();
		Command(Client *client);
		~Command();

		/**
		* @brief Sanitize ONE complete IRC line (already split on \r\n by the
		*        Server), skip an optional ":prefix" and split the remainder
		*        into the command name (tokens[0]) + arguments.
		*
		* @param line a single message, terminator already removed
		* @return Command_t with valid=false when the line carries no command
		*         (empty line, prefix only) ; otherwise valid=true and
		*         Args = the parameters only (command name is consumed later
		*         by the name -> CommandTypes lookup + validation).
		**/
		void	parseCommand(const std::string &line);

		/* Getters */
		Authority_t					getAuthority() const;
		Command_t					getType() const;
		std::vector<std::string>	getArgs() const;
		bool						getValid() const;
		std::size_t					getErrNum() const;
};

#endif // COMMAND_HPP
