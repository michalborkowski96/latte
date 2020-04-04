#include "lexer.h"
#include <stdexcept>
#include <map>
#include <utility>
#include <memory>
#include <cctype>
#include <algorithm>

Token::Token(Type type, std::string&& data, size_t pos) : type(type), data(std::move(data)), pos(pos) {}
Token::Token(Type type, const std::string& data, size_t pos) : type(type), data(std::move(data)), pos(pos) {}

LexerException::LexerException(size_t pos) : pos(pos) {}

const std::map<Token::Type, std::string> Token::TYPE_NAMES {
	{Type::KEYWORD_CLASS, "class"},
	{Type::KEYWORD_RETURN, "return"},
	{Type::KEYWORD_IF, "if"},
	{Type::KEYWORD_WHILE, "while"},
	{Type::KEYWORD_FOR, "for"},
	{Type::KEYWORD_EXTENDS, "extends"},
	{Type::LITERAL_NUMBER, "number literal"},
	{Type::NAME, "name"},
	{Type::SEMICOLON, ";"},
	{Type::COMMA, ","},
	{Type::COLON, ":"},
	{Type::DOT, "."},
	{Type::LITERAL_STRING, "string literal"},
	{Type::LITERAL_TRUE, "true"},
	{Type::LITERAL_FALSE, "false"},
	{Type::OP_ASSIGN, "="},
	{Type::OP_ADD, "+"},
	{Type::OP_DIV, "/"},
	{Type::OP_MUL, "*"},
	{Type::OP_INC, "++"},
	{Type::OP_DEC, "--"},
	{Type::OP_MOD, "%"},
	{Type::OP_MIN, "-"},
	{Type::OP_NOT, "!"},
	{Type::OP_AND, "&&"},
	{Type::OP_OR, "||"},
	{Type::OP_LT, "<"},
	{Type::OP_LE, "<="},
	{Type::OP_GT, ">"},
	{Type::OP_GE, ">="},
	{Type::OP_EQ, "=="},
	{Type::OP_NE, "!="},
	{Type::BRACKET_OPEN, "("},
	{Type::BRACKET_CLOSE, ")"},
	{Type::CURLY_OPEN, "{"},
	{Type::CURLY_CLOSE, "}"},
	{Type::SQUARE_OPEN, "["},
	{Type::SQUARE_CLOSE, "]"},
	{Type::KEYWORD_NEW, "new"},
	{Type::KEYWORD_NULL, "null"},
	{Type::KEYWORD_VOID, "void"},
};

namespace {
	const std::map<std::string, Token::Type> SIMPLE_TOKENS = {
		{"+", Token::Type::OP_ADD},
		{"-", Token::Type::OP_MIN},
		{"++", Token::Type::OP_INC},
		{"--", Token::Type::OP_DEC},
		{"*", Token::Type::OP_MUL},
		{"/", Token::Type::OP_DIV},
		{"%", Token::Type::OP_MOD},
		{"!", Token::Type::OP_NOT},
		{"||", Token::Type::OP_OR},
		{"&&", Token::Type::OP_AND},
		{"<", Token::Type::OP_LT},
		{">", Token::Type::OP_GT},
		{"<=", Token::Type::OP_LE},
		{">=", Token::Type::OP_GE},
		{"==", Token::Type::OP_EQ},
		{"!=", Token::Type::OP_NE},
		{"=", Token::Type::OP_ASSIGN},
		{"(", Token::Type::BRACKET_OPEN},
		{")", Token::Type::BRACKET_CLOSE},
		{"{", Token::Type::CURLY_OPEN},
		{"}", Token::Type::CURLY_CLOSE},
		{"[", Token::Type::SQUARE_OPEN},
		{"]", Token::Type::SQUARE_CLOSE},
		{";", Token::Type::SEMICOLON},
		{":", Token::Type::COLON},
		{",", Token::Type::COMMA},
		{".", Token::Type::DOT},
	};

	const std::map<char, char> ESCAPED_CHARACTERS = {
		{'t', '\t'},
		{'b', '\b'},
		{'n', '\n'},
		{'r', '\r'},
		{'f', '\f'},
		{'\'', '\''},
		{'"', '"'},
		{'\\', '\\'},
	};

	auto max_size = [](const std::map<std::string, Token::Type>& dic){
		size_t r = 0;
		for(const auto& t : dic) {
			if(t.first.length() > r) {
				r = t.first.length();
			}
		}
		return r;
	};

	const size_t MAX_SIMPLE_TOKEN_SIZE = max_size(SIMPLE_TOKENS);

