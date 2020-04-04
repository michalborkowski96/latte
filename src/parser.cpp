#include "parser.h"

#include <stdexcept>
#include <type_traits>
#include <functional>
#include <list>

using namespace ProgramTree;

void ParserException::operator()(std::ostream& o, const LocationTranslator& lt) const {
	o << "Parsing error at " << lt(pos) << ":\n" << msg << "\n";
	for(const auto& e : env) {
		o << "while parsing ";
		if(std::get<0>(e)) {
			o << std::get<0>(e);
		}
		if(std::get<1>(e)) {
			o << ' ' << *(std::get<1>(e));
		}
		o << " starting at " << lt(std::get<2>(e)) << '\n';
	}
}

ParserException::ParserException(size_t pos, const std::string& msg, std::deque<std::tuple<const char*, std::shared_ptr<std::string>, size_t>>&& env) : pos(pos), msg(msg), env(std::move(env)) {}

namespace {

	class Parser {
		const std::vector<Token>& tokens;
		std::vector<Token>::const_iterator pos;
		std::vector<Token>::const_iterator end;
		std::deque<std::tuple<const char*, std::shared_ptr<std::string>, size_t>> env;

		template<typename T, typename ...Ts>
		std::unique_ptr<T> make_node(size_t begin, Ts&&... args) {
			size_t e = -1;
			if(pos != end) {
				auto ei = pos - 1;
				e = ei->pos + ei->data.size() - 1;
			} else if (tokens.rbegin() != tokens.rend()) {
				auto i = tokens.rbegin();
				e = i->pos + i->data.size() - 1;
			}
			return std::make_unique<T>(begin, e, std::forward<Ts>(args)...);
		}

		void check_eof() {
			if(pos == end) throw make_error("Unexpected EOF");
		}

		void push_env(const char* name) {
			env.push_back(std::make_tuple(name, nullptr, pos == end ? -1 : pos->pos));
		}

		void update_env(const std::string& details) {
			std::get<1>(env.back()) = std::make_shared<std::string>(details);
		}

		void update_env() {
			std::get<1>(env.back()) = std::shared_ptr<std::string>(nullptr);
		}

		void pop_env() {
			env.pop_back();
		}

		void next_token() {
			++pos;
			check_eof();
		}

		bool check_pos_type(Token::Type t) {
			return pos->type == t;
		}

		template<typename ...Ts>
		bool check_pos_type(Token::Type t, Ts... args) {
			if(pos->type == t) {
				return true;
			}
			return check_pos_type(args...);
		}

		const std::string get_pos_type_name(Token::Type t) {
			return Token::TYPE_NAMES.at(t);
		}

		template<typename ...Ts>
		const std::string get_pos_type_name(Token::Type t, Ts... args) {
			return Token::TYPE_NAMES.at(t) + " or " + get_pos_type_name(args...);
		}

		template<typename ...Ts>
		void expect(Ts... args) {
			check_eof();
			if(!check_pos_type(args...)) {
				throw make_error(std::string("Unexpected token: ") + pos->data + ", expected: " + get_pos_type_name(args...));
			}
		}

		std::string parse_type_string(size_t* begin = nullptr, bool allow_void = false) {
			if(allow_void) {
				expect(Token::Type::NAME, Token::Type::KEYWORD_VOID);
			} else {
				expect(Token::Type::NAME);
			}
			if(begin) {
				*begin = pos->pos;
			}
			if(pos->type == Token::Type::KEYWORD_VOID) {
				++pos;
				return VOID_NAME;
			}
			std::string type = pos->data;
			++pos;
			if(pos != end && pos->type == Token::Type::SQUARE_OPEN) {
				++pos;
				expect(Token::Type::SQUARE_CLOSE);
				++pos;
				type += "[]";
			}
			return type;
		}

