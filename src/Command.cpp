#include "Command.hpp"
#include "Client.hpp"
#include <cstddef>
#include <map>
#include <string>
#include <utility>

Command::Command(): client(NULL), authority(REGULAR), type(UNKNOWN), args(), valid(false), errNum(0) {}

Command::Command(Client* client): client(client), authority(REGULAR), type(UNKNOWN), args(), valid(false), errNum(0) {}

Command::~Command() {}

static bool	isSeparator(char c) {
	return (c == ' ' || c == '\t');
}

std::string	Command::sanitize(const std::string &line)
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

/**
* Split `line` from index `start` on whitespace into this->tokens.
*
* A token starting with ':' takes the REST of the line as ONE argument
* (RFC 1459 trailing parameter):
*     PRIVMSG #42 :hello world   ->  ["PRIVMSG", "#42", "hello world"]
*     USER a b c :Real Name      ->  ["USER", "a", "b", "c", "Real Name"]
* The trailing text may be empty ("NICK :") and is kept as "" so the
* validation step can reject it. tokens is cleared by parseCommand().
*/

void	skipSpaces(const std::string& line, std::string::iterator& it) {
	for (; it != line.end(); it++) {
		if (!isSeparator(*it)) {
			break ;
		}
	}
}

void	Command::tokenize(std::string& line, std::string::iterator& it) {
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

/**
* @brief This function parses the buffer in the client class which is used to construct this class
*
* @param
* @return Command_t: which is a struct consisting of [cmd, args, valid, errNum].
**/
// TODO: this will change later since the struct will be merged into the class private attributes.
void	Command::parseCommand(const std::string &line) {
	std::string				sanitized = this->sanitize(line);
	std::string::iterator	it = sanitized.begin();

	this->tokens.clear();
	skipSpaces(line, it);
	if (it == line.end()) {
		return ; // TODO: Check the errNUms to set it before exiting the function call stack;
	}
	// ! This might look repetetive as in tekenize, but this one checks ':' at the start of the command & skips it, meanwhile tokenize checks it at the mid/end;
	// ! Like this example: :nick!user@host PRIVMSG ...
	//! Which is skiped to leave PRIVMSG ... only
	if (*it == ':') {
		for (; it != sanitized.end() && !isSeparator(*it); it++) ;
		skipSpaces(sanitized, it);
		if (it == sanitized.end()) {
			return ; // TODO: Check the errNUms to set it before exiting the function call stack;
		}
	}

	tokenize(sanitized, it);
	if (this->tokens.empty()) {
		return ; // TODO: Check the errNUms to set it before exiting the function call stack;
	}
	this->valid = true;
	for (std::size_t idx = 0; idx < this->tokens.size(); idx++) {
		if (!idx) {
			// TODO: Create a function that will return the appropriate cmd type from a string;
			this->type = commandType(this->tokens.at(idx));
			continue ;
		}
		this->args.push_back(this->tokens.at(idx));
	}
}

Command_t	Command::commandType(const std::string& command) {
	typedef std::pair<std::string, Command_t> Entry;

    static const Entry entries[] = {
        Entry("USER", USER), Entry("PASS", PASS),
        Entry("NICK", NICK), Entry("JOIN", JOIN),
		Entry("PRIVMSG", PRIVMSG), Entry("KICK", KICK),
		Entry("KICK", KICK), Entry("INVITE", INVITE),
		Entry("TOPIC", TOPIC), Entry("MODE", MODE)
    };
	static const std::map<std::string, Command_t> types(entries, entries + (sizeof(entries) / sizeof(entries[0])));

	std::map<std::string, Command_t>::const_iterator	it = types.find(command);

	if (it == types.end()) {
		return UNKNOWN;
	}
	return it->second;
}
/*
case PASS:    minArgs = maxArgs = 1; break;
case USER:    minArgs = maxArgs = 4; break;
case NICK:    minArgs = maxArgs = 1; break;
case JOIN:    minArgs = 1; maxArgs = 2; break;
case PRIVMSG: minArgs = maxArgs = 2; break;
case PART:    minArgs = 1; maxArgs = 2; break;
case KICK:    minArgs = 2; maxArgs = 3; break;
case INVITE:  minArgs = maxArgs = 2; break;
case TOPIC:   minArgs = 1; maxArgs = 2; break;
case MODE:    minArgs = 1; break;
*/

void	validateArgs() {
	// TODO: Create this function on which we would validate the args depending on the cmd.
	// typedef std::pai ;
}

/*
	Getters
*/

Authority_t	Command::getAuthority() const {
	return this->authority;
}

Command_t	Command::getType() const {
	return this->type;
}

std::vector<std::string>	Command::getArgs() const {
	return this->args;
}

bool	Command::getValid() const {
	return this->valid;
}

std::size_t	Command::getErrNum() const {
	return this->errNum;
}
