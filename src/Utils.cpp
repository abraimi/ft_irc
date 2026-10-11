#include "Utils.hpp"
#include "Client.hpp"
#include <cctype>
#include <iomanip>
#include <sstream>

std::string Utils::uppercase(const std::string& value) {
	std::string result = value;
	for (std::string::size_type i = 0; i < result.size(); ++i)
		result[i] = static_cast<char>(std::toupper(
			static_cast<unsigned char>(result[i])));
	return result;
}

bool Utils::equalsIgnoreCase(const std::string& left,
		const std::string& right) {
	if (left.size() != right.size())
		return false;
	for (std::string::size_type i = 0; i < left.size(); ++i)
		if (std::tolower(static_cast<unsigned char>(left[i]))
			!= std::tolower(static_cast<unsigned char>(right[i])))
			return false;
	return true;
}

std::string Utils::composeWireMessage(const std::string& prefix,
		const std::string& command, const std::string& parameters,
		const std::string& trailing, bool includeTrailing) {
	std::string message = prefix + " " + command;
	if (!parameters.empty())
		message += " " + parameters;
	if (includeTrailing)
		message += " :" + trailing;
	message += "\r\n";
	return message;
}

std::string Utils::composeNumericReply(const Client& recipient, int numeric,
		const std::string& parameters, const std::string& trailing) {
	std::ostringstream command;
	command << std::setfill('0') << std::setw(3) << numeric;
	std::string middle = recipient.getNickname().empty() ? "*" : recipient.getNickname();
	if (!parameters.empty())
		middle += " " + parameters;
	return composeWireMessage(":ft_irc", command.str(), middle, trailing, true);
}

std::string Utils::composeNumericReply(const Client& recipient, int numeric,
		const std::string& parameters) {
	std::ostringstream command;
	command << std::setfill('0') << std::setw(3) << numeric;
	std::string middle = recipient.getNickname().empty() ? "*" : recipient.getNickname();
	if (!parameters.empty())
		middle += " " + parameters;
	return composeWireMessage(":ft_irc", command.str(), middle, "", false);
}

std::string Utils::composeClientReply(const Client& sender,
		const std::string& command, const std::string& parameters,
		const std::string& trailing) {
	const std::string prefix = ":" + sender.getNickname() + "!"
		+ sender.getUsername() + "@" + sender.getHostname();
	return composeWireMessage(prefix, command, parameters, trailing, true);
}

std::string Utils::composeClientReply(const Client& sender,
		const std::string& command, const std::string& parameters) {
	const std::string prefix = ":" + sender.getNickname() + "!"
		+ sender.getUsername() + "@" + sender.getHostname();
	return composeWireMessage(prefix, command, parameters, "", false);
}

std::string Utils::composeServerReply(const std::string& command,
		const std::string& parameters, const std::string& trailing) {
	return composeWireMessage(":ft_irc", command, parameters, trailing, true);
}

std::string Utils::composeServerReply(const std::string& command,
		const std::string& parameters) {
	return composeWireMessage(":ft_irc", command, parameters, "", false);
}
