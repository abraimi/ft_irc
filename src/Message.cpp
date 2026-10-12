#include "Message.hpp"
#include "MessageException.hpp"
#include "Utils.hpp"
#include <cctype>
#include <map>
#include <utility>

static const std::size_t MAX_NICKNAME_LENGTH = 9;
static const std::size_t MAX_CHANNEL_LENGTH = 200;

Message::Message(Client *client)
	: client(client), cmdType(UNKNOWN), commandName(), args(), valid(false) {}

Message::~Message() {}

static bool isSeparator(char c) { return c == ' ' || c == '\t'; }

void Message::parseMessage(const std::string &line) {
	args.clear();
	cmdType = UNKNOWN;
	commandName.clear();
	valid = false;
	std::string clean = line;
	while (!clean.empty() && (clean[clean.size() - 1] == '\r' || clean[clean.size() - 1] == '\n'))
		clean.erase(clean.size() - 1);
	std::string::size_type pos = 0;
	while (pos < clean.size() && isSeparator(clean[pos]))
		++pos;
	if (pos == clean.size())
		return;
	if (clean[pos] == ':') {
		std::string::size_type prefixEnd = clean.find_first_of(" \t", pos);
		if (prefixEnd == std::string::npos)
			return;
		pos = prefixEnd;
		while (pos < clean.size() && isSeparator(clean[pos]))
			++pos;
	}
	std::string::size_type commandEnd = clean.find_first_of(" \t", pos);
	if (commandEnd == std::string::npos)
		commandName = clean.substr(pos);
	else
		commandName = clean.substr(pos, commandEnd - pos);
	if (commandName.empty())
		return;
	commandName = Utils::uppercase(commandName);
	cmdType = commandType(commandName);
	if (commandEnd != std::string::npos)
		pos = commandEnd;
	else
		pos = clean.size();
	while (pos < clean.size()) {
		while (pos < clean.size() && isSeparator(clean[pos]))
			++pos;
		if (pos == clean.size())
			break;
		if (clean[pos] == ':') {
			args.push_back(clean.substr(pos + 1));
			break;
		}
		std::string::size_type end = clean.find_first_of(" \t", pos);
		if (end == std::string::npos) {
			args.push_back(clean.substr(pos));
			break;
		}
		args.push_back(clean.substr(pos, end - pos));
		pos = end;
	}
	valid = true;
}

Command_t Message::commandType(const std::string &command) {
	typedef std::pair<std::string, Command_t> Entry;
	typedef std::map<std::string, Command_t> EntryMap;
	static const Entry entries[] = {
		Entry("PASS", PASS), Entry("USER", USER), Entry("NICK", NICK),
		Entry("JOIN", JOIN), Entry("PRIVMSG", PRIVMSG), Entry("KICK", KICK),
		Entry("INVITE", INVITE), Entry("TOPIC", TOPIC), Entry("MODE", MODE),
		Entry("CAP", CAP), Entry("PING", PING)
	};
	static const EntryMap types(entries, entries + sizeof(entries) / sizeof(entries[0]));
	EntryMap::const_iterator it = types.find(command);
	return it == types.end() ? UNKNOWN : it->second;
}

static bool validNickname(const std::string &nick) {
	static const std::string firstSpecial = "[]\\`_^{|}";
	static const std::string otherSpecial = "-[]\\`_^{|}";
	if (nick.empty() || nick.size() > MAX_NICKNAME_LENGTH
		|| (!std::isalpha(static_cast<unsigned char>(nick[0]))
			&& firstSpecial.find(nick[0]) == std::string::npos))
		return false;
	for (std::string::size_type i = 1; i < nick.size(); ++i) {
		unsigned char c = static_cast<unsigned char>(nick[i]);
		if (!std::isalpha(c) && !std::isdigit(c)
			&& otherSpecial.find(static_cast<char>(c)) == std::string::npos)
			return false;
	}
	return true;
}

static bool validChannel(const std::string &name) {
	if (name.size() < 2 || name.size() > MAX_CHANNEL_LENGTH
		|| (name[0] != '#' && name[0] != '&'))
		return false;
	for (std::string::size_type i = 0; i < name.size(); ++i) {
		const unsigned char character = static_cast<unsigned char>(name[i]);
		if (std::iscntrl(character) || name[i] == ',' || name[i] == ':')
			return false;
	}
	return true;
}

