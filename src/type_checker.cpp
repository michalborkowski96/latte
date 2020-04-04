#include "type_checker.h"
#include "helper_visitors.h"

#include <map>
#include <string>
#include <set>
#include <stack>

using namespace ProgramTree;
using namespace TypeChecker;

namespace {
	bool does_cast_implicitly(const TypeInfo& info, const std::string& from, const std::string& to) {
		//implicit cast is available up the inheritance tree or from null to a classname
		if(from == to) {
			return true;
		}
		if(is_array(from) && is_array(to)) {
			return does_cast_implicitly(info, from.substr(0, from.size() - 2), to.substr(0, to.size() - 2));
		}
		if(from == Token::TYPE_NAMES.at(Token::Type::KEYWORD_NULL) && info.classes.find(to) != info.classes.end()) {
			return true;
		}
		auto r = info.classes.find(from);
		if(r == info.classes.end()) {
			return false;
		}
		InheritanceTreeNode* node = r->second->inheritance_tree_node;
		while(node) {
			if(node->class_info->data->name == to) {
				return true;
			}
			node = node->parent;
		}
		return false;
	}

	bool does_explicit_cast(const TypeInfo& info, const std::string& from, const std::string& to) {
		//explicit cast is available in both directions along the inheritance tree
		return does_cast_implicitly(info, from, to) || does_cast_implicitly(info, to, from);
	}

	template<typename T, typename U>
	std::string build_function_arg_string(const std::vector<T>& args, U getter) {
		std::string expr_type = "(";
		if(!args.empty()) {
			expr_type += getter(args[0]);
		}
		for(size_t i = 1; i < args.size(); ++i) {
			expr_type += ',';
			expr_type += getter(args[i]);
		}
		expr_type += ')';
		return expr_type;
	}

	std::string build_function_arg_string(const std::vector<std::pair<std::string, std::string>>& args)  {
		return build_function_arg_string(args, [](const std::pair<std::string, std::string>& a){
			return a.first;
		});
	}

		std::string build_function_arg_string(const std::vector<std::string>& args)  {
		return build_function_arg_string(args, [](const std::string& a){
			return a;
		});
	}

	std::vector<std::string> get_args_from_function_string(const std::string& fun)  {
		std::vector<std::string> ret;
		size_t bracket = 1;
		size_t begin = fun.find('(') + 1;
		for(size_t i = begin + 1; i < fun.size(); ++i) {
			if(fun[i] == '(') {
				++bracket;
			} else if(fun[i] == ')') {
				if(!--bracket) {
					ret.push_back(fun.substr(begin, i - begin));
					break;
				}
			} else if(fun[i] == ',') {
				if(bracket == 1) {
					ret.push_back(fun.substr(begin, i - begin));
					begin = i + 1;
				}
			}
		}
		return ret;
	}

	template<typename T>
	std::string build_function_type_string(const std::string& return_type, const std::vector<T> &args) {
		return std::string("@function<") + return_type + build_function_arg_string(args) + '>';
	}

	struct FunctionSourceSetter : public DefaultVisitor {
		std::unique_ptr<Expression>& target;
		std::vector<std::unique_ptr<Expression>>& args;
		FunctionSourceSetter() = delete;
		FunctionSourceSetter(std::unique_ptr<Expression>& target, std::vector<std::unique_ptr<Expression>>& args) : target(target), args(args) {}

		virtual void default_action() {
			throw std::runtime_error("internal type checker error");
		}

		virtual void apply(Variable& arg) {
			target = std::make_unique<StaticFunctionCall>(arg.begin, arg.end, std::move(arg.name), std::move(args));
		}

		virtual void apply(ClassMember& arg) {
			target = std::make_unique<VirtualFunctionCall>(arg.begin, arg.end, std::move(arg.object), std::move(arg.member), std::move(args));
		}
	};

	struct IsDefinition : public DefaultConstVisitor {
		virtual void default_action() {
			result = false;
		}

		virtual void apply(const Definition &arg) {
			(void) arg;
			result = true;
		}

		IsDefinition() = delete;
		IsDefinition(bool& result) : result(result) {}
	private:
		bool& result;
	};

	struct IsNonReturningCall : public DefaultConstVisitor {
		virtual void default_action() {
			result = false;
		}

		virtual void apply(const StaticFunctionCall &arg) {
			result = arg.fun == "error";
		}

		IsNonReturningCall() = delete;
		IsNonReturningCall(bool& result) : result(result) {}
	private:
		bool& result;
	};

