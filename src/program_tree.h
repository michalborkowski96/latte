#ifndef PRORGAM_TREE_H
#define PRORGAM_TREE_H

#include "lexer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <stdexcept>
#include <map>
#include <deque>
#include <type_traits>
#include <set>

namespace ProgramTree {
	using Integer = int64_t;
	static_assert(sizeof(Integer) == sizeof(size_t), "ProgramTree::Integer must have the size of a pointer.");

	enum class BinOpType {
		Addition, Division, Multiplication, Substraction, Alternative, Conjunction, Modulo, LessThan, LessEqual, GreaterThan, GreaterEqual, Equal, NotEqual
	};

	enum class UnOpType {
		IntNegation, BoolNegation
	};

	enum class LiteralType {
		Integer, Bool, String
	};

	template<BinOpType T>
	struct BinaryOperator;

	template<UnOpType T>
	struct UnaryOperator;

	template<LiteralType LT>
	struct Literal;

	struct Variable;
	struct Null;
	struct CallOperator;
	struct StaticFunctionCall;
	struct VirtualFunctionCall;
	struct SubscriptOperator;
	struct ClassMember;
	struct Cast;
	struct NewObject;
	struct NewArray;
	struct Assignment;
	struct Incrementation;
	struct Decrementation;
	struct ExprStatement;
	struct Return;
	struct If;
	struct While;
	struct For;
	struct Block;
	struct Empty;
	struct Definition;

	class Visitor {
	public:
		virtual void apply(BinaryOperator<BinOpType::Addition>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::Multiplication>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::Division>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::Substraction>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::Alternative>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::Conjunction>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::Modulo>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::LessThan>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::LessEqual>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::GreaterThan>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::GreaterEqual>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::Equal>& arg) = 0;
		virtual void apply(BinaryOperator<BinOpType::NotEqual>& arg) = 0;
		virtual void apply(UnaryOperator<UnOpType::IntNegation>& arg) = 0;
		virtual void apply(UnaryOperator<UnOpType::BoolNegation>& arg) = 0;
		virtual void apply(Literal<LiteralType::Bool>& arg) = 0;
		virtual void apply(Literal<LiteralType::Integer>& arg) = 0;
		virtual void apply(Literal<LiteralType::String>& arg) = 0;
		virtual void apply(Variable& arg) = 0;
		virtual void apply(Null& arg) = 0;
		virtual void apply(CallOperator& arg) = 0;
		virtual void apply(StaticFunctionCall& arg) = 0;
		virtual void apply(VirtualFunctionCall& arg) = 0;
		virtual void apply(SubscriptOperator& arg) = 0;
		virtual void apply(ClassMember& arg) = 0;
		virtual void apply(Cast& arg) = 0;
		virtual void apply(NewObject& arg) = 0;
		virtual void apply(NewArray& arg) = 0;
		virtual void apply(Assignment& arg) = 0;
		virtual void apply(Incrementation& arg) = 0;
		virtual void apply(Decrementation& arg) = 0;
		virtual void apply(ExprStatement& arg) = 0;
		virtual void apply(Return& arg) = 0;
		virtual void apply(If& arg) = 0;
		virtual void apply(While& arg) = 0;
		virtual void apply(For& arg) = 0;
		virtual void apply(Block& arg) = 0;
		virtual void apply(Empty& arg) = 0;
		virtual void apply(Definition& arg) = 0;
		virtual ~Visitor() = default;
	};

	class ConstVisitor {
	public:
		virtual void apply(const BinaryOperator<BinOpType::Addition>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::Multiplication>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::Division>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::Substraction>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::Alternative>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::Conjunction>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::Modulo>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::LessThan>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::LessEqual>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::GreaterThan>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::GreaterEqual>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::Equal>& arg) = 0;
		virtual void apply(const BinaryOperator<BinOpType::NotEqual>& arg) = 0;
		virtual void apply(const UnaryOperator<UnOpType::IntNegation>& arg) = 0;
		virtual void apply(const UnaryOperator<UnOpType::BoolNegation>& arg) = 0;
		virtual void apply(const Literal<LiteralType::Bool>& arg) = 0;
		virtual void apply(const Literal<LiteralType::Integer>& arg) = 0;
		virtual void apply(const Literal<LiteralType::String>& arg) = 0;
		virtual void apply(const Variable& arg) = 0;
		virtual void apply(const Null& arg) = 0;
		virtual void apply(const CallOperator& arg) = 0;
		virtual void apply(const StaticFunctionCall& arg) = 0;
		virtual void apply(const VirtualFunctionCall& arg) = 0;
		virtual void apply(const SubscriptOperator& arg) = 0;
		virtual void apply(const ClassMember& arg) = 0;
		virtual void apply(const Cast& arg) = 0;
		virtual void apply(const NewObject& arg) = 0;
		virtual void apply(const NewArray& arg) = 0;
		virtual void apply(const Assignment& arg) = 0;
		virtual void apply(const Incrementation& arg) = 0;
		virtual void apply(const Decrementation& arg) = 0;
		virtual void apply(const ExprStatement& arg) = 0;
		virtual void apply(const Return& arg) = 0;
		virtual void apply(const If& arg) = 0;
		virtual void apply(const While& arg) = 0;
		virtual void apply(const For& arg) = 0;
		virtual void apply(const Block& arg) = 0;
		virtual void apply(const Empty& arg) = 0;
		virtual void apply(const Definition& arg) = 0;
		virtual ~ConstVisitor() = default;
	};