	const std::map<std::string, Token::Type> KEYWORDS = {
		{"class", Token::Type::KEYWORD_CLASS},
		{"return", Token::Type::KEYWORD_RETURN},
		{"if", Token::Type::KEYWORD_IF},
		{"else", Token::Type::KEYWORD_ELSE},
		{"while", Token::Type::KEYWORD_WHILE},
		{"for", Token::Type::KEYWORD_FOR},
		{"true", Token::Type::LITERAL_TRUE},
		{"false", Token::Type::LITERAL_FALSE},
		{"extends", Token::Type::KEYWORD_EXTENDS},
		{"new", Token::Type::KEYWORD_NEW},
		{"null", Token::Type::KEYWORD_NULL},
		{"void", Token::Type::KEYWORD_VOID},
	};

	const size_t MAX_KEYWORD_SIZE = max_size(KEYWORDS);
}

std::vector<Token> tokenize(const std::string& input) {
	std::vector<Token> result;
	if(input.size() == 0) {
		return result;
	}
	size_t pos = 0;

	const size_t limit = input.size();

	auto skip_whitespaces = [&](){
		while(pos <= limit && isspace(input[pos])) {
			++pos;
		}
	};

	auto is_identifier_char = [](char c) {
		return isalnum(c) || c == '_';
	};

	auto match_simple_token = [&](){
		for(size_t length = std::min(MAX_SIMPLE_TOKEN_SIZE, limit - pos); length > 0; --length) {
			std::string s = input.substr(pos, length);
			if(SIMPLE_TOKENS.find(s) != SIMPLE_TOKENS.end()) {
				result.push_back(Token(SIMPLE_TOKENS.at(s), std::move(s), pos));
				pos += length;
				return true;
			}
		}
		return false;
	};

	auto get_identifier = [&]() -> std::string {
		size_t begin = pos;
		size_t i = pos;
		if(input[i] == '_') {
			return "";
		}
		for(; i < limit && is_identifier_char(input[i]); ++i) {}
		return input.substr(begin, i - begin);
	};

	auto match_keyword = [&](std::string& identifier){
		if(identifier.size() > MAX_KEYWORD_SIZE) {
			return false;
		}
		if(KEYWORDS.find(identifier) != KEYWORDS.end()) {
			result.push_back(Token(KEYWORDS.at(identifier), std::move(identifier), pos));
			pos += identifier.size();
			return true;
		}
		return false;
	};

	auto match_string_literal = [&](){
		std::string str;
		if(pos >= limit || input[pos] != '"') {
			return false;
		}
		size_t i;
		size_t chunk_begin = pos + 1;
		for(i = pos + 1; i < limit && input[i] != '"'; ++i) {
			if(input[i] == '\\' && i + 1 < limit) {
				++i;
				if(ESCAPED_CHARACTERS.count(input[i]) == 0) {
					return false;
				}
				str += input.substr(chunk_begin, i - chunk_begin - 1);
				str += ESCAPED_CHARACTERS.at(input[i]);
				chunk_begin = i + 1;
			}
		}
		if(i == limit || input[i] != '"') {
			return false;
		}
		str += input.substr(chunk_begin, i - chunk_begin);
		result.push_back(Token(Token::Type::LITERAL_STRING, std::move(str), pos));
		pos = i + 1;
		return true;
	};

	while(true) {
		skip_whitespaces();
		if(pos >= limit) {
			break;
		}
		if(limit - pos >= 2 && input[pos] == '#') {
			while(++pos != limit && input[pos] != '\n') {}
			continue;
		}
		if(limit - pos >= 3 && input[pos] == '/' && input[pos + 1] == '/') {
			while(++pos != limit && input[pos] != '\n') {}
			continue;
		}
		if(limit - pos >= 3 && input[pos] == '/' && input[pos + 1] == '*') {
			pos += 2;
			while(pos + 1 < limit && (input[pos] != '*' || input[pos + 1] != '/')) {
				++pos;
			}
			pos += 2;
			continue;
		}
		if(match_simple_token()) {
			continue;
		}
		if(match_string_literal()) {
			continue;
		}
		std::string id = get_identifier();
		size_t off = id.size();
		if(id.empty()) {
			break;
		}
		if(match_keyword(id)) {
			pos += off;
			continue;
		}
		bool is_num = isdigit(id[0]) || id[0] == '-';
		if(is_num) {
			for(size_t i = 1; i < id.size(); ++i) {
				if(!isdigit(id[i])) {
					is_num = false;
					break;
				}
			}
		}
		if(is_num) {
			result.push_back(Token(Token::Type::LITERAL_NUMBER, std::move(id), pos));
		} else {
			result.push_back(Token(Token::Type::NAME, std::move(id), pos));
		}
		pos += off;
	}

	if(pos != limit) {
		throw LexerException(pos);
	}
	return result;
}
