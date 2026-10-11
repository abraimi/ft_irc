#include "MessageException.hpp"
#include "Utils.hpp"
#include <map>

ErrStrMapType MessageException::initErrStrs() {
	static const ErrStrType errs[] = {
		ErrStrType(ERR_NOSUCHNICK, "No such nick/channel"), 
		ErrStrType(ERR_NOSUCHCHANNEL, "No such channel"), 
		ErrStrType(ERR_CANNOTSENDTOCHAN, "Cannot send to channel"), 
		ErrStrType(ERR_NORECIPIENT, "No recipient given"), 
		ErrStrType(ERR_NOTEXTTOSEND, "No text to send"), 
		ErrStrType(ERR_TOOMANYTARGETS, "Too many targets"),
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
		ErrStrType(ERR_INVALIDMODEPARAM, "Invalid mode parameter"),
	};

	return ErrStrMapType(errs, errs + (sizeof(errs) / sizeof(errs[0])));
}

const ErrStrMapType	MessageException::ErrStrs = MessageException::initErrStrs();

MessageException::MessageException(): errNum(ERR_NONE), errStr() {}


MessageException::MessageException(ErrNum_t errNum, Client* client):
	errNum(errNum), errStr() {
	ErrStrMapType::const_iterator	it = ErrStrs.find(errNum);
	const std::string reason = it == ErrStrs.end() ? "Unknown error" : it->second;
	errStr = Utils::composeNumericReply(*client, static_cast<int>(errNum), "", reason);
}

MessageException::MessageException(ErrNum_t errNum, Client* client,
		const std::string& param): errNum(errNum), errStr() {
	ErrStrMapType::const_iterator	it = ErrStrs.find(errNum);
	const std::string reason = it == ErrStrs.end() ? "Unknown error" : it->second;
	errStr = Utils::composeNumericReply(*client, static_cast<int>(errNum), param, reason);
}

MessageException::~MessageException() throw() {}

const char*	MessageException::what() const throw() {
	return this->errStr.c_str();
}
