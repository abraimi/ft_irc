#include "Command.hpp"
#include "Client.hpp"
#include <string>

Command::Command(): client(NULL) {}

Command::Command(Client* client): client(client) {}

Command::~Command() {}

/**
* IRC parameter separators are SPACE (0x20) and HTAB. Everything here is
* index based and bounds checked, so unlike the old iterator version it can
* never walk past end() on trailing whitespace.
*/
static bool	isSeparator(char c)
{
	return (c == ' ' || c == '\t');
}

static std::size_t	skipSpaces(const std::string &line, std::size_t i)
{
	while (i < line.size() && isSeparator(line[i]))
		i++;
	return (i);
}

static std::string	sanitize(const std::string &line)
{
	std::string				clean = line;
	while (!clean.empty() && (clean[clean.size() - 1] == '\r'
			|| clean[clean.size() - 1] == '\n'))
		clean.erase(clean.size() - 1);
	return (clean);
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
void	Command::tokenize(const std::string &line, std::size_t start)
{
	std::size_t i = start;

	while (true)
	{
		i = skipSpaces(line, i);
		if (i >= line.size())
			return ;
		if (line[i] == ':')
		{
			std::string trailing = line.substr(i + 1);

			while (!trailing.empty() && isSeparator(trailing[trailing.size() - 1]))
				trailing.erase(trailing.size() - 1);
			this->tokens.push_back(trailing);
			return ;
		}
		std::size_t begin = i;

		while (i < line.size() && !isSeparator(line[i]))
			i++;
		this->tokens.push_back(line.substr(begin, i - begin));
	}
}

/**
* @brief This function parses the buffer in the client class which is used to construct this class
*
* @param
* @return Command_t: which is a struct consisting of [cmd, args, valid, errNum].
**/
Command_t	Command::parseCommand(const std::string &line)
{
	Command_t	cmd;
	std::string	clean = sanitize(line);
	std::size_t	pos = 0;

	this->tokens.clear();

	pos = skipSpaces(clean, 0);
	if (pos >= clean.size())
		return (cmd); // empty / blank line : no command, silently ignore

	// optional message prefix:  :nick!user@host PRIVMSG ...
	if (clean[pos] == ':')
	{
		while (pos < clean.size() && !isSeparator(clean[pos]))
			pos++;
		pos = skipSpaces(clean, pos);
		if (pos >= clean.size())
			return (cmd); // prefix without a command behind it
	}

	tokenize(clean, pos);
	if (this->tokens.empty())
		return (cmd);

	// Args holds the parameters only ; tokens[0] is the command name and is
	// consumed by the next step (name -> CommandTypes lookup + validation).
	for (std::size_t i = 1; i < this->tokens.size(); i++)
		cmd.args.push_back(this->tokens[i]);
	cmd.valid = true;
	return (cmd);
}
