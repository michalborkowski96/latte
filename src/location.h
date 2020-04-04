#ifndef LOCATION_H
#define LOCATION_H

#include <map>
#include <string>

class LocationTranslator {
	const std::map<size_t, size_t> line_breaks;
	const size_t filesize;

	static std::map<size_t, size_t> get_line_breaks(const std::string& str);

public:

	std::string operator()(size_t pos) const;

	LocationTranslator() = delete;
	LocationTranslator(const std::string& str);
};

#endif
