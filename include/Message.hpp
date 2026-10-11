#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "Client.hpp"
#include <string>
#include <vector>

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
	MODE, // 8
	CAP,  // minimal client capability negotiation compatibility
	PING  // server liveness probe
};


class Message {
	private:
		Client						*client;

		Command_t					cmdType;
		std::string					commandName;
		std::vector<std::string>	args;
		bool						valid;		// false => nothing to execute (CmdType stays UNKNOWN)

		Command_t	commandType(const std::string& command);

	public:
		Message(Client *client);
		~Message();

		void	parseMessage(const std::string &line);
		void	validateArgs();

		/* Getters */
		Command_t					getType() const;
		const std::string			&getCommandName() const;
		const std::vector<std::string> &getArgs() const;
		bool						getValid() const;
};

#endif // MESSAGE_HPP