	struct TypeCheckerVisitor : public ProgramTree::Visitor {
		//statement is expected to set 'does_return' and 'optimized_statement'
		//expression is expected to set 'arg.type', 'optimized_expression', 'side_effects', 'variable_access'
		template<ProgramTree::LiteralType OT, ProgramTree::LiteralType RT, ProgramTree::BinOpType BOT, typename A>
		void apply_binary(ProgramTree::BinaryOperator<BOT>& arg, const std::string& operand_type, const std::string& result_type, A operation, const std::string& operation_name) {
			arg.left->visit(this);
			if(optimized_expression) {
				std::swap(arg.left, optimized_expression);
			}
			bool left_side_effects = side_effects;
			arg.right->visit(this);
			if(optimized_expression) {
				std::swap(arg.right, optimized_expression);
			}
			bool valid_type = true;
			if(arg.right->type.empty()) {
				valid_type = false;
			} else if(!does_cast_implicitly(info, arg.right->type, operand_type)) {
				push_error(arg.begin, arg.end, "type " + arg.right->type + " cannot be casted to " + operand_type + " in the right operand of " + operation_name + " expression.");
				valid_type = false;
			}
			if(arg.left->type.empty()) {
				valid_type = false;
			} else if(!does_cast_implicitly(info, arg.left->type, operand_type)) {
				push_error(arg.begin, arg.end, "type " + arg.left->type + " cannot be casted to " + operand_type + " in the left operand of " + operation_name + " expression.");
				valid_type = false;
			}
			if(valid_type) {
				arg.type = result_type;
			} else {
				arg.type = "";
			}
			const typename ProgramTree::literal_type<OT>::type* l_val = nullptr;
			ProgramTree::LiteralGetter<OT> lg(l_val);
			arg.left->visit(&lg);
			const typename ProgramTree::literal_type<OT>::type* r_val = nullptr;
			ProgramTree::LiteralGetter<OT> rg(r_val);
			arg.right->visit(&rg);
			if(l_val && r_val) {
				optimized_expression = std::make_unique<ProgramTree::Literal<RT>>(arg.begin, arg.end, operation(*l_val, *r_val));
				optimized_expression->type = arg.type;
			} else {
				optimized_expression = nullptr;
			}
			side_effects = side_effects | left_side_effects;
			variable_access = false;
		}

		template<ProgramTree::LiteralType LT>
		void optimize_addition(std::unique_ptr<ProgramTree::Expression>& l, std::unique_ptr<ProgramTree::Expression>& r, const std::string& result_type) {
			const typename ProgramTree::literal_type<LT>::type* l_val = nullptr;
			ProgramTree::LiteralGetter<LT> lg(l_val);
			l->visit(&lg);
			const typename ProgramTree::literal_type<LT>::type* r_val = nullptr;
			ProgramTree::LiteralGetter<LT> rg(r_val);
			r->visit(&rg);
			if(l_val && r_val) {
				optimized_expression = std::make_unique<ProgramTree::Literal<LT>>(l->begin, r->end, *l_val + *r_val);
				optimized_expression->type = result_type;
			} else {
				optimized_expression = nullptr;
			}
		}

		virtual void apply(BinaryOperator<BinOpType::Addition>& arg) {
			//IMPORTANT change string addition to a function call
			arg.left->visit(this);
			if(optimized_expression) {
				std::swap(arg.left, optimized_expression);
			}
			bool left_side_effects = side_effects;
			arg.right->visit(this);
			if(optimized_expression) {
				std::swap(arg.right, optimized_expression);
			}

			if(arg.left->type.empty() || arg.right->type.empty()) {
				arg.type = "";
			} else if(does_cast_implicitly(info, arg.right->type, STR_NAME) && does_cast_implicitly(info, arg.left->type, STR_NAME)) {
				optimize_addition<LiteralType::String>(arg.left, arg.right, STR_NAME);
				std::vector<std::unique_ptr<Expression>> args;
				args.push_back(std::move(arg.left));
				args.push_back(std::move(arg.right));
				optimized_expression = std::make_unique<StaticFunctionCall>(arg.begin, arg.end, CONCAT_FUN_NAME.substr(), std::move(args));
				arg.type = STR_NAME;
				optimized_expression->type = STR_NAME;
			} else if(does_cast_implicitly(info, arg.left->type, INT_NAME) && does_cast_implicitly(info, arg.right->type, INT_NAME)) {
				optimize_addition<LiteralType::Integer>(arg.left, arg.right, INT_NAME);
				arg.type = INT_NAME;
			} else {
				push_error(arg.begin, arg.end, "the plus operator got arguments of " + arg.left->type + " and " + arg.right->type + " instead of string + string or int + int");
				arg.type = "";
			}
			side_effects = side_effects | left_side_effects;
			variable_access = false;
		}

		virtual void apply(BinaryOperator<BinOpType::Multiplication>& arg) {
			apply_binary<LiteralType::Integer, LiteralType::Integer>(arg, INT_NAME, INT_NAME, [](literal_type<LiteralType::Integer>::type a, literal_type<LiteralType::Integer>::type b) -> literal_type<LiteralType::Integer>::type {return a * b;}, "a multiplication");
		}

		virtual void apply(BinaryOperator<BinOpType::Division>& arg) {
			apply_binary<LiteralType::Integer, LiteralType::Integer>(arg, INT_NAME, INT_NAME, [this, &arg](literal_type<LiteralType::Integer>::type a, literal_type<LiteralType::Integer>::type b) -> literal_type<LiteralType::Integer>::type {
				if(b == 0) {
					push_error(arg.begin, arg.end, "division by zero");
					return 0;
				}
				return a / b;
			}, "a division");
		}

		virtual void apply(BinaryOperator<BinOpType::Substraction>& arg) {
			apply_binary<LiteralType::Integer, LiteralType::Integer>(arg, INT_NAME, INT_NAME, [](literal_type<LiteralType::Integer>::type a, literal_type<LiteralType::Integer>::type b) -> literal_type<LiteralType::Integer>::type {return a - b;}, "a substraction");
		}