		std::unique_ptr<Definition> parse_definition(bool only_declarations) {
			push_env("variable definition");
			size_t begin;
			std::string type(parse_type_string(&begin));
			std::vector<std::pair<std::string, std::unique_ptr<Expression>>> defs;
			while(expect(Token::Type::NAME), true) {
				std::string name = pos->data;
				update_env(name);
				next_token();
				std::unique_ptr<Expression> value = nullptr;
				if(!only_declarations && pos->type == Token::Type::OP_ASSIGN) {
					next_token();
					value = parse_expression();
				}
				update_env();
				defs.push_back(std::make_pair(std::move(name), std::move(value)));
				if(pos->type == Token::Type::SEMICOLON) {
					++pos;
					break;
				} else if(pos->type == Token::Type::COMMA) {
					++pos;
					continue;
				} else {
					expect(Token::Type::SEMICOLON, Token::Type::COMMA);
				}
			}
			pop_env();
			return make_node<Definition>(begin, std::move(type), std::move(defs));
		}

		std::unique_ptr<Statement> parse_statement() {
			push_env("statement");
			check_eof();
			size_t begin = pos->pos;
			std::unique_ptr<Statement> result;
			if(pos->type == Token::Type::SEMICOLON) {
				++pos;
				result = make_node<Empty>(begin);
			} else if(pos->type == Token::Type::CURLY_OPEN) {
				result = parse_block();
			} else if(pos->type == Token::Type::KEYWORD_WHILE) {
				++pos;
				expect(Token::Type::BRACKET_OPEN);
				++pos;
				std::unique_ptr<Expression> condition = parse_expression();
				expect(Token::Type::BRACKET_CLOSE);
				++pos;
				result = make_node<While>(begin, std::move(condition), parse_statement());
			} else if(pos->type == Token::Type::KEYWORD_FOR) {
				++pos;
				expect(Token::Type::BRACKET_OPEN);
				++pos;
				expect(Token::Type::NAME);
				std::string var_type = pos->data;
				++pos;
				expect(Token::Type::NAME);
				std::string var_name = pos->data;
				++pos;
				expect(Token::Type::COLON);
				++pos;
				std::unique_ptr<Expression> array = parse_expression();
				expect(Token::Type::BRACKET_CLOSE);
				++pos;
				result = make_node<For>(begin, std::move(var_type), std::move(var_name), std::move(array), parse_statement());
			} else if(pos->type == Token::Type::KEYWORD_IF) {
				++pos;
				expect(Token::Type::BRACKET_OPEN);
				++pos;
				std::unique_ptr<Expression> condition = parse_expression();
				expect(Token::Type::BRACKET_CLOSE);
				++pos;
				std::unique_ptr<Statement> case_then = parse_statement();
				std::unique_ptr<Statement> case_else = nullptr;
				if(pos != end && pos->type == Token::Type::KEYWORD_ELSE) {
					++pos;
					case_else = parse_statement();
				}
				result = make_node<If>(begin, std::move(condition), std::move(case_then), std::move(case_else));
			} else if(pos->type == Token::Type::KEYWORD_RETURN) {
				next_token();
				if(pos->type == Token::Type::SEMICOLON) {
					++pos;
					result = make_node<Return>(begin, nullptr);
				} else {
					std::unique_ptr<Expression> val = parse_expression();
					expect(Token::Type::SEMICOLON);
					++pos;
					result = make_node<Return>(begin, std::move(val));
				}
			} else if(pos->type == Token::Type::NAME && pos + 1 != end && ((pos + 1)->type == Token::Type::NAME || ((pos + 1)->type == Token::Type::SQUARE_OPEN && pos + 2 != end && (pos + 2)->type == Token::Type::SQUARE_CLOSE))) {
				result = parse_definition(false);
			} else {
				std::unique_ptr<Expression> e = parse_expression();
				check_eof();
				if(pos->type == Token::Type::OP_ASSIGN) {
					++pos;
					result = make_node<Assignment>(begin, std::move(e), parse_expression());
				} else if (pos->type == Token::Type::OP_INC) {
					++pos;
					result = make_node<Incrementation>(begin, std::move(e));
				} else if (pos->type == Token::Type::OP_DEC) {
					++pos;
					result = make_node<Decrementation>(begin, std::move(e));
				} else {
					result = make_node<ExprStatement>(begin, parse_expression(std::move(e)));
				}
				expect(Token::Type::SEMICOLON);
				++pos;
			}
			pop_env();
			return result;
		}

		std::unique_ptr<Block> parse_block() {
			push_env("block");
			expect(Token::Type::CURLY_OPEN);
			size_t begin = pos->pos;
			next_token();
			std::vector<std::unique_ptr<Statement>> statements;
			while(pos != end && pos->type != Token::Type::CURLY_CLOSE) {
				statements.push_back(parse_statement());
			}
			++pos;
			pop_env();
			return make_node<Block>(begin, std::move(statements));
		}

