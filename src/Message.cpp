#include "Message.hpp"
#include "Client.hpp"
#include "MessageException.hpp"
#include <map>
#include <string>

Message::Message(Client* client): client(client), authority(REGULAR), cmdType(UNKNOWN), args(), valid(false) {}

Message::~Message() {}

static bool	isSeparator(char c) {
	return (c == ' ' || c == '\t');
}

void	skipSpaces(const std::string& line, std::string::iterator& it) {
	for (; it != line.end(); it++) {
		if (!isSeparator(*it)) {
			break ;
		}
	}
}


std::string	Message::sanitize(const std::string &line)
{
	std::string	toClean = line;
	std::size_t	lastOf = toClean.find_last_not_of("\r\n");

	if (lastOf == std::string::npos) {
		toClean.clear();
	} else {
		toClean.erase(lastOf + 1);
	}

	return (toClean);
}

void	Message::tokenize(std::string& line, std::string::iterator& it) {
	std::string::iterator	textEnd, textBegin;

	for (; 0x69420F; ) {
		skipSpaces(line, it);
		if (it == line.end()) {
			break ;
		}
		if (*it == ':') {
			textEnd = line.end();
			for (; it + 1 != textEnd && isSeparator(*(textEnd - 1)); textEnd--) ;
			this->tokens.push_back(std::string(it + 1, textEnd));
			break ;
		}
		textBegin = it;
		for (; it != line.end() && !isSeparator(*it); it++) ;

		this->tokens.push_back(std::string(textBegin, it));
	}

}

void	Message::parseMessage(const std::string &line) {
	std::string				sanitized = this->sanitize(line);
	std::string::iterator	it = sanitized.begin();

	this->tokens.clear();
	skipSpaces(line, it);
	if (it == line.end()) {
		throw MessageException(ERR_NOTEXTTOSEND, this->client);
	}
	// ! Like this example: :nick!user@host PRIVMSG ...
	//! Which is skiped to leave PRIVMSG ... only
	if (*it == ':') {
		for (; it != sanitized.end() && !isSeparator(*it); it++) ;
		skipSpaces(sanitized, it);
		if (it == sanitized.end()) {
			throw MessageException(ERR_NOTEXTTOSEND, this->client);
		}
	}

	tokenize(sanitized, it);
	if (this->tokens.empty()) {
		throw MessageException(ERR_NOTEXTTOSEND, this->client);
	}
	for (std::size_t idx = 0; idx < this->tokens.size(); idx++) {
		if (!idx) {
			for (std::string::iterator it = this->tokens.at(idx).begin(); it != this->tokens.at(idx).end(); it++) {
				*it = static_cast<char>(std::toupper(*it));
			}
			this->cmdType = commandType(this->tokens.at(idx));
			continue ;
		}
		this->args.push_back(this->tokens.at(idx));
	}
	this->valid = true;
}

Command_t	Message::commandType(const std::string& command) {
	typedef std::pair<std::string, Command_t>	Entry;
	typedef std::map<std::string, Command_t> 	EntryMap;
    static const Entry entries[] = {
        Entry("USER", USER), Entry("PASS", PASS),
        Entry("NICK", NICK), Entry("JOIN", JOIN),
		Entry("PRIVMSG", PRIVMSG), Entry("KICK", KICK),
		Entry("KICK", KICK), Entry("INVITE", INVITE),
		Entry("TOPIC", TOPIC), Entry("MODE", MODE)
    };
	static const EntryMap		types(entries, entries + (sizeof(entries) / sizeof(entries[0])));
	EntryMap::const_iterator	it = types.find(command);

	if (it == types.end()) {
		return UNKNOWN;
	}
	return it->second;
}