		virtual void apply(BinaryOperator<BinOpType::Alternative>& arg) {
			apply_binary<LiteralType::Bool, LiteralType::Bool>(arg, BOOL_NAME, BOOL_NAME, [](literal_type<LiteralType::Bool>::type a, literal_type<LiteralType::Bool>::type b) -> literal_type<LiteralType::Bool>::type {return a || b;}, "an alternative");
		}

		virtual void apply(BinaryOperator<BinOpType::Conjunction>& arg) {
			apply_binary<LiteralType::Bool, LiteralType::Bool>(arg, BOOL_NAME, BOOL_NAME, [](literal_type<LiteralType::Bool>::type a, literal_type<LiteralType::Bool>::type b) -> literal_type<LiteralType::Bool>::type {return a && b;}, "a conjunction");
		}

		virtual void apply(BinaryOperator<BinOpType::Modulo>& arg) {
			apply_binary<LiteralType::Integer, LiteralType::Integer>(arg, INT_NAME, INT_NAME, [this, &arg](literal_type<LiteralType::Integer>::type a, literal_type<LiteralType::Integer>::type b) -> literal_type<LiteralType::Integer>::type {
				if(b == 0) {
					push_error(arg.begin, arg.end, "modulo by zero");
					return 0;
				}
				return a % b;
			}, "a modulo");
		}

		virtual void apply(BinaryOperator<BinOpType::LessThan>& arg) {
			apply_binary<LiteralType::Integer, LiteralType::Bool>(arg, INT_NAME, BOOL_NAME, [](literal_type<LiteralType::Integer>::type a, literal_type<LiteralType::Integer>::type b) -> literal_type<LiteralType::Integer>::type {return a < b;}, "a lt comparison");
		}

		virtual void apply(BinaryOperator<BinOpType::LessEqual>& arg) {
			apply_binary<LiteralType::Integer, LiteralType::Bool>(arg, INT_NAME, BOOL_NAME, [](literal_type<LiteralType::Integer>::type a, literal_type<LiteralType::Integer>::type b) -> literal_type<LiteralType::Integer>::type {return a <= b;}, "a le comparison");
		}

		virtual void apply(BinaryOperator<BinOpType::GreaterThan>& arg) {
			apply_binary<LiteralType::Integer, LiteralType::Bool>(arg, INT_NAME, BOOL_NAME, [](literal_type<LiteralType::Integer>::type a, literal_type<LiteralType::Integer>::type b) -> literal_type<LiteralType::Integer>::type {return a > b;}, "a gt comparison");
		}

		virtual void apply(BinaryOperator<BinOpType::GreaterEqual>& arg) {
			apply_binary<LiteralType::Integer, LiteralType::Bool>(arg, INT_NAME, BOOL_NAME, [](literal_type<LiteralType::Integer>::type a, literal_type<LiteralType::Integer>::type b) -> literal_type<LiteralType::Integer>::type {return a >= b;}, "a ge comparison");
		}

		template<ProgramTree::LiteralType LT>
		void try_optimize_equality(std::unique_ptr<ProgramTree::Expression>& l, std::unique_ptr<ProgramTree::Expression>& r, bool negation) {
			const typename ProgramTree::literal_type<LT>::type* l_val = nullptr;
			ProgramTree::LiteralGetter<LT> lg(l_val);
			l->visit(&lg);
			const typename ProgramTree::literal_type<LT>::type* r_val = nullptr;
			ProgramTree::LiteralGetter<LT> rg(r_val);
			r->visit(&rg);
			if(l_val && r_val) {
				ProgramTree::literal_type<ProgramTree::LiteralType::Bool>::type result = (*l_val == *r_val);
				result = negation ? !result : result;
				optimized_expression = std::make_unique<ProgramTree::Literal<ProgramTree::LiteralType::Bool>>(l->begin, r->end, std::move(result));
				optimized_expression->type = ProgramTree::BOOL_NAME;
			}
		}

		std::string apply_equality(std::unique_ptr<ProgramTree::Expression>& l, std::unique_ptr<ProgramTree::Expression>& r, bool negation) {
			std::string result;
			l->visit(this);
			if(optimized_expression) {
				std::swap(l, optimized_expression);
			}
			bool left_side_effects = side_effects;
			r->visit(this);
			if(optimized_expression) {
				std::swap(r, optimized_expression);
			}
			if(l->type.empty() || r->type.empty()) {
				result = "";
			} else if (does_cast_implicitly(info, l->type, r->type) || does_cast_implicitly(info, r->type, l->type)) {
				result = BOOL_NAME;
			} else {
				push_error(l->begin, r->end, "type " + l->type + " and " + r->type + " cannot be compared.");
				result = "";
			}
			optimized_expression = nullptr;
			try_optimize_equality<LiteralType::Bool>(l, r, negation);
			try_optimize_equality<LiteralType::Integer>(l, r, negation);
			try_optimize_equality<LiteralType::String>(l, r, negation);
			side_effects = side_effects | left_side_effects;
			variable_access = false;
			return result;
		}

