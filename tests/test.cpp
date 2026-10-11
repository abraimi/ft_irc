#include "Message.hpp"
#include <iostream>
#include <string>
#include <vector>

/**
 * Test main for the parser (sanitization + tokenization).
 * Run with:  make test
 * Exits 0 when every check passes, 1 otherwise.
 */

static int	g_checks = 0;
static int	g_failures = 0;

static std::string	join(const std::vector<std::string> &v)
{
	std::string	out;

	for (size_t i = 0; i < v.size(); i++)
	{
		if (i)
			out += "|";
		out += v[i];
	}
	return (out);
}

/* parse `line` once and compare validity + Args (Args joined with "|") */
static void	expect(const std::string &name, const std::string &line,
					bool expValid, const std::string &expArgs)
{
	Message		c;
	Command_t	cmd = c.parseMessage(line);
	std::string	got = join(cmd.args); // empty for invalid lines

	g_checks++;
	if (cmd.valid != expValid || got != expArgs
		|| cmd.type != UNKNOWN // lookup step not implemented yet
		|| cmd.errNum != 0)
	{
		g_failures++;
		std::cout << "FAIL  " << name << "\n"
			<< "        line    : |" << line << "|\n"
			<< "        valid   : got " << cmd.valid << ", want " << expValid << "\n"
			<< "        args    : got |" << got << "|, want |" << expArgs << "|\n"
			<< "        type : got " << (int)cmd.type << ", want -1 (UNKNOWN)\n"
			<< "        numeric : got " << cmd.errNum << ", want 0" << std::endl;
		return ;
	}
	std::cout << "ok    " << name << std::endl;
}

/* state checks that are not about one specific line */
static void	expectState(const std::string &name, bool cond)
{
	g_checks++;
	if (!cond)
	{
		g_failures++;
		std::cout << "FAIL  " << name << std::endl;
		return ;
	}
	std::cout << "ok    " << name << std::endl;
}

int	main()
{
	std::cout << "-- sanitization --" << std::endl;
	expect("bare CRLF stripped", "NICK reda\r\n", true, "reda");
	expect("bare LF stripped", "NICK reda\n", true, "reda");
	expect("empty line invalid", "", false, "");
	expect("blank line invalid", "    \t  ", false, "");
	expect("leading spaces skipped", "  NICK reda", true, "reda");
	expect("prefix without command", ":just.a.prefix", false, "");
	expect("bare colon line", ":", false, "");

	std::cout << "-- tokenization --" << std::endl;
	expect("last token flushed", "NICK reda", true, "reda");
	expect("two plain params", "PASS pass word", true, "pass|word");
	expect("trailing param is one arg", "PRIVMSG #42 :hello world", true,
		"#42|hello world");
	expect("USER with realname trailing", "USER a b c :Real Name", true,
		"a|b|c|Real Name");
	expect("empty trailing kept", "NICK :", true, "");
	expect("double spaces inside trailing kept", "PRIVMSG #42 :hello  world",
		true, "#42|hello  world");
	expect("multiple spaces between params", "  JOIN   #42    key   ",
		true, "#42|key");
	expect("tab separators", "NICK\treda\t", true, "reda");
	expect("colon not at token start is normal", "JOIN #42:key", true, "#42:key");
	expect("trailing spaces trimmed from trailing text",
		"PRIVMSG #42 :hi\t ", true, "#42|hi");

	std::cout << "-- prefix --" << std::endl;
	expect("nick!user@host prefix skipped",
		":nick!user@host PRIVMSG #42 :hi there", true, "#42|hi there");
	expect("server prefix skipped", ":irc.example.com 001 reda :welcome",
		true, "reda|welcome");

	std::cout << "-- parser state --" << std::endl;
	{
		Message		c;
		Command_t	first = c.parseMessage("FIRST one");
		Command_t	again = c.parseMessage("SECOND two"); // same parser reused

		expectState("tokens cleared between parses",
			first.args.size() == 1 && first.args[0] == "one"
			&& again.args.size() == 1 && again.args[0] == "two");
	}
	{
		Command_t	def;

		expectState("default Command_t is UNKNOWN/invalid",
			def.type == UNKNOWN && def.valid == false
			&& def.errNum == 0 && def.authority == REGULAR
			&& def.args.empty());
	}

	std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks
		<< " checks passed" << std::endl;
	if (g_failures)
	{
		std::cout << g_failures << " FAILED" << std::endl;
		return (1);
	}
	std::cout << "ALL OK" << std::endl;
	return (0);
}
