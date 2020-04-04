#include "helper_visitors.h"

using namespace ProgramTree;

void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::Addition>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::Multiplication>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::Division>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::Substraction>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::Alternative>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::Conjunction>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::Modulo>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::LessThan>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::LessEqual>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::GreaterThan>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::GreaterEqual>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::Equal>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const BinaryOperator<BinOpType::NotEqual>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const UnaryOperator<UnOpType::IntNegation>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const UnaryOperator<UnOpType::BoolNegation>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Literal<LiteralType::Bool>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Literal<LiteralType::Integer>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Literal<LiteralType::String>& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Variable& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Null& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const CallOperator& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const StaticFunctionCall& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const VirtualFunctionCall& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const SubscriptOperator& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const ClassMember& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Cast& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const NewObject& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const NewArray& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Assignment& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Incrementation& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Decrementation& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const ExprStatement& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Return& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const If& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const While& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const For& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Block& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Empty& arg) {
	(void) arg;
	default_action();
}
void DefaultConstVisitor::apply(const Definition& arg) {
	(void) arg;
	default_action();
}

void DefaultVisitor::apply(BinaryOperator<BinOpType::Addition>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::Multiplication>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::Division>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::Substraction>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::Alternative>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::Conjunction>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::Modulo>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::LessThan>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::LessEqual>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::GreaterThan>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::GreaterEqual>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::Equal>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(BinaryOperator<BinOpType::NotEqual>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(UnaryOperator<UnOpType::IntNegation>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(UnaryOperator<UnOpType::BoolNegation>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Literal<LiteralType::Bool>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Literal<LiteralType::Integer>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Literal<LiteralType::String>& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Variable& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Null& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(CallOperator& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(StaticFunctionCall& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(VirtualFunctionCall& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(SubscriptOperator& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(ClassMember& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Cast& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(NewObject& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(NewArray& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Assignment& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Incrementation& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Decrementation& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(ExprStatement& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Return& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(If& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(While& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(For& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Block& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Empty& arg) {
	(void) arg;
	default_action();
}
void DefaultVisitor::apply(Definition& arg) {
	(void) arg;
	default_action();
}

template<>
void LiteralGetter<LiteralType::Integer>::apply(const Literal<LiteralType::Integer> &arg) {
	dest = &arg.val;
}

template<>
void LiteralGetter<LiteralType::Bool>::apply(const Literal<LiteralType::Bool> &arg) {
	dest = &arg.val;
}

template<>
void LiteralGetter<LiteralType::String>::apply(const Literal<LiteralType::String> &arg) {
	dest = &arg.val;
}