	template <LiteralType T>
	struct literal_type {
		using type = void;
		literal_type() = delete;
	};

	template <>
	struct literal_type<LiteralType::Integer> {
		using type = Integer;
		literal_type() = delete;
	};

	template <>
	struct literal_type<LiteralType::Bool> {
		using type = bool;
		literal_type() = delete;
	};

	template <>
	struct literal_type<LiteralType::String> {
		using type = std::string;
		literal_type() = delete;
	};

	struct Expression {
		const size_t begin;
		const size_t end;
		std::string type; //to be set by the type checker
		virtual void visit(ConstVisitor* visitor) const = 0;
		virtual void visit(Visitor* visitor) = 0;

		virtual ~Expression() = default;
		Expression(Expression&&) = delete;
		Expression(const Expression&) = delete;
		Expression() = delete;

	protected:
		Expression(size_t begin, size_t end);
	};

	template<BinOpType T>
	struct BinaryOperator : public Expression {
		std::unique_ptr<Expression> left;
		std::unique_ptr<Expression> right;
		BinaryOperator() = delete;
		~BinaryOperator() = default;
		BinaryOperator(size_t begin, size_t end, std::unique_ptr<Expression>&& left, std::unique_ptr<Expression>&& right) : Expression(begin, end), left(std::move(left)), right(std::move(right)) {}
		virtual void visit(ConstVisitor* visitor) const override {
			visitor->apply(*this);
		}
		virtual void visit(Visitor* visitor) override {
			visitor->apply(*this);
		}
		BinaryOperator(const BinaryOperator&) = delete;
	};

	template<UnOpType T>
	struct UnaryOperator : public Expression {
		std::unique_ptr<Expression> expr;
		UnaryOperator() = delete;
		~UnaryOperator() = default;
		UnaryOperator(size_t begin, size_t end, std::unique_ptr<Expression>&& expr) : Expression(begin, end), expr(std::move(expr)) {}
		virtual void visit(ConstVisitor* visitor) const override {
			visitor->apply(*this);
		}
		virtual void visit(Visitor* visitor) override {
			visitor->apply(*this);
		}
		UnaryOperator(const UnaryOperator&) = delete;
	};

	template<LiteralType LT>
	struct Literal : public Expression {
		typename literal_type<LT>::type val;
		Literal() = delete;
		~Literal() = default;
		Literal(size_t begin, size_t end, typename literal_type<LT>::type&& val) : Expression(begin, end), val(std::move(val)) {}
		Literal(const Literal&) = delete;
		virtual void visit(ConstVisitor* visitor) const override {
			visitor->apply(*this);
		}
		virtual void visit(Visitor* visitor) override {
			visitor->apply(*this);
		}
	};

	struct CallOperator : public Expression {
		std::unique_ptr<Expression> fun;
		std::vector<std::unique_ptr<Expression>> args;
		CallOperator() = delete;
		~CallOperator() = default;
		CallOperator(size_t begin, size_t end, std::unique_ptr<Expression>&& fun, std::vector<std::unique_ptr<Expression>>&& args);
		CallOperator(const CallOperator&) = delete;
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
	};

	struct StaticFunctionCall : public Expression {
		std::string fun;
		std::vector<std::unique_ptr<Expression>> args;
		StaticFunctionCall() = delete;
		~StaticFunctionCall() = default;
		StaticFunctionCall(size_t begin, size_t end, std::string&& fun, std::vector<std::unique_ptr<Expression>>&& args);
		StaticFunctionCall(const StaticFunctionCall&) = delete;
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
	};