		template<ProgramTree::LiteralType LT, ProgramTree::UnOpType UOT, typename A>
		void apply_unary(ProgramTree::UnaryOperator<UOT>& arg, A operation, const std::string& type_name, const std::string& operation_name) {
			arg.expr->visit(this);
			if(optimized_expression) {
				std::swap(arg.expr, optimized_expression);
			}
			if(!arg.expr->type.empty() && does_cast_implicitly(info, arg.expr->type, type_name)) {
				arg.type = type_name;
			} else {
				if(!arg.expr->type.empty()) {
					push_error(arg.begin, arg.end, "type " + arg.expr->type + " cannot be casted to " + type_name + " in " + operation_name + " expression.");
				}
				arg.type = "";
			}
			const typename ProgramTree::literal_type<LT>::type* val = nullptr;
			ProgramTree::LiteralGetter<LT> lg(val);
			arg.expr->visit(&lg);
			if(val) {
				optimized_expression = std::make_unique<ProgramTree::Literal<LT>>(arg.begin, arg.end, operation(*val));
				optimized_expression->type = arg.type;
			} else {
				optimized_expression = nullptr;
			}
			variable_access = false;
		}

		virtual void apply(BinaryOperator<BinOpType::Equal>& arg) {
			arg.type = apply_equality(arg.left, arg.right, false);
		}

		virtual void apply(BinaryOperator<BinOpType::NotEqual>& arg) {
			arg.type = apply_equality(arg.left, arg.right, true);
		}

		virtual void apply(UnaryOperator<UnOpType::IntNegation>& arg) {
			apply_unary<LiteralType::Integer>(arg, [](literal_type<LiteralType::Integer>::type i){return -i;}, INT_NAME, "an int negation");
		}

		virtual void apply(UnaryOperator<UnOpType::BoolNegation>& arg) {
			apply_unary<LiteralType::Bool>(arg, [](literal_type<LiteralType::Bool>::type b){return !b;}, BOOL_NAME, "a bool negation");
		}

		virtual void apply(Literal<LiteralType::Bool>& arg) {
			side_effects = false;
			variable_access = false;
			optimized_expression = nullptr;
			arg.type = BOOL_NAME;
		}

		virtual void apply(Literal<LiteralType::Integer>& arg) {
			side_effects = false;
			variable_access = false;
			optimized_expression = nullptr;
			arg.type = INT_NAME;
		}

		virtual void apply(Literal<LiteralType::String>& arg) {
			side_effects = false;
			variable_access = false;
			optimized_expression = nullptr;
			arg.type = STR_NAME;
		}

		virtual void apply(Variable& arg) {
			/* order of resolution:
			 * 1. Local variables
			 * 2. Class functions
			 * 3. Class variables
			 * 4. self
			 * 5. Functions
			 */
			side_effects = false;
			optimized_expression = nullptr;
			if(variables.find(arg.name) != variables.end()) {
				arg.type = variables.at(arg.name).top();
				variable_access = true;
			} else if (class_name && info.classes.at(*class_name)->function_name_to_id.find(arg.name) != info.classes.at(*class_name)->function_name_to_id.end()) {
				const auto& f = info.classes.at(*class_name)->functions[info.classes.at(*class_name)->function_name_to_id.at(arg.name)];
				arg.type = build_function_type_string(f->return_type, f->args);
				variable_access = false;
				std::unique_ptr<Variable> opt_var = std::make_unique<Variable>(arg.begin, arg.begin, std::string(THIS_NAME));
				opt_var->type = *class_name;
				optimized_expression = std::make_unique<ClassMember>(arg.begin, arg.end, std::move(opt_var), std::string(arg.name));
				optimized_expression->type = arg.type;
			} else if (class_name && info.classes.at(*class_name)->variable_name_to_id.find(arg.name) != info.classes.at(*class_name)->variable_name_to_id.end()) {
				arg.type = info.classes.at(*class_name)->variables[info.classes.at(*class_name)->variable_name_to_id.at(arg.name)].first;
				variable_access = true;
				std::unique_ptr<Variable> opt_var = std::make_unique<Variable>(arg.begin, arg.begin, std::string(THIS_NAME));
				opt_var->type = *class_name;
				optimized_expression = std::make_unique<ClassMember>(arg.begin, arg.end, std::move(opt_var), std::string(arg.name));
				optimized_expression->type = arg.type;
			} else if (class_name && arg.name == THIS_NAME) {
				arg.type = *class_name;
				variable_access = true;
			} else if (info.functions.find(arg.name) != info.functions.end()) {
				const auto& f = info.functions.at(arg.name);
				arg.type = build_function_type_string(f->return_type, f->args);
				variable_access = false;
			} else if (BUILTIN_FUNCTIONS.find(arg.name) != BUILTIN_FUNCTIONS.end()) {
				const auto& f = BUILTIN_FUNCTIONS.at(arg.name);
				arg.type = build_function_type_string(f.first, f.second);
				variable_access = false;
			} else {
				push_error(arg.begin, arg.end, "use of undeclared variable/function " + arg.name + '.');
				arg.type = "";
				variable_access = false;
			}
		}

