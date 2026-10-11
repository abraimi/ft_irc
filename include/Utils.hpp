#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

class Client;

class Utils {
	private:
		Utils();
		Utils(const Utils& other);
		Utils& operator=(const Utils& other);
		static std::string composeWireMessage(const std::string& prefix,
			const std::string& command, const std::string& parameters,
			const std::string& trailing, bool includeTrailing);

	public:
		static std::string	uppercase(const std::string& value);
		static bool			equalsIgnoreCase(const std::string& left, const std::string& right);
		static std::string	composeNumericReply(const Client& recipient, int numeric,
			const std::string& parameters, const std::string& trailing);
		static std::string	composeNumericReply(const Client& recipient, int numeric,
			const std::string& parameters);
		static std::string	composeClientReply(const Client& sender,
			const std::string& command, const std::string& parameters,
			const std::string& trailing);
		static std::string	composeClientReply(const Client& sender,
			const std::string& command, const std::string& parameters);
		static std::string	composeServerReply(const std::string& command,
			const std::string& parameters, const std::string& trailing);
		static std::string	composeServerReply(const std::string& command,
			const std::string& parameters);
};

#endif
