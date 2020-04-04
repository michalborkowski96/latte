#include "program_tree.h"

#include <stdexcept>
#include <functional>
#include <list>

bool ProgramTree::is_array(const std::string& type) {
	return type.size() >= 2 && type[type.size() - 1] == ']' && type[type.size() - 2] == '[';
}

using namespace ProgramTree;

Expression::Expression(size_t begin, size_t end) : begin(begin), end(end) {}

Statement::Statement(size_t begin, size_t end) : begin(begin), end(end) {}

Null::Null(size_t begin, size_t end) : Expression(begin, end) {}

Empty::Empty(size_t begin, size_t end) : Statement(begin, end) {}

Variable::Variable(size_t begin, size_t end, std::string&& name) : Expression(begin, end), name(std::move(name)) {}

CallOperator::CallOperator(size_t begin, size_t end, std::unique_ptr<Expression>&& fun, std::vector<std::unique_ptr<Expression>>&& args) : Expression(begin, end), fun(std::move(fun)), args(std::move(args)) {}

StaticFunctionCall::StaticFunctionCall(size_t begin, size_t end, std::string&& fun, std::vector<std::unique_ptr<Expression>>&& args) : Expression(begin, end), fun(std::move(fun)), args(std::move(args)) {}

VirtualFunctionCall::VirtualFunctionCall(size_t begin, size_t end, std::unique_ptr<Expression>&& object, std::string&& fun, std::vector<std::unique_ptr<Expression>>&& args) : Expression(begin, end), object(std::move(object)), fun(std::move(fun)), args(std::move(args)) {}

SubscriptOperator::SubscriptOperator(size_t begin, size_t end, std::unique_ptr<Expression>&& arr, std::unique_ptr<Expression>&& index) : Expression(begin, end), arr(std::move(arr)), index(std::move(index)) {}

ClassMember::ClassMember(size_t begin, size_t end, std::unique_ptr<Expression>&& object, std::string&& member) : Expression(begin, end), object(std::move(object)), member(std::move(member)) {}

Cast::Cast(size_t begin, size_t end, std::unique_ptr<Expression>&& expr, std::string&& target) : Expression(begin, end), expr(std::move(expr)), target(std::move(target)) {}

NewObject::NewObject(size_t begin, size_t end, std::string&& new_type) : Expression(begin, end), new_type(std::move(new_type)) {}

NewArray::NewArray(size_t begin, size_t end, std::string&& new_type, std::unique_ptr<Expression>&& size) : Expression(begin, end), new_type(std::move(new_type)), size(std::move(size)) {}

Assignment::Assignment(size_t begin, size_t end, std::unique_ptr<Expression>&& var, std::unique_ptr<Expression>&& value) : Statement(begin, end), var(std::move(var)), value(std::move(value)) {}

Incrementation::Incrementation(size_t begin, size_t end, std::unique_ptr<Expression>&& var) : Statement(begin, end), var(std::move(var)) {}

Decrementation::Decrementation(size_t begin, size_t end, std::unique_ptr<Expression>&& var) : Statement(begin, end), var(std::move(var)) {}

ExprStatement::ExprStatement(size_t begin, size_t end, std::unique_ptr<Expression>&& expr) : Statement(begin, end), expr(std::move(expr)) {}

Return::Return(size_t begin, size_t end, std::unique_ptr<Expression>&& val) : Statement(begin, end), val(std::move(val)) {}

If::If(size_t begin, size_t end, std::unique_ptr<Expression>&& condition, std::unique_ptr<Statement>&& case_then, std::unique_ptr<Statement>&& case_else) : Statement(begin, end), condition(std::move(condition)), case_then(std::move(case_then)), case_else(std::move(case_else)) {}

While::While(size_t begin, size_t end, std::unique_ptr<Expression>&& condition, std::unique_ptr<Statement>&& action) : Statement(begin, end), condition(std::move(condition)), action(std::move(action)) {}

For::For(size_t begin, size_t end, std::string&& var_type, std::string&& var_name, std::unique_ptr<Expression>&& array, std::unique_ptr<Statement>&& action) : Statement(begin, end), var_type(std::move(var_type)), var_name(std::move(var_name)), array(std::move(array)), action(std::move(action)) {}

Block::Block(size_t begin, size_t end, std::vector<std::unique_ptr<Statement>>&& statements) : Statement(begin, end), statements(std::move(statements)) {}

Definition::Definition(size_t begin, size_t end, std::string&& type, std::vector<std::pair<std::string, std::unique_ptr<Expression>>>&& defs) : Statement(begin, end), type(std::move(type)), defs(std::move(defs)) {}

Function::Function(size_t dec_begin, size_t dec_end, std::string&& name, std::string&& return_type, std::vector<std::pair<std::string, std::string>>&& arguments, std::unique_ptr<Block>&& body) : dec_begin(dec_begin), dec_end(dec_end), name(std::move(name)),  return_type(std::move(return_type)), arguments(std::move(arguments)), body(std::move(body)) {}

Class::Class(size_t dec_begin, size_t dec_end, std::string&& name, std::string&& superclass, std::vector<std::shared_ptr<Function>>&& functions, std::vector<std::unique_ptr<Definition>>&& variables) : dec_begin(dec_begin), dec_end(dec_end), name(std::move(name)), superclass(std::move(superclass)), functions(std::move(functions)), variables(std::move(variables)) {}

Program::Program(std::vector<std::shared_ptr<Class>>&& classes, std::vector<std::shared_ptr<Function>>&& functions) : classes(std::move(classes)), functions(std::move(functions)) {}

void Variable::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void Null::visit(Visitor* visitor) {
	visitor->apply(*this);
}
void Assignment::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void Incrementation::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void Decrementation::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void Return::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void ExprStatement::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void CallOperator::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void StaticFunctionCall::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void VirtualFunctionCall::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void SubscriptOperator::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void ClassMember::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void Cast::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void NewObject::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void NewArray::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void If::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void While::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void For::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void Block::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void Empty::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void Definition::visit(Visitor* visitor) {
	visitor->apply(*this);
}

void Variable::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void Null::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void Assignment::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}
void Incrementation::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void Decrementation::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void ExprStatement::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void Return::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void CallOperator::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void StaticFunctionCall::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void VirtualFunctionCall::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void SubscriptOperator::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void ClassMember::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void Cast::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void NewObject::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void NewArray::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void If::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void While::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void For::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void Block::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void Empty::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}

void Definition::visit(ConstVisitor* visitor) const {
	visitor->apply(*this);
}