	struct VirtualFunctionCall : public Expression {
		std::unique_ptr<Expression> object;
		std::string fun;
		std::vector<std::unique_ptr<Expression>> args;
		VirtualFunctionCall() = delete;
		~VirtualFunctionCall() = default;
		VirtualFunctionCall(size_t begin, size_t end, std::unique_ptr<Expression>&& object, std::string&& fun, std::vector<std::unique_ptr<Expression>>&& args);
		VirtualFunctionCall(const VirtualFunctionCall&) = delete;
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
	};

	struct SubscriptOperator : public Expression {
		std::unique_ptr<Expression> arr;
		std::unique_ptr<Expression> index;
		SubscriptOperator() = delete;
		~SubscriptOperator() = default;
		SubscriptOperator(size_t begin, size_t end, std::unique_ptr<Expression>&& arr, std::unique_ptr<Expression>&& index);
		SubscriptOperator(const SubscriptOperator&) = delete;
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
	};

	struct NewObject : public Expression {
		std::string new_type;
		NewObject() = delete;
		~NewObject() = default;
		NewObject(size_t begin, size_t end, std::string&& type);
		NewObject(const NewObject&) = delete;
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
	};

	struct NewArray : public Expression {
		std::string new_type;
		std::unique_ptr<Expression> size;
		NewArray() = delete;
		~NewArray() = default;
		NewArray(size_t begin, size_t end, std::string&& type, std::unique_ptr<Expression>&& size);
		NewArray(const NewArray&) = delete;
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
	};

	struct ClassMember : public Expression {
		std::unique_ptr<Expression> object;
		std::string member;

		ClassMember() = delete;
		ClassMember(const ClassMember&) = delete;
		ClassMember(ClassMember&&) = default;
		~ClassMember() = default;
		ClassMember(size_t begin, size_t end, std::unique_ptr<Expression>&& object, std::string&& member);

		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
	};

	struct Variable : public Expression {
		std::string name;

		Variable() = delete;
		Variable(const Variable&) = delete;
		Variable(Variable&&) = default;
		~Variable() = default;
		Variable(size_t begin, size_t end, std::string&& name);

		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
	};

	struct Null : public Expression {
		Null(size_t begin, size_t end);
		Null(const Null&) = delete;
		Null(Null&&) = default;
		~Null() = default;
		Null() = delete;

		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
	};

	struct Cast : public Expression {
		std::unique_ptr<Expression> expr;
		std::string target;

		Cast() = delete;
		Cast(const Cast&) = delete;
		Cast(Cast&&) = default;
		~Cast() = default;
		Cast(size_t begin, size_t end, std::unique_ptr<Expression>&& expr, std::string&& target);

		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
	};

	struct Statement {
		const size_t begin;
		const size_t end;
		virtual void visit(ConstVisitor* visitor) const = 0;
		virtual void visit(Visitor* visitor) = 0;

		virtual ~Statement() = default;
		Statement(Statement&&) = delete;
		Statement(const Statement&) = delete;

	protected:
		Statement() = delete;
		Statement(size_t begin, size_t end);
	};

	struct Assignment : public Statement {
		std::unique_ptr<Expression> var;
		std::unique_ptr<Expression> value;

		Assignment() = delete;
		~Assignment() = default;
		Assignment(size_t begin, size_t end, std::unique_ptr<Expression>&& var, std::unique_ptr<Expression>&& value);

		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;

		Assignment(const Assignment&) = delete;
	};

	struct ExprStatement : public Statement {
		std::unique_ptr<Expression> expr;

		ExprStatement() = delete;
		~ExprStatement() = default;
		ExprStatement(size_t begin, size_t end, std::unique_ptr<Expression>&& expr);

		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;

		ExprStatement(const ExprStatement&) = delete;
	};

	struct Incrementation : public Statement {
		std::unique_ptr<Expression> var;
		Incrementation() = delete;
		~Incrementation() = default;
		Incrementation(size_t begin, size_t end, std::unique_ptr<Expression>&& var);
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
		Incrementation(const Incrementation&) = delete;
	};

	struct Decrementation : public Statement {
		std::unique_ptr<Expression> var;
		Decrementation() = delete;
		~Decrementation() = default;
		Decrementation(size_t begin, size_t end, std::unique_ptr<Expression>&& var);
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
		Decrementation(const Decrementation&) = delete;
	};

	struct Return : public Statement {
		std::unique_ptr<Expression> val; //may be null
		Return(size_t begin, size_t end, std::unique_ptr<Expression>&& val);
		~Return() = default;
		Return() = delete;
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
		Return(const Return&) = delete;
	};

