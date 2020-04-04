#include "location.h"

std::map<size_t, size_t> LocationTranslator::get_line_breaks(const std::string& str) {
	std::map<size_t, size_t> lb;
	lb[0] = 0;
	size_t lines = 0;
	for(size_t i = 0; i < str.size(); ++i) {
		if(str[i] == '\n') {
			lb[i] = ++lines;
		}
	}
	return lb;
}

std::string LocationTranslator::operator()(size_t pos) const {
	if(pos >= filesize) {
		return "endfile";
	}
	auto v = line_breaks.upper_bound(pos);
	size_t k;
	if(v == line_breaks.end()) {
		k = line_breaks.rbegin()->first;
	} else {
		k = (--v)->first;
	}
	v = line_breaks.find(k);
	return "line " + std::to_string(v->second) + ", column " + std::to_string(pos - v->first);
}

LocationTranslator::LocationTranslator(const std::string& str): line_breaks(get_line_breaks(str)), filesize(str.size()) {}
