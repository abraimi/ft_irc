#ifndef MESSAGEEXCEPTION_HPP
#define MESSAGEEXCEPTION_HPP

#include "Client.hpp"
#include <exception>
#include <map>
#include <string>
#include <utility>

/**
* IRC numeric errors required by the supported Command_t set
 * (PASS USER NICK JOIN PRIVMSG KICK INVITE TOPIC MODE) plus the
 * registration gate and UNKNOWN. Explicit values are IRC wire numerics,
 * so no magic numbers appear at the throw sites.
*/
enum ErrNum_t {
	ERR_NONE = 0,

	ERR_NOSUCHNICK       = 401, // PRIVMSG, INVITE
	ERR_NOSUCHCHANNEL    = 403, // JOIN, PRIVMSG, KICK, TOPIC, MODE
	ERR_CANNOTSENDTOCHAN = 404, // PRIVMSG
	ERR_NORECIPIENT      = 411, // PRIVMSG
	ERR_NOTEXTTOSEND     = 412, // PRIVMSG
	ERR_UNKNOWNCOMMAND   = 421, // UNKNOWN
	ERR_NONICKNAMEGIVEN  = 431, // NICK
	ERR_ERRONEUSNICKNAME = 432, // NICK, INVITE
	ERR_NICKNAMEINUSE    = 433, // NICK
	ERR_USERNOTINCHANNEL = 441, // KICK
	ERR_NOTONCHANNEL     = 442, // KICK, TOPIC
	ERR_USERONCHANNEL    = 443, // INVITE
	ERR_NOTREGISTERED    = 451, // gate on every command
	ERR_NEEDMOREPARAMS   = 461, // arity, all commands
	ERR_ALREADYREGISTRED = 462, // PASS, USER
	ERR_PASSWDMISMATCH   = 464, // PASS
	ERR_KEYSET           = 467, // MODE +k
	ERR_CHANNELISFULL    = 471, // JOIN
	ERR_UNKNOWNMODE      = 472, // MODE
	ERR_INVITEONLYCHAN   = 473, // JOIN
	ERR_BANNEDFROMCHAN   = 474, // JOIN
	ERR_BADCHANNELKEY    = 475, // JOIN
	ERR_BADCHANMASK      = 476, // JOIN
	ERR_CHANOPRIVSNEEDED = 482, // KICK, TOPIC, MODE
	ERR_INVALIDMODEPARAM = 696  // MODE
};

/**
* Carries one IRC error as a complete wire line.
* what() => ":server <numeric> <target> [<param>] :<reason>\r\n"
*/

typedef std::pair<ErrNum_t, std::string>	ErrStrType;
typedef std::map<ErrNum_t, std::string>		ErrStrMapType;

class MessageException : public std::exception {
	private:
		ErrNum_t	errNum;
		std::string	errStr;

		static const ErrStrMapType	ErrStrs;
		static ErrStrMapType		initErrStrs();

		public:
		MessageException();
		MessageException(ErrNum_t err, Client* client);
		MessageException(ErrNum_t err, const std::string& param, const std::string& server);
		virtual ~MessageException() throw();
			
		virtual const char* what() const throw();
};

#endif // MESSAGEEXCEPTION_HPP