		virtual void apply(Null& arg) {
			side_effects = false;
			optimized_expression = nullptr;
			arg.type = Token::TYPE_NAMES.at(Token::Type::KEYWORD_NULL);
			variable_access = true;
		}

		virtual void apply(StaticFunctionCall& arg) {
			(void) arg;
			push_error(arg.begin, arg.end, "Internal parser error.");
		}

		virtual void apply(VirtualFunctionCall& arg) {
			(void) arg;
			push_error(arg.begin, arg.end, "Internal parser error.");
		}

		virtual void apply(CallOperator& arg) {
			std::vector<std::string> actual_args;
			bool valid = true;
			for(auto& a : arg.args) {
				a->visit(this);
				if(optimized_expression) {
					std::swap(a, optimized_expression);
				}
				actual_args.push_back(a->type);
				if(a->type.empty()) {
					valid = false;
				}
			}
			arg.fun->visit(this);
			if(optimized_expression) {
				std::swap(arg.fun, optimized_expression);
			}
			if(arg.fun->type.empty()) {
				valid = false;
			}
			if(!valid) {
				arg.type = "";
			} else {
				if(arg.fun->type[0] == '@') {
					bool valid = true;
					std::vector<std::string> declared_args = get_args_from_function_string(arg.fun->type);
					if(declared_args.size() != actual_args.size()) {
						valid = false;
					} else {
						for(size_t i = 0; i < actual_args.size(); ++i) {
							if(!does_cast_implicitly(info, actual_args[i], declared_args[i])) {
								valid = false;
							}
						}
					}
					if(!valid) {
						push_error(arg.begin, arg.end, "type " + arg.fun->type + " cannot be called with arguments: " + build_function_arg_string(actual_args));
						arg.type = "";
					} else {
						arg.type = arg.fun->type.substr(arg.fun->type.find('<') + 1, arg.fun->type.find('(') - arg.fun->type.find('<') - 1);
					}
				} else {
					push_error(arg.begin, arg.end, "type " + arg.fun->type + " does not support the call operator.");
					arg.type = "";
				}
			}
			variable_access = false;
			side_effects = true;
			optimized_expression = nullptr;
			FunctionSourceSetter fss(optimized_expression, arg.args);
			arg.fun->visit(&fss);
			optimized_expression->type = std::move(arg.type);
		}

		virtual void apply(SubscriptOperator& arg) {
			arg.index->visit(this);
			if(optimized_expression) {
				std::swap(arg.index, optimized_expression);
			}
			bool index_side_effects = side_effects;
			if(!arg.index->type.empty() && !does_cast_implicitly(info, arg.index->type, INT_NAME)) {
				push_error(arg.begin, arg.end, " cannot cast from type " + arg.index->type + " to " + INT_NAME + " as the index argument in the array subscript operator.");
			}
			arg.arr->visit(this);
			if(optimized_expression) {
				std::swap(arg.index, optimized_expression);
			}
			optimized_expression = nullptr;
			side_effects = index_side_effects || side_effects;
			if(arg.arr->type.empty()){
				arg.type = "";
			} else if(!is_array(arg.arr->type)) {
				push_error(arg.begin, arg.end, " type " + arg.arr->type + " does not support index subscript operator.");
			} else {
				arg.type = arg.arr->type.substr(0, arg.arr->type.size() - 2);
			}
		}

		virtual void apply(ClassMember& arg) {
			arg.object->visit(this);
			if(optimized_expression) {
				std::swap(arg.object, optimized_expression);
				optimized_expression = nullptr;
			}
			if(!arg.object->type.empty()) {
				if(is_array(arg.object->type)) {
					if(arg.member == LENGTH_ATTR_NAME) {
						arg.type = INT_NAME;
					} else {
						arg.type = "";
						push_error(arg.begin, arg.end, "member access operator applied to an array requests member " + arg.member + " but only " + LENGTH_ATTR_NAME + " is available.");
					}
					variable_access = false;
				} else {
					const auto ci = info.classes.find(arg.object->type);
					if(ci == info.classes.end()) {
						push_error(arg.begin, arg.end, "member access operator applied to a non-class non-array type " + arg.object->type);
						arg.type = "";
						variable_access = true;
						return;
					}
					const ClassInfo& c = *(ci->second);
					auto as_var = c.variable_name_to_id.find(arg.member);
					auto as_fun = c.function_name_to_id.find(arg.member);
					if(as_var != c.variable_name_to_id.end() && as_fun != c.function_name_to_id.end()) {
						push_error(arg.begin, arg.end, "ambiguous member access: " + arg.member + " of the class " + arg.object->type + " can be both a variable and a function.");
						arg.type = "";
						variable_access = true;
					} else if(as_var == c.variable_name_to_id.end() && as_fun == c.function_name_to_id.end()) {
						push_error(arg.begin, arg.end, "member " + arg.member + " of the class " + arg.object->type + " not found.");
						arg.type = "";
						variable_access = true;
					} else if(as_var != c.variable_name_to_id.end()) {
						arg.type = c.variables[as_var->second].first;
						variable_access = true;
					} else {
						arg.type = build_function_type_string(c.functions[as_fun->second]->return_type, c.functions[as_fun->second]->args);
						variable_access = false;
					}
				}
			}
		}

