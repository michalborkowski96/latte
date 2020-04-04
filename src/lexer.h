#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <map>

struct Token {
	enum class Type {
		KEYWORD_CLASS, KEYWORD_RETURN, KEYWORD_IF, KEYWORD_WHILE, KEYWORD_EXTENDS, KEYWORD_NEW, KEYWORD_ELSE, KEYWORD_NULL, KEYWORD_VOID, KEYWORD_FOR,
		LITERAL_NUMBER, NAME, SEMICOLON, COMMA, COLON, DOT, LITERAL_STRING, LITERAL_TRUE, LITERAL_FALSE,
		OP_ASSIGN,
		OP_ADD, OP_DIV, OP_MUL, OP_INC, OP_DEC, OP_MOD,
		OP_MIN,
		OP_NOT, OP_AND, OP_OR,
		OP_LT, OP_LE, OP_GT, OP_GE, OP_EQ, OP_NE,
		BRACKET_OPEN, BRACKET_CLOSE, CURLY_OPEN, CURLY_CLOSE, SQUARE_OPEN, SQUARE_CLOSE
	};

	static const std::map<Type, std::string> TYPE_NAMES;

	const Type type;
	std::string data;
	const size_t pos;

	Token() = delete;
	Token(const Token&) = default;
	Token(Token&&) = default;
	Token(Type type, std::string&& data, size_t pos);
	Token(Type type, const std::string& data, size_t pos);
};

struct LexerException : public std::exception {
	const size_t pos;
	LexerException() = delete;
	LexerException(size_t pos);
};

std::vector<Token> tokenize(const std::string& input);

#endif
