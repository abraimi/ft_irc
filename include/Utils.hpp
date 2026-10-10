#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

class Utils {
	private:
		Utils();
		Utils(const Utils& other);
		Utils& operator=(const Utils& other);

	public:
		static std::string	uppercase(const std::string& value);
		static bool			equalsIgnoreCase(const std::string& left, const std::string& right);
};

#endif