static bool getArgumentBounds(Command_t command, std::size_t &minimum,
		std::size_t &maximum) {
	typedef std::pair<Command_t, std::pair<std::size_t, std::size_t> > Entry;
	typedef std::map<Command_t, std::pair<std::size_t, std::size_t> > BoundsMap;
	static const Entry entries[] = {
		Entry(PASS, std::make_pair(1, 1)), Entry(USER, std::make_pair(4, 4)),
		Entry(NICK, std::make_pair(1, 1)), Entry(JOIN, std::make_pair(1, 2)),
		Entry(PRIVMSG, std::make_pair(2, 2)), Entry(KICK, std::make_pair(2, 3)),
		Entry(INVITE, std::make_pair(2, 2)), Entry(TOPIC, std::make_pair(1, 2)),
		Entry(MODE, std::make_pair(1, 0))
	};
	static const BoundsMap bounds(entries, entries + sizeof(entries) / sizeof(entries[0]));
	BoundsMap::const_iterator it = bounds.find(command);
	if (it == bounds.end())
		return false;
	minimum = it->second.first;
	maximum = it->second.second;
	return true;
}

static void validateModeArgs(const std::vector<std::string> &args, Client *client) {
	if (args.size() <= 1)
		return;
	bool hasMode = false;
	bool hasFlag = false;
	std::size_t required = 0;
	char sign = 0;
	for (std::string::size_type i = 0; i < args[1].size(); ++i) {
		char flag = args[1][i];
		if (flag == '+' || flag == '-') {
			if (i + 1 == args[1].size()
				|| (i > 0 && (args[1][i - 1] == '+' || args[1][i - 1] == '-')))
				throw MessageException(ERR_INVALIDMODEPARAM, client, args[1]);
			hasMode = true;
			sign = flag;
			continue;
		}
		if (!hasMode || sign == 0
			|| (flag != 'i' && flag != 't' && flag != 'k' && flag != 'o' && flag != 'l'))
			throw MessageException(ERR_UNKNOWNMODE, client, std::string(1, flag));
		hasFlag = true;
		if (flag == 'o' || flag == 'k' || (flag == 'l' && sign == '+'))
			++required;
	}
	if (!hasMode || !hasFlag || args.size() != required + 2)
		throw MessageException(ERR_INVALIDMODEPARAM, client, args[1]);
}

void Message::validateArgs() {
	if (!valid || cmdType == UNKNOWN)
		return;
	if (cmdType == NICK && args.empty())
		throw MessageException(ERR_NONICKNAMEGIVEN, client);
	if (cmdType == PRIVMSG && args.empty())
		throw MessageException(ERR_NORECIPIENT, client, "PRIVMSG");
	if (cmdType == PRIVMSG && args.size() == 1)
		throw MessageException(ERR_NOTEXTTOSEND, client);
	std::size_t minimum = 0;
	std::size_t maximum = 0;
	if (!getArgumentBounds(cmdType, minimum, maximum))
		return;
	if (args.size() < minimum || (maximum && args.size() > maximum))
		throw MessageException(ERR_NEEDMOREPARAMS, client, commandName);
	if (cmdType == NICK && !validNickname(args[0]))
		throw MessageException(args[0].empty() ? ERR_NONICKNAMEGIVEN : ERR_ERRONEUSNICKNAME, client);
	if ((cmdType == JOIN || cmdType == KICK || cmdType == TOPIC)
		&& !validChannel(args[0]))
		throw MessageException(ERR_BADCHANMASK, client, args[0]);
	if (cmdType == INVITE && !validNickname(args[0]))
		throw MessageException(ERR_ERRONEUSNICKNAME, client, args[0]);
	if (cmdType == INVITE && !validChannel(args[1]))
		throw MessageException(ERR_BADCHANMASK, client, args[1]);
	if (cmdType == PRIVMSG) {
		if (args[0].empty())
			throw MessageException(ERR_NORECIPIENT, client, "PRIVMSG");
		if (args[0].find(',') != std::string::npos)
			throw MessageException(ERR_TOOMANYTARGETS, client, args[0]);
		if (args[1].empty())
			throw MessageException(ERR_NOTEXTTOSEND, client);
	}
	if (cmdType == MODE)
		validateModeArgs(args, client);
}

Command_t Message::getType() const { return cmdType; }
const std::string &Message::getCommandName() const { return commandName; }
const std::vector<std::string> &Message::getArgs() const { return args; }
bool Message::getValid() const { return valid; }