		virtual void apply(Cast& arg) {
			arg.expr->visit(this);
			if(optimized_expression) {
				std::swap(arg.expr, optimized_expression);
			}
			if(!does_explicit_cast(info, arg.expr->type, arg.target)) {
				arg.type = "";
				push_error(arg.begin, arg.end, " cannot explicitly cast from " + arg.expr->type + " to " + arg.target + '.');
			} else {
				arg.type = arg.target;
			}
		}

		virtual void apply(NewObject& arg) {
			optimized_expression = nullptr;
			side_effects = false;
			variable_access = false;
			if(info.classes.find(arg.new_type) == info.classes.end()) {
				arg.type = "";
				push_error(arg.begin, arg.end, " cannot construct a new object of unknown class " + arg.new_type + '.');
			} else {
				arg.type = arg.new_type;
			}
		}

		virtual void apply(NewArray& arg) {
			arg.size->visit(this);
			if(!arg.size->type.empty() && !does_cast_implicitly(info, arg.size->type, INT_NAME)) {
				push_error(arg.begin, arg.end, " cannot cast from type " + arg.size->type + " to " + INT_NAME + " as the size argument in the construction of an array.");
			}
			if(optimized_expression) {
				std::swap(arg.size, optimized_expression);
			}
			optimized_expression = nullptr;
			variable_access = false;
			if(info.classes.find(arg.new_type) == info.classes.end() && DEFAULT_TYPES.find(arg.new_type) == DEFAULT_TYPES.end()) {
				arg.type = "";
				push_error(arg.begin, arg.end, " cannot construct a new array of unknown type " + arg.new_type + '.');
			} else {
				arg.type = arg.new_type + "[]";
			}
		}

		virtual void apply(Assignment& arg) {
			arg.value->visit(this);
			if(optimized_expression) {
				std::swap(arg.value, optimized_expression);
			}
			arg.var->visit(this);
			if(optimized_expression) {
				std::swap(arg.var, optimized_expression);
			}
			if(!arg.value->type.empty() && !arg.var->type.empty() && !does_cast_implicitly(info, arg.value->type, arg.var->type)) {
				push_error(arg.begin, arg.end, "cannot cast from " + arg.value->type + " to " + arg.var->type + " for assignment.");
			}
			if(!arg.var->type.empty() && !variable_access) {
				push_error(arg.begin, arg.end, "assignment expects a variable.");
			}
			does_return = false;
			optimized_statement = nullptr;
		}

		void apply_inc_dec(std::unique_ptr<ProgramTree::Expression>& expr) {
			does_return = false;
			expr->visit(this);
			if(optimized_expression) {
				std::swap(expr, optimized_expression);
			}
			if(!expr->type.empty() && !does_cast_implicitly(info, expr->type, INT_NAME)) {
				push_error(expr->begin, expr->end, "cannot cast from " + expr->type + " to " + INT_NAME + " for incrementation/decrementation.");
			}
			if(!variable_access) {
				push_error(expr->begin, expr->end, "incrementation/decrementation expects a variable.");
			}
			optimized_statement = nullptr;
		}

		virtual void apply(Incrementation& arg) {
			apply_inc_dec(arg.var);
		}

		virtual void apply(Decrementation& arg) {
			apply_inc_dec(arg.var);
		}

		virtual void apply(ExprStatement& arg) {
			arg.expr->visit(this);
			if(optimized_expression) {
				std::swap(arg.expr, optimized_expression);
			}
			IsNonReturningCall inrc(does_return);
			arg.expr->visit(&inrc);
			optimized_statement = side_effects ? nullptr : std::make_unique<Empty>(arg.begin, arg.end);
		}

		virtual void apply(Return& arg) {
			optimized_statement = nullptr;
			does_return = true;
			if(!arg.val) {
				if(*declared_return_type != VOID_NAME) {
					push_error(arg.begin, arg.end, "argumentless return in a non-void function");
				}
				return;
			}
			arg.val->visit(this);
			if(optimized_expression) {
				std::swap(arg.val, optimized_expression);
			}
			if(!arg.val->type.empty() && !does_cast_implicitly(info, arg.val->type, *declared_return_type)) {
				push_error(arg.begin, arg.end, "cannot cast from " + arg.val->type + " to " + (*declared_return_type) + " in the return statement.");
			}
		}

