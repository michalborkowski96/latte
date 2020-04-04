#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "program_tree.h"
#include "location.h"

#include <iostream>

struct ParserException {
	const size_t pos;
	const std::string msg;
	const std::deque<std::tuple<const char*, std::shared_ptr<std::string>, size_t>> env;
	ParserException() = delete;
	ParserException(size_t pos, const std::string& msg, std::deque<std::tuple<const char*, std::shared_ptr<std::string>, size_t>>&& env);
	void operator()(std::ostream& o, const LocationTranslator& lt) const;
};

ProgramTree::Program parse(std::vector<Token>& tokens);

#endif
