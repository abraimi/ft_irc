#include "CommandException.hpp"
#include <map>
#include <sstream>

/**
* RFC 1459 trailing text for each numeric the supported commands need.
* Dynamic bits (nick, channel, offending command) travel as `param`.
* Returns the table that initializes the static const ErrStrs member,
* built exactly once before main; every later use is a map lookup by key.
*/
ErrStrMapType CommandException::initErrStrs() {
	static const ErrStrType errs[] = {
		ErrStrType(ERR_NOSUCHNICK, "No such nick/channel"), 
		ErrStrType(ERR_NOSUCHCHANNEL, "No such channel"), 
		ErrStrType(ERR_CANNOTSENDTOCHAN, "Cannot send to channel"), 
		ErrStrType(ERR_NORECIPIENT, "No recipient given"), 
		ErrStrType(ERR_NOTEXTTOSEND, "No text to send"), 
		ErrStrType(ERR_UNKNOWNCOMMAND, "Unknown command"), 
		ErrStrType(ERR_NONICKNAMEGIVEN, "No nickname given"), 
		ErrStrType(ERR_ERRONEUSNICKNAME, "Erroneous nickname"), 
		ErrStrType(ERR_NICKNAMEINUSE, "Nickname is already in use"), 
		ErrStrType(ERR_USERNOTINCHANNEL, "They aren't on that channel"), 
		ErrStrType(ERR_NOTONCHANNEL, "You're not on that channel"), 
		ErrStrType(ERR_USERONCHANNEL, "is already on channel"), 
		ErrStrType(ERR_NOTREGISTERED, "You have not registered"), 
		ErrStrType(ERR_NEEDMOREPARAMS, "Not enough parameters"), 
		ErrStrType(ERR_ALREADYREGISTRED, "You may not reregister"), 
		ErrStrType(ERR_PASSWDMISMATCH, "Password incorrect"), 
		ErrStrType(ERR_KEYSET, "Channel key already set"), 
		ErrStrType(ERR_CHANNELISFULL, "Cannot join channel (+l)"), 
		ErrStrType(ERR_UNKNOWNMODE, "is unknown mode char to me"), 
		ErrStrType(ERR_INVITEONLYCHAN, "Cannot join channel (+i)"), 
		ErrStrType(ERR_BANNEDFROMCHAN, "Cannot join channel (+b)"), 
		ErrStrType(ERR_BADCHANNELKEY, "Cannot join channel (+k)"), 
		ErrStrType(ERR_BADCHANMASK, "Bad channel mask"), 
		ErrStrType(ERR_CHANOPRIVSNEEDED, "You're not channel operator"), 
	};

	return ErrStrMapType(errs, errs + (sizeof(errs) / sizeof(errs[0])));
}

const ErrStrMapType	CommandException::ErrStrs = CommandException::initErrStrs();

CommandException::CommandException(): errNum(ERR_NONE), errStr() {}


CommandException::CommandException(ErrNum_t errNum, Client* client):
	errNum(errNum), errStr() {
	ErrStrMapType::const_iterator	it = ErrStrs.find(errNum);
	std::ostringstream				line;

	line << ":" << "ft_irc" << " " 
		<< errNum << " " 
		<< (client->getNickname().empty() ? "*" : client->getNickname());
	// if (!param.empty()) {
	// 	line << " " << param;
	// }
	line << " :";
	if (it != ErrStrs.end()) {
		line << it->second;
	} else {
		line << "Unknown error";
	}
	line << "\r\n";
	errStr = line.str();
}

CommandException::~CommandException() throw() {
}

const char*	CommandException::what() const throw() {
	return this->errStr.c_str();
}
