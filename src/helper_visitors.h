#ifndef HELPER_VISITORS_H
#define HELPER_VISITORS_H

#include "program_tree.h"

namespace ProgramTree {
	struct DefaultConstVisitor : public ConstVisitor {
		virtual void default_action() = 0;
		virtual void apply(const BinaryOperator<BinOpType::Addition>& arg);
		virtual void apply(const BinaryOperator<BinOpType::Multiplication>& arg);
		virtual void apply(const BinaryOperator<BinOpType::Division>& arg);
		virtual void apply(const BinaryOperator<BinOpType::Substraction>& arg);
		virtual void apply(const BinaryOperator<BinOpType::Alternative>& arg);
		virtual void apply(const BinaryOperator<BinOpType::Conjunction>& arg);
		virtual void apply(const BinaryOperator<BinOpType::Modulo>& arg);
		virtual void apply(const BinaryOperator<BinOpType::LessThan>& arg);
		virtual void apply(const BinaryOperator<BinOpType::LessEqual>& arg);
		virtual void apply(const BinaryOperator<BinOpType::GreaterThan>& arg);
		virtual void apply(const BinaryOperator<BinOpType::GreaterEqual>& arg);
		virtual void apply(const BinaryOperator<BinOpType::Equal>& arg);
		virtual void apply(const BinaryOperator<BinOpType::NotEqual>& arg);
		virtual void apply(const UnaryOperator<UnOpType::IntNegation>& arg);
		virtual void apply(const UnaryOperator<UnOpType::BoolNegation>& arg);
		virtual void apply(const Literal<LiteralType::Bool>& arg);
		virtual void apply(const Literal<LiteralType::Integer>& arg);
		virtual void apply(const Literal<LiteralType::String>& arg);
		virtual void apply(const Variable& arg);
		virtual void apply(const Null& arg);
		virtual void apply(const CallOperator& arg);
		virtual void apply(const StaticFunctionCall& arg);
		virtual void apply(const VirtualFunctionCall& arg);
		virtual void apply(const SubscriptOperator& arg);
		virtual void apply(const ClassMember& arg);
		virtual void apply(const Cast& arg);
		virtual void apply(const NewObject& arg);
		virtual void apply(const NewArray& arg);
		virtual void apply(const Assignment& arg);
		virtual void apply(const Incrementation& arg);
		virtual void apply(const Decrementation& arg);
		virtual void apply(const ExprStatement& arg);
		virtual void apply(const Return& arg);
		virtual void apply(const If& arg);
		virtual void apply(const While& arg);
		virtual void apply(const For& arg);
		virtual void apply(const Block& arg);
		virtual void apply(const Empty& arg);
		virtual void apply(const Definition& arg);
	};

	struct DefaultVisitor : public Visitor {
		virtual void default_action() = 0;
		virtual void apply(BinaryOperator<BinOpType::Addition>& arg);
		virtual void apply(BinaryOperator<BinOpType::Multiplication>& arg);
		virtual void apply(BinaryOperator<BinOpType::Division>& arg);
		virtual void apply(BinaryOperator<BinOpType::Substraction>& arg);
		virtual void apply(BinaryOperator<BinOpType::Alternative>& arg);
		virtual void apply(BinaryOperator<BinOpType::Conjunction>& arg);
		virtual void apply(BinaryOperator<BinOpType::Modulo>& arg);
		virtual void apply(BinaryOperator<BinOpType::LessThan>& arg);
		virtual void apply(BinaryOperator<BinOpType::LessEqual>& arg);
		virtual void apply(BinaryOperator<BinOpType::GreaterThan>& arg);
		virtual void apply(BinaryOperator<BinOpType::GreaterEqual>& arg);
		virtual void apply(BinaryOperator<BinOpType::Equal>& arg);
		virtual void apply(BinaryOperator<BinOpType::NotEqual>& arg);
		virtual void apply(UnaryOperator<UnOpType::IntNegation>& arg);
		virtual void apply(UnaryOperator<UnOpType::BoolNegation>& arg);
		virtual void apply(Literal<LiteralType::Bool>& arg);
		virtual void apply(Literal<LiteralType::Integer>& arg);
		virtual void apply(Literal<LiteralType::String>& arg);
		virtual void apply(Variable& arg);
		virtual void apply(Null& arg);
		virtual void apply(CallOperator& arg);
		virtual void apply(StaticFunctionCall& arg);
		virtual void apply(VirtualFunctionCall& arg);
		virtual void apply(SubscriptOperator& arg);
		virtual void apply(ClassMember& arg);
		virtual void apply(Cast& arg);
		virtual void apply(NewObject& arg);
		virtual void apply(NewArray& arg);
		virtual void apply(Assignment& arg);
		virtual void apply(Incrementation& arg);
		virtual void apply(Decrementation& arg);
		virtual void apply(ExprStatement& arg);
		virtual void apply(Return& arg);
		virtual void apply(If& arg);
		virtual void apply(While& arg);
		virtual void apply(For& arg);
		virtual void apply(Block& arg);
		virtual void apply(Empty& arg);
		virtual void apply(Definition& arg);
	};

	template<LiteralType LT>
	struct LiteralGetter : public DefaultConstVisitor {
		virtual void default_action() {
		}

		virtual void apply(const Literal<LT> &arg);

		LiteralGetter() = delete;
		LiteralGetter(const typename literal_type<LT>::type*& dest) : dest(dest) {}
	private:
		const typename literal_type<LT>::type*& dest;
	};

	template<>
	void LiteralGetter<LiteralType::Integer>::apply(const Literal<LiteralType::Integer> &arg);

	template<>
	void LiteralGetter<LiteralType::Bool>::apply(const Literal<LiteralType::Bool> &arg);

	template<>
	void LiteralGetter<LiteralType::String>::apply(const Literal<LiteralType::String> &arg);
}

#endif
