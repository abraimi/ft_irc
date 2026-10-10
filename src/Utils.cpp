#include "Utils.hpp"

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