		virtual void apply(If& arg) {
			bool ret_true, ret_false;
			arg.condition->visit(this);
			if(optimized_expression) {
				std::swap(optimized_expression, arg.condition);
			}
			if(!arg.condition->type.empty() && !does_cast_implicitly(info, arg.condition->type, BOOL_NAME)) {
				push_error(arg.begin, arg.end, "cannot cast from " + arg.condition->type + " to bool in the condition of if statement.");
			}
			arg.case_then->visit(this);
			if(optimized_statement) {
				std::swap(optimized_statement, arg.case_then);
			}
			ret_true = does_return;
			if(arg.case_else) {
				arg.case_else->visit(this);
				if(optimized_statement) {
					std::swap(optimized_statement, arg.case_else);
				}
				ret_false = does_return;
			} else {
				ret_false = false;
			}
			bool is_def;
			IsDefinition id(is_def);
			arg.case_then->visit(&id);
			if(is_def) {
				push_error(arg.begin, arg.end, "cannot define variable in if-then, try wrapping in a block.");
			}
			if(arg.case_else) {
				arg.case_else->visit(&id);
				if(is_def) {
					push_error(arg.begin, arg.end, "cannot define variable in if-else, try wrapping in a block.");
				}
			}
			const literal_type<LiteralType::Bool>::type* opt_cond = nullptr;
			LiteralGetter<LiteralType::Bool> lg(opt_cond);
			arg.condition->visit(&lg);
			optimized_statement = nullptr;
			if(opt_cond) {
				if(*opt_cond) {
					optimized_statement = std::move(arg.case_then);
					does_return = ret_true;
				} else {
					optimized_statement = std::move(arg.case_else);
					if(!optimized_statement) {
						optimized_statement = std::make_unique<Empty>(arg.end, arg.end);
					}
					does_return = ret_false;
				}
			} else {
				does_return = ret_true && ret_false;
			}
		}

		virtual void apply(While& arg) {
			arg.condition->visit(this);
			if(optimized_expression) {
				std::swap(optimized_expression, arg.condition);
			}
			if(!arg.condition->type.empty() && !does_cast_implicitly(info, arg.condition->type, BOOL_NAME)) {
				push_error(arg.condition->begin, arg.condition->end, "cannot cast from " + arg.condition->type + " to boolean in the condition of while loop.");
			}
			arg.action->visit(this);
			if(optimized_statement) {
				std::swap(optimized_statement, arg.action);
			}
			const literal_type<LiteralType::Bool>::type* opt_cond = nullptr;
			LiteralGetter<LiteralType::Bool> lg(opt_cond);
			arg.condition->visit(&lg);
			if(opt_cond) {
				if(*opt_cond) {
					does_return = true;
					optimized_statement = nullptr;
				} else {
					does_return = false;
					optimized_statement = std::make_unique<Empty>(arg.begin, arg.end);
				}
			} else {
				does_return = false;
				optimized_statement = nullptr;
			}
			bool is_def;
			IsDefinition id(is_def);
			arg.action->visit(&id);
			if(is_def) {
				push_error(arg.begin, arg.end, "cannot define variable in while, try wrapping in a block.");
			}
		}

		virtual void apply(For& arg) {
			bool is_def;
			IsDefinition id(is_def);
			arg.action->visit(&id);
			if(is_def) {
				push_error(arg.begin, arg.end, "cannot define variable in for, try wrapping in a block.");
			}
			arg.array->visit(this);
			if(optimized_expression) {
				std::swap(optimized_expression, arg.array);
			}
			if(!arg.array->type.empty()) {
				if(!is_array(arg.array->type)) {
					push_error(arg.array->begin, arg.array->end, "non-array type used as a for argument.");
				} else if (!does_cast_implicitly(info, arg.array->type.substr(0, arg.array->type.size() - 2), arg.var_type)) {
					push_error(arg.array->begin, arg.array->end, "type " + arg.array->type.substr(0, arg.array->type.size() - 2) + " does not implicitly cast to " + arg.var_type + " in for argument.");
				}
			}
			if(info.classes.find(arg.var_type) == info.classes.end() && DEFAULT_TYPES.find(arg.var_type) == DEFAULT_TYPES.end()) {
				push_error(arg.begin, arg.end, "Usage of undeclared type " + arg.var_type);
			}
			last_block_declarations.emplace(std::set<std::string>({arg.var_name}));
			variables[arg.var_name].push(arg.var_type);
			arg.action->visit(this);
			if(optimized_statement) {
				std::swap(optimized_statement, arg.action);
			}
			remove_last_variable_block();
			does_return = false;
			optimized_statement = nullptr;
		}

		virtual void apply(Block& arg) {
			last_block_declarations.emplace();
			bool valid_return = false;
			for(auto& s : arg.statements) {
				s->visit(this);
				if(optimized_statement) {
					std::swap(s, optimized_statement);
				}
				if(valid_return) {
					s = std::make_unique<Empty>(s->begin, s->end);
				} else {
					valid_return = valid_return || does_return;
				}
			}
			does_return = valid_return;
			remove_last_variable_block();
			optimized_statement = nullptr;
		}

		virtual void apply(Empty& arg) {
			(void) arg;
			does_return = false;
			optimized_statement = nullptr;
		}

		virtual void apply(Definition& arg) {
			does_return = false;
			{
				std::string type = arg.type;
				if(is_array(arg.type)) {
					type = type.substr(0, type.size() - 2);
				}
				if(info.classes.find(type) == info.classes.end() && DEFAULT_TYPES.find(type) == DEFAULT_TYPES.end()) {
					push_error(arg.begin, arg.end, "Usage of undeclared type " + type);
				}
			}
			for(auto& def : arg.defs) {
				if(def.second) {
					def.second->visit(this);
					if(optimized_expression) {
						std::swap(optimized_expression, def.second);
					}
					if(!def.second->type.empty() && !does_cast_implicitly(info, def.second->type, arg.type)) {
						push_error(arg.begin, arg.end, "cannot cast from " + def.second->type + " to " + arg.type + " in the definition of variable " + def.first);
					}
				}
				if(last_block_declarations.top().count(def.first)) {
					push_error(arg.begin, arg.end, "redeclaration of variable " + def.first);
				} else {
					variables[def.first].push(arg.type);
					last_block_declarations.top().insert(def.first);
				}
			}
			optimized_statement = nullptr;
		}