		Function parse_function() {
			push_env("function");
			size_t dec_begin;
			std::string return_type(parse_type_string(&dec_begin, true));
			expect(Token::Type::NAME);
			std::string name(pos->data);
			update_env(name);
			++pos;
			expect(Token::Type::BRACKET_OPEN);
			std::vector<std::pair<std::string, std::string>> args;
			next_token();
			while(pos != end && pos->type != Token::Type::BRACKET_CLOSE) {
				std::string arg_type(parse_type_string());
				expect(Token::Type::NAME);
				std::string arg_name(pos->data);
				++pos;
				expect(Token::Type::COMMA, Token::Type::BRACKET_CLOSE);
				args.push_back(std::make_pair<std::string, std::string>(std::move(arg_type), std::move(arg_name)));
				if(pos->type == Token::Type::COMMA) {
					++pos;
				}
			}
			expect(Token::Type::BRACKET_CLOSE);
			size_t dec_end = pos->pos;
			++pos;
			std::unique_ptr<Block> body = parse_block();
			pop_env();
			return Function(dec_begin, dec_end, std::move(name), std::move(return_type), std::move(args), std::move(body));
		}

		Class parse_class() {
			push_env("class");
			expect(Token::Type::KEYWORD_CLASS);
			size_t dec_begin = pos->pos;
			next_token();
			expect(Token::Type::NAME);
			std::string name(pos->data);
			update_env(name);
			std::string superclass;
			next_token();
			if(pos->type == Token::Type::KEYWORD_EXTENDS) {
				next_token();
				expect(Token::Type::NAME);
				superclass = pos->data;
				next_token();
			}
			expect(Token::Type::CURLY_OPEN);
			size_t dec_end = pos->pos;
			next_token();
			std::vector<std::unique_ptr<Definition>> variables;
			std::vector<std::shared_ptr<Function>> functions;
			while(pos != end && pos->type != Token::Type::CURLY_CLOSE) {
				auto begin = pos;
				next_token();
				if(pos->type == Token::Type::SQUARE_OPEN) {
					next_token();
					next_token();
				}
				next_token();
				if(pos->type == Token::Type::BRACKET_OPEN) {
					pos = begin;
					functions.push_back(std::make_shared<Function>(parse_function()));
				} else {
					pos = begin;
					variables.push_back(parse_definition(true));
				}
			}
			expect(Token::Type::CURLY_CLOSE);
			++pos;
			pop_env();
			return Class(dec_begin, dec_end, std::move(name), std::move(superclass), std::move(functions), std::move(variables));
		}

		using Priority = unsigned char;

		static const std::map<Token::Type, Priority> BIN_OP_PRIORITIES;

		static const std::map<Token::Type, std::function<std::unique_ptr<Expression>(std::unique_ptr<Expression>&& left, std::unique_ptr<Expression>&& right)>> BIN_OP_CONSTRUCTORS;