	struct If : public Statement {
		std::unique_ptr<Expression> condition;
		std::unique_ptr<Statement> case_then;
		std::unique_ptr<Statement> case_else; //may be empty
		If(size_t begin, size_t end, std::unique_ptr<Expression>&& condition, std::unique_ptr<Statement>&& case_then, std::unique_ptr<Statement>&& case_else);
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
		If(const If&) = delete;
		~If() = default;
		If() = delete;
	};

	struct While : public Statement {
		std::unique_ptr<Expression> condition;
		std::unique_ptr<Statement> action;
		While(size_t begin, size_t end, std::unique_ptr<Expression>&& condition, std::unique_ptr<Statement>&& action);
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
		While(const While&) = delete;
		~While() = default;
		While() = delete;
	};

	struct For : public Statement {
		std::string var_type;
		std::string var_name;
		std::unique_ptr<Expression> array;
		std::unique_ptr<Statement> action;
		For(size_t begin, size_t end, std::string&& var_type, std::string&& var_name, std::unique_ptr<Expression>&& array, std::unique_ptr<Statement>&& action);
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
		For(const For&) = delete;
		~For() = default;
		For() = delete;
	};

	struct Block : public Statement {
		std::vector<std::unique_ptr<Statement>> statements;
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
		Block(Block&&) = delete;
		Block(const Block&) = delete;
		Block() = delete;
		~Block() = default;
		Block(size_t begin, size_t end, std::vector<std::unique_ptr<Statement>>&& statements);
	};

	struct Empty : public Statement {
		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
		Empty(const Empty&) = delete;
		Empty() = delete;
		~Empty() = default;
		Empty(size_t begin, size_t end);
	};

	struct Definition : public Statement {
		std::string type;
		std::vector<std::pair<std::string, std::unique_ptr<Expression>>> defs; //may be null
		Definition() = delete;
		Definition(const Definition&) = delete;
		Definition(Definition&&) = default;
		~Definition() = default;
		Definition(size_t begin, size_t end, std::string&& type, std::vector<std::pair<std::string, std::unique_ptr<Expression>>>&& defs);

		virtual void visit(ConstVisitor* visitor) const override;
		virtual void visit(Visitor* visitor) override;
	};

	struct Function {
		size_t dec_begin;
		size_t dec_end;
		std::string name;
		std::string return_type;
		std::vector<std::pair<std::string, std::string>> arguments;
		std::unique_ptr<Block> body;
		Function() = delete;
		Function(const Function&) = delete;
		Function(Function&&) = default;
		Function(size_t dec_begin, size_t dec_end, std::string&& name, std::string&& return_type, std::vector<std::pair<std::string, std::string>>&& arguments, std::unique_ptr<Block>&& body);
	};

	struct Class {
		size_t dec_begin;
		size_t dec_end;
		std::string name;
		std::string superclass;
		std::vector<std::shared_ptr<Function>> functions;
		std::vector<std::unique_ptr<Definition>> variables;
		Class() = delete;
		Class(const Class&) = delete;
		Class(Class&&) = default;
		Class(size_t dec_begin, size_t dec_end, std::string&& name, std::string&& superclass, std::vector<std::shared_ptr<Function>>&& functions, std::vector<std::unique_ptr<Definition>>&& variables);
	};

	struct Program {
		std::vector<std::shared_ptr<Class>> classes;
		std::vector<std::shared_ptr<Function>> functions;
		Program() = delete;
		Program(const Program&) = delete;
		Program(Program&&) = default;
		Program(std::vector<std::shared_ptr<Class>>&& classes, std::vector<std::shared_ptr<Function>>&& functions);
	};

	const std::string VOID_NAME = "void";
	const std::string INT_NAME = "int";
	const std::string STR_NAME = "string";
	const std::string BOOL_NAME = "boolean";
	const std::set<std::string> DEFAULT_TYPES = {VOID_NAME, INT_NAME, STR_NAME, BOOL_NAME};
	const std::map<std::string, std::pair<std::string, std::vector<std::string>>> BUILTIN_FUNCTIONS = {{"printInt", {"void", {"int"}}}, {"printString", {"void", {"string"}}}, {"error", {"void", {}}}, {"readInt", {"int", {}}}, {"readString", {"string", {}}}};
	const std::string CONCAT_FUN_NAME = "_concat";
	const std::string THIS_NAME = "self";
	const std::string LENGTH_ATTR_NAME = "length";

	bool is_array(const std::string& type);
}
#endif