		TypeCheckerVisitor() = delete;
		TypeCheckerVisitor(const TypeInfo& info, std::list<TypeCheckerError>& errors) : info(info), errors(errors) {}
		void check(const FunctionInfo& fun, const std::string* f_name, const std::string* cl_name = nullptr) {
			declared_return_type = &(fun.return_type);
			fun_name = f_name;
			class_name = cl_name;
			does_return = false;
			last_block_declarations.emplace();
			variables.clear();
			for(const auto& a : fun.args) {
				variables[a.second].push(a.first);
				last_block_declarations.top().insert(a.second);
			}
			bool valid_return = false;
			for(auto& s : fun.data->body->statements) {
				s->visit(this);
				if(optimized_statement) {
					std::swap(s, optimized_statement);
				}
				if(valid_return) {
					s = std::make_unique<Empty>(s->begin, (*(fun.data->body->statements.rbegin()))->begin);
				} else {
					valid_return = valid_return || does_return;
				}
			}
			if(!valid_return) {
				if(fun.return_type == VOID_NAME) {
					fun.data->body->statements.push_back(std::make_unique<Return>(fun.data->body->end, fun.data->body->end, nullptr));
				} else {
					push_error(fun.data->body->end, fun.data->body->end, "not all paths return a value in a non-void function.");
				}
			}
		}
	private:
		const TypeInfo& info;
		std::map<std::string, std::stack<std::string>> variables;
		std::list<TypeCheckerError>& errors;
		const std::string* declared_return_type;
		std::unique_ptr<ProgramTree::Statement> optimized_statement;
		std::unique_ptr<ProgramTree::Expression> optimized_expression;
		std::stack<std::set<std::string>> last_block_declarations;
		const std::string* fun_name;
		bool does_return;
		bool side_effects;
		bool variable_access;
		const std::string* class_name;

		void push_error(size_t begin, size_t end, const std::string& msg) {
			errors.emplace_back(begin, end, "Function " + (*fun_name) + ": " + msg);
		}

		void remove_last_variable_block(){
			for(const auto& var : last_block_declarations.top()) {
				auto st = variables.find(var);
				st->second.pop();
				if(st->second.empty()) {
					variables.erase(st);
				}
			}
			last_block_declarations.pop();
		}
	};
}

namespace TypeChecker {

	class TypeChecker {
		const TypeInfo& info;
		std::list<TypeCheckerError> errors;

		void push_error(size_t begin, size_t end, const std::string& msg) {
			errors.emplace_back(begin, end, msg);
		}

		TypeChecker() = delete;
		TypeChecker(const TypeInfo& info) : info(info) {}

		void check_types() {
			TypeCheckerVisitor v(info, errors);
			for(const auto& f : info.functions) {
				v.check(*f.second, &f.first);
			}
			for(const auto& c : info.classes) {
				if(c.second->inheritance_tree_node->parent) {
					const ClassInfo* superclass = c.second->inheritance_tree_node->parent->class_info;
					for(const auto& fun : superclass->function_name_to_id) {
						const auto sfun = superclass->functions[fun.second];
						const auto ifun = c.second->functions[fun.second];
						if(sfun == ifun) {
							continue;
						}
						if(!does_cast_implicitly(info, ifun->return_type, sfun->return_type)) {
							push_error(ifun->data->dec_begin, ifun->data->dec_end, "Class " + c.second->data->name + ": overridden function " + fun.first + ": cannot implicitly cast type " + ifun->return_type + " to " + sfun->return_type + " as the return type.");
						}
						if(sfun->args.size() != ifun->args.size()) {
							push_error(ifun->data->dec_begin, ifun->data->dec_end, "Class " + c.second->data->name + ": overridden function " + fun.first + " has an incorrect argument count.");
							continue;
						}
						for(size_t i = 0; i < sfun->args.size(); ++i) {
							if(!does_cast_implicitly(info, sfun->return_type, ifun->return_type)) {
								push_error(ifun->data->dec_begin, ifun->data->dec_end, "Class " + c.second->data->name + ": overridden function " + fun.first + ": cannot implicitly cast type " + ifun->return_type + " to " + sfun->return_type + " as the type of argument " + std::to_string(i) + '.');
							}
						}
					}
				}
				for(const auto& f : c.second->function_name_to_id) {
					if(c.second->functions[f.second]->class_info->data->name == c.second->data->name) {
						v.check(*(c.second->functions[f.second]), &f.first, &(c.first));
					}
				}
			}
		}

		friend TypeInfo check_types(ProgramTree::Program& prog);
	};

	TypeInfo check_types(ProgramTree::Program& prog) {
		TypeInfo info = build_program_info(prog);
		TypeChecker c(info);
		c.check_types();
		if(c.errors.empty()) {
			return info;
		} else {
			throw TypeCheckerException(std::move(c.errors));
		}
	}
}