		const std::map<Token::Type, std::function<std::unique_ptr<Expression>(Parser&)>> SIMPLE_PARSERS = {
			{Token::Type::BRACKET_OPEN, [](Parser& p){
				++p.pos;
				std::unique_ptr<Expression> expr = p.parse_expression();
				if(p.pos == p.end || p.pos->type != Token::Type::BRACKET_CLOSE) {
					throw p.make_error("Matching bracket not found.");
				}
				++p.pos;
				return expr;
			}},
			{Token::Type::NAME, [](Parser& p){
				size_t begin = p.pos->pos;
				std::string name((p.pos)++->data);
				return p.make_node<Variable>(begin, std::move(name));
			}},
			{Token::Type::KEYWORD_NULL, [](Parser& p){
				size_t begin = (p.pos++)->pos;
				return p.make_node<Null>(begin);
			}},
			{Token::Type::LITERAL_NUMBER, [](Parser& p){
				size_t begin = p.pos->pos;
				static_assert(std::is_same<Integer, long>::value, "Integer must be long long, or parsing may fail.");
				try {
					return p.make_node<Literal<LiteralType::Integer>>(begin, std::stol((p.pos++)->data));
				} catch (...) {
					throw p.make_error("Literal number to Integer parsing failure.");
				}
			}},
			{Token::Type::LITERAL_FALSE, [](Parser& p){
				return p.make_node<Literal<LiteralType::Bool>>((p.pos++)->pos, false);
			}},
			{Token::Type::LITERAL_TRUE, [](Parser& p){
				return p.make_node<Literal<LiteralType::Bool>>((p.pos++)->pos, true);
			}},
			{Token::Type::KEYWORD_NEW, [](Parser& p) -> std::unique_ptr<Expression> {
				size_t begin = (p.pos++)->pos;
				p.expect(Token::Type::NAME);
				std::string name(p.pos->data);
				++p.pos;
				if(p.pos != p.end && p.pos->type == Token::Type::SQUARE_OPEN) {
					p.next_token();
					std::unique_ptr<Expression> size = p.parse_expression();
					p.expect(Token::Type::SQUARE_CLOSE);
					++p.pos;
					return p.make_node<NewArray>(begin, std::move(name), std::move(size));
				} else {
					return p.make_node<NewObject>(begin, std::move(name));
				}
			}},
			{Token::Type::LITERAL_STRING, [](Parser& p){
				size_t begin = p.pos->pos;
				return p.make_node<Literal<LiteralType::String>>(begin, std::string((p.pos++)->data));
			}},
		};

		ParserException make_error(const std::string& str) {
			return ParserException(pos == end ? -1 : pos->pos, str, std::move(env));
		}

		std::unique_ptr<Expression> parse_simple_expression() {
			check_eof();
			size_t begin = pos->pos;
			if(pos->type == Token::Type::OP_NOT) {
				++pos;
				return make_node<UnaryOperator<UnOpType::BoolNegation>>(begin, parse_simple_expression());
			} else if (pos->type == Token::Type::OP_MIN) {
				++pos;
				return make_node<UnaryOperator<UnOpType::IntNegation>>(begin, parse_simple_expression());
			} else if (pos->type == Token::Type::BRACKET_OPEN && pos + 1 != end && (pos + 1)->type == Token::Type::NAME && pos + 2 != end && (pos + 2)->type == Token::Type::BRACKET_CLOSE) {
				++pos;
				std::string type(pos->data);
				pos += 2;
				return make_node<Cast>(begin, parse_simple_expression(), std::move(type));
			}
			auto simple_parser = SIMPLE_PARSERS.find(pos->type);
			if(simple_parser == SIMPLE_PARSERS.end()) {
				throw make_error("Can't match to a known simple expression: " + pos->data);
			}
			std::unique_ptr<Expression> expr = simple_parser->second(*this);
			while(pos != end) {
				if(pos->type == Token::Type::DOT) {
					next_token();
					expect(Token::Type::NAME);
					expr = make_node<ClassMember>(begin, std::move(expr), std::string(pos->data));
				} else if(pos->type == Token::Type::BRACKET_OPEN) {
					std::vector<std::unique_ptr<Expression>> args;
					next_token();
					if(pos->type != Token::Type::BRACKET_CLOSE) {
						args.push_back(parse_expression());
					}
					while(pos != end && pos->type != Token::Type::BRACKET_CLOSE) {
						expect(Token::Type::COMMA);
						next_token();
						args.push_back(parse_expression());
					}
					check_eof();
					expr = make_node<CallOperator>(begin, std::move(expr), std::move(args));
				} else if(pos->type == Token::Type::SQUARE_OPEN) {
					next_token();
					std::unique_ptr<Expression> index = parse_expression();
					expect(Token::Type::SQUARE_CLOSE);
					expr = make_node<SubscriptOperator>(begin, std::move(expr), std::move(index));
				} else {
					break;
				}
				++pos;
			}
			return expr;
		}