static bool	validNickname(const std::string& nick) {
	static const std::string	special = "-[]\\`^{}_";

	if (nick.empty() || nick.size() > 9) {
		return false;
	}
	for (std::string::const_iterator it = nick.begin(); it != nick.end(); it++) {
		unsigned char c = static_cast<unsigned char>(*it);

		if (!std::isalpha(c) && !std::isdigit(c)
			&& special.find(static_cast<char>(c)) == std::string::npos) {
			return false;
		}
		if (it == nick.begin() && std::isdigit(c)) {
			return false;
		}
	}
	return true;
}

static bool	validChannel(const std::string& name) {
	if (name.empty() || name.size() > 200
		|| (name[0] != '#' && name[0] != '&')) {
		return false;
	}
	for (std::string::const_iterator it = name.begin(); it != name.end(); it++) {
		if (*it == ' ' || *it == '\t' || *it == ',' || *it == '\a'
			|| *it == '\r' || *it == '\n') {
			return false;
		}
	}
	return true;
}
//TODO: This function may need to be removed and replaced a more thorough one.
static bool	validChannelList(const std::string& list) {
	std::string::size_type	start = 0;

	while (start <= list.size()) {
		std::string::size_type	comma = list.find(',', start);
		std::string::size_type	len = (comma == std::string::npos)
			? list.size() - start : comma - start;

		if (!validChannel(list.substr(start, len))) {
			return false;
		}
		if (comma == std::string::npos) {
			break ;
		}
		start = comma + 1;
	}
	return true;
}

void	Message::validateArgs() {
	typedef std::pair<std::size_t, std::size_t>	argCount;

	static const argCount	limits[] = {
		argCount(1, 1),	// PASS
		argCount(4, 4),	// USER
		argCount(1, 1),	// NICK
		argCount(1, 2),	// JOIN
		argCount(2, 2),	// PRIVMSG
		argCount(2, 3),	// KICK
		argCount(2, 2),	// INVITE
		argCount(1, 2),	// TOPIC
		argCount(1, 0)	// MODE: no upper bound
	};

	if (this->cmdType < PASS || this->cmdType > MODE) {
		throw	MessageException(ERR_UNKNOWNCOMMAND, client);
	}
	const argCount& count = limits[this->cmdType];

	if (this->args.size() < count.first || (count.second && this->args.size() > count.second)) {
		throw	MessageException(ERR_NEEDMOREPARAMS, client);
	}

	if (this->cmdType == PASS && this->args[0].empty()) {
		throw MessageException(ERR_NEEDMOREPARAMS, client);
	}

	if (this->cmdType == NICK && !validNickname(this->args[0])) {
		throw MessageException(ERR_ERRONEUSNICKNAME, client);
	}
	if (this->cmdType == JOIN && !validChannelList(this->args[0])) {
		throw MessageException(ERR_BADCHANMASK, client);
	}

	if (this->cmdType == PRIVMSG) {
		if (this->args.size() && this->args.at(0).empty()) {
			throw MessageException(ERR_NORECIPIENT, client);
		}
		if (this->args.size() > 1 && this->args.at(1).empty()) {
			throw MessageException(ERR_NOTEXTTOSEND, client);
		}
	}
	/*
		Channel specific Checks
	*/
	if (this->cmdType == KICK || this->cmdType == TOPIC || this->cmdType == MODE) {
		if (!validChannel(this->args[0])) {
			throw MessageException(ERR_BADCHANMASK, client);
		}
	}
	if (this->cmdType == INVITE) {
		if (this->args.size() && !validNickname(this->args[0])) {
			throw MessageException(ERR_ERRONEUSNICKNAME, client);
		}
		if (this->args.size() > 1 && !validChannel(this->args[1])) {
			throw MessageException(ERR_BADCHANMASK, client);
		}
	}
}

/*
	Getters
*/

Authority_t	Message::getAuthority() const {
	return this->authority;
}

Command_t	Message::getType() const {
	return this->cmdType;
}

std::vector<std::string>	Message::getArgs() const {
	return this->args;
}

bool	Message::getValid() const {
	return this->valid;
}