		std::unique_ptr<Expression> parse_expression(std::unique_ptr<Expression> e) {
			std::list<std::unique_ptr<Expression>> expressions;
			expressions.push_back(std::move(e));
			std::map<Priority, std::list<std::pair<std::list<std::unique_ptr<Expression>>::iterator, Token::Type>>> operators;
			{
				std::map<Token::Type, Priority>::const_iterator priority;
				while(pos != end && (priority = BIN_OP_PRIORITIES.find(pos->type)) != BIN_OP_PRIORITIES.end()) {
					operators[priority->second].push_back(std::make_pair(--expressions.end(), pos->type));
					++pos;
					expressions.push_back(parse_simple_expression());
				}
			}

			for(auto i = operators.rbegin(); i != operators.rend(); ++i) {
				for(auto j = i->second.begin(); j != i->second.end(); ++j) {
					auto jn = j->first;
					++jn;
					(*jn) = BIN_OP_CONSTRUCTORS.at(j->second)(std::move(*(j->first)), std::move(*jn));
					expressions.erase(j->first);
				}
			}

			return std::unique_ptr<Expression>(std::move(expressions.front()));
		}

		std::unique_ptr<Expression> parse_expression() {
			return parse_expression(parse_simple_expression());
		}

	public:
		Parser() = delete;
		Parser(const Parser&) = delete;
		Parser(Parser&&) = delete;
		Parser(std::vector<Token>& tokens) : tokens(tokens), pos(tokens.begin()), end(tokens.end()) {}

		Program parse_program() {
			std::vector<std::shared_ptr<Class>> classes;
			std::vector<std::shared_ptr<Function>> functions;
			while(pos != tokens.end()) {
				expect(Token::Type::KEYWORD_CLASS, Token::Type::KEYWORD_VOID, Token::Type::NAME);
				if(pos->type == Token::Type::KEYWORD_CLASS) {
					classes.push_back(std::make_shared<Class>(parse_class()));
				} else {
					functions.push_back(std::make_shared<Function>(parse_function()));
				}
			}
			return Program(std::move(classes), std::move(functions));
		}
	};

	const std::map<Token::Type, Parser::Priority> Parser::BIN_OP_PRIORITIES = {{Token::Type::OP_OR, 0}, {Token::Type::OP_AND, 1}, {Token::Type::OP_LT, 2}, {Token::Type::OP_LE, 2}, {Token::Type::OP_GT, 2}, {Token::Type::OP_GE, 2}, {Token::Type::OP_EQ, 2}, {Token::Type::OP_NE, 2}, {Token::Type::OP_ADD, 3}, {Token::Type::OP_MIN, 3}, {Token::Type::OP_MUL, 4}, {Token::Type::OP_DIV, 4}, {Token::Type::OP_MOD, 4}};

	template<BinOpType BOT>
	std::unique_ptr<Expression> construct_binary(std::unique_ptr<Expression>&& left, std::unique_ptr<Expression>&& right){
		return std::make_unique<BinaryOperator<BOT>>(left->begin, right->end, std::move(left), std::move(right));
	}

	const std::map<Token::Type, std::function<std::unique_ptr<Expression>(std::unique_ptr<Expression>&& left, std::unique_ptr<Expression>&& right)>> Parser::BIN_OP_CONSTRUCTORS = {
		{Token::Type::OP_ADD, [](auto left, auto right){
			return construct_binary<BinOpType::Addition>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_MUL, [](auto left, auto right){
			return construct_binary<BinOpType::Multiplication>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_MIN, [](auto left, auto right){
			return construct_binary<BinOpType::Substraction>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_MOD, [](auto left, auto right){
			return construct_binary<BinOpType::Modulo>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_DIV, [](auto left, auto right){
			return construct_binary<BinOpType::Division>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_OR, [](auto left, auto right){
			return construct_binary<BinOpType::Alternative>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_AND, [](auto left, auto right){
			return construct_binary<BinOpType::Conjunction>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_LE, [](auto left, auto right){
			return construct_binary<BinOpType::LessEqual>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_LT, [](auto left, auto right){
			return construct_binary<BinOpType::LessThan>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_GE, [](auto left, auto right){
			return construct_binary<BinOpType::GreaterEqual>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_GT, [](auto left, auto right){
			return construct_binary<BinOpType::GreaterThan>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_EQ, [](auto left, auto right){
			return construct_binary<BinOpType::Equal>(std::move(left), std::move(right));
		}},
		{Token::Type::OP_NE, [](auto left, auto right){
			return construct_binary<BinOpType::NotEqual>(std::move(left), std::move(right));
		}},
	};
}

Program parse(std::vector<Token>& tokens) {
	Parser p(tokens);
	return p.parse_program();
}
