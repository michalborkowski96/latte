#include "backend.h"
#include "program_tree.h"
#include "helper_visitors.h"

#include <functional>
#include <stack>

using namespace ProgramTree;
using namespace TypeChecker;

namespace {
	std::string encode_class_function_name(const std::string& cl, const std::string& fun) {
		return "_class_" + cl + '$' + fun;
	}

	std::string encode_constructor_name(const std::string& cl) {
		return "_class_$" + cl ;
	}

	std::string encode_vtable_name(const std::string& cl) {
		return "_class_@" + cl ;
	}

	std::string empty_string_label = "_empty_str";

	std::string empty_array_label = "_empty_arr";

	std::string str_zero = "0";

	std::string string_label(size_t id) {
		return "_string_" + std::to_string(id);
	}

	const std::string& get_def_val_for_type(const std::string& type) {
		if(is_array(type)) {
			return empty_array_label;
		}
		if(type == STR_NAME) {
			return empty_string_label;
		} else {
			return str_zero;
		}
	}

	template<typename T>
	void do_print(std::ostream& o, const T& t) {
		o << t;
	}

	template<typename T, typename ...Ts>
	void do_print(std::ostream& o, const T& t, Ts... args) {
		o << t;
		do_print(o, args...);
	}

	template<typename T>
	void print(std::ostream& o, const T& t) {
		o << t << '\n';
	}

	template<typename T, typename ...Ts>
	void print(std::ostream& o, const T& t, Ts... args) {
		o << t;
		do_print(o, args...);
		o << '\n';
	}

	class x86_64 : public ConstVisitor {
		const TypeInfo& info;
		std::ostream& output;
		size_t& label;
		std::stack<std::string> variable_names; //variables as they are on the stack
		std::map<std::string, std::stack<size_t>> variable_ids; //name to stack of numbers on the stack
		std::stack<size_t> last_block_variables; //how many variables have been declared in the current block
		const std::vector<std::pair<std::string, std::string>>& function_args;
		std::map<std::string, size_t>& string_literals;

		size_t next_label() {
			return label++;
		}

		template<typename ...Ts>
		void print(Ts... args) {
			::print(output, args...);
		}

		class GetAddr : public DefaultConstVisitor {
			x86_64* parent;
			GetAddr() = delete;
		public:
			GetAddr(x86_64* parent) : parent(parent) {}

			virtual void default_action() override {
				throw std::runtime_error("expected address of a non-variable variable, type checker error");
			}

			virtual void apply(const Variable& arg) override {
				if(parent->variable_ids.find(arg.name) != parent->variable_ids.end()) {
					parent->print("lea rax, [rsp+", (parent->variable_names.size() - parent->variable_ids.at(arg.name).top() - 1) * 8, ']');
					return;
				}
				size_t i = -1;
				for(size_t j = 0; j < parent->function_args.size(); ++j) {
					if(parent->function_args[j].second == arg.name) {
						i = j;
						break;
					}
				}
				if(i != ((size_t) -1)) {
					parent->print("lea rax, [rsp+", (parent->variable_names.size() + parent->function_args.size() - i) * 8, ']');
					return;
				}
				throw std::runtime_error("function in GetAddr::apply(const Variable&), type checker error!");
			}

			virtual void apply(const SubscriptOperator& arg) override {
				parent->get_two_variables([&](){arg.index->visit(parent);}, [&](){arg.arr->visit(parent);});
				parent->print("cmp [rax], rbx");
				parent->print("jle error");
				parent->print("lea rax, [rax + rbx * 8 + 8]");
			}

			virtual void apply(const Cast& arg) override {
				arg.expr->visit(this);
			}

			virtual void apply(const ClassMember& arg) override {
				arg.object->visit(parent);
				const ClassInfo* cl = parent->info.classes.at(arg.object->type).get();
				parent->print("add rax, ", std::to_string((cl->variable_name_to_id.at(arg.member) + 1) * 8));
			}
		};

	public:
		void get_two_variables(std::function<void()> rbx, std::function<void()> rax) {
			rbx();
			print("push rax");
			++last_block_variables.top();
			variable_names.push("");
			rax();
			print("pop rbx");
			--last_block_variables.top();
			variable_names.pop();
		}
		void cmp_bin_op(const std::unique_ptr<Expression>& l, const std::unique_ptr<Expression>& r, const std::string& op) {
			int_bin_op(l, r, "cmp");
			print(op, " bl");
			print("xor rax, rax");
			print("mov al, bl");
		}
		void int_bin_op(const std::unique_ptr<Expression>& l, const std::unique_ptr<Expression>& r, const std::string& op) {
			get_two_variables([&](){r->visit(this);}, [&](){l->visit(this);});
			print(op, " rax, rbx");
		}
		void int_div(const std::unique_ptr<Expression>& l, const std::unique_ptr<Expression>& r) {
			get_two_variables([&](){r->visit(this);}, [&](){l->visit(this);});
			print("cqo");
			print("idiv rbx");
		}
		virtual void apply(const BinaryOperator<BinOpType::Addition>& arg) {
			int_bin_op(arg.left, arg.right, "add");
		}
		virtual void apply(const BinaryOperator<BinOpType::Substraction>& arg) {
			int_bin_op(arg.left, arg.right, "sub");
		}
		virtual void apply(const BinaryOperator<BinOpType::Multiplication>& arg) {
			int_bin_op(arg.left, arg.right, "imul");
		}
		virtual void apply(const BinaryOperator<BinOpType::Division>& arg) {
			int_div(arg.left, arg.right);
		}
		virtual void apply(const BinaryOperator<BinOpType::Modulo>& arg) {
			int_div(arg.left, arg.right);
			print("mov rax, rdx");
		}
		void boolean_op(const std::unique_ptr<Expression>& l, const std::unique_ptr<Expression>& r, const std::string& jump_cmd) {
			size_t label = next_label();
			l->visit(this);
			print("test rax, rax");
			print(jump_cmd, " _boolean_op_after_", label);
			r->visit(this);
			print("_boolean_op_after_", label, ':');
		}
		virtual void apply(const BinaryOperator<BinOpType::Alternative>& arg) {
			boolean_op(arg.left, arg.right, "jnz");
		}
		virtual void apply(const BinaryOperator<BinOpType::Conjunction>& arg) {
			boolean_op(arg.left, arg.right, "jz");
		}
		virtual void apply(const BinaryOperator<BinOpType::LessThan>& arg) {
			cmp_bin_op(arg.left, arg.right, "setl");
		}
		virtual void apply(const BinaryOperator<BinOpType::LessEqual>& arg) {
			cmp_bin_op(arg.left, arg.right, "setle");
		}
		virtual void apply(const BinaryOperator<BinOpType::GreaterThan>& arg) {
			cmp_bin_op(arg.left, arg.right, "setg");
		}
		virtual void apply(const BinaryOperator<BinOpType::GreaterEqual>& arg) {
			cmp_bin_op(arg.left, arg.right, "setge");
		}
		virtual void apply(const BinaryOperator<BinOpType::Equal>& arg) {
			cmp_bin_op(arg.left, arg.right, "sete");
		}
		virtual void apply(const BinaryOperator<BinOpType::NotEqual>& arg) {
			cmp_bin_op(arg.left, arg.right, "setne");
		}
		virtual void apply(const UnaryOperator<UnOpType::IntNegation>& arg) {
			arg.expr->visit(this);
			print("imul rax, -1");
		}
		virtual void apply(const UnaryOperator<UnOpType::BoolNegation>& arg) {
			arg.expr->visit(this);
			print("test rax, rax");
			print("setz bl");
			print("xor rax, rax");
			print("mov al, bl");
		}
		virtual void apply(const Literal<LiteralType::Bool>& arg) {
			print("mov rax, ", ((short) arg.val));
		}
		virtual void apply(const Literal<LiteralType::Integer>& arg) {
			print("mov rax, ", arg.val);
		}
		virtual void apply(const Literal<LiteralType::String>& arg) {
			size_t id;
			if(string_literals.find(arg.val) != string_literals.end()) {
				id = string_literals.at(arg.val);
			} else {
				id = next_label();
				string_literals[arg.val] = id;
			}
			print("mov rax, ", string_label(id));
		}
		virtual void apply(const Variable& arg) {
			GetAddr ga(this);
			ga.apply(arg);
			print("mov rax, [rax]");
		}
		virtual void apply(const Null& arg) {
			(void) arg;
			print("mov rax, 0");
		}
		virtual void apply(const StaticFunctionCall& arg) {
			for(const auto& a : arg.args) {
				a->visit(this);
				print("push rax");
				++last_block_variables.top();
				variable_names.push("");
			}
			print("call ", arg.fun);
			last_block_variables.top() -= arg.args.size();
			for(size_t i = 0; i < arg.args.size(); ++i) {
				variable_names.pop();
			}
			if(arg.args.size()) {
				print("add rsp, ", arg.args.size() * 8);
			}
		}
		virtual void apply(const VirtualFunctionCall& arg) {
			for(const auto& a : arg.args) {
				a->visit(this);
				print("push rax");
				++last_block_variables.top();
				variable_names.push("");
			}
			arg.object->visit(this);
			print("push rax");
			print("mov rax, [rax]");
			print("add rax, ", info.classes.at(arg.object->type)->function_name_to_id.at(arg.fun) * 8);
			print("mov rax, [rax]");
			print("call rax");
			last_block_variables.top() -= arg.args.size();
			for(size_t i = 0; i < arg.args.size(); ++i) {
				variable_names.pop();
			}
			print("add rsp, ", (arg.args.size() + 1) * 8);
		}
		virtual void apply(const CallOperator& arg) {
			(void) arg;
			throw std::runtime_error("Internal type checker error.");
		}
		virtual void apply(const SubscriptOperator& arg) {
			GetAddr ga(this);
			ga.apply(arg);
			print("mov rax, [rax]");
		}
		virtual void apply(const ClassMember& arg) {
			if(is_array(arg.object->type)) {
				arg.object->visit(this);
			} else {
				GetAddr ga(this);
				ga.apply(arg);
			}
			print("mov rax, [rax]");
		}
		virtual void apply(const Cast& arg) {
			arg.expr->visit(this);
		}
		virtual void apply(const NewObject& arg)  {
			print("call ", encode_constructor_name(arg.new_type));
		}
		virtual void apply(const NewArray& arg)  {
			arg.size->visit(this);
			print("push qword ", arg.new_type == STR_NAME ? empty_string_label : std::to_string(0));
			print("push rax");
			print("call _new_array");
			print("add rsp, 16");
		}
		virtual void apply(const Assignment& arg) {
			get_two_variables([&](){arg.value->visit(this);}, [&](){GetAddr ga(this);arg.var->visit(&ga);});
			print("mov qword [rax], rbx");
		}
		virtual void apply(const Incrementation& arg) {
			GetAddr ga(this);
			arg.var->visit(&ga);
			print("inc qword [rax]");
		}
		virtual void apply(const Decrementation& arg) {
			GetAddr ga(this);
			arg.var->visit(&ga);
			print("dec qword [rax]");
		}
		virtual void apply(const ExprStatement& arg) {
			arg.expr->visit(this);
		}
		virtual void apply(const Return& arg) {
			if(arg.val) {
				arg.val->visit(this);
			}
			if(variable_names.size()) {
				print("add rsp, ", variable_names.size() * 8);
			}
			print("ret");
		}
		virtual void apply(const If& arg) {
			size_t label = next_label();
			arg.condition->visit(this);
			print("test rax, rax");
			if(arg.case_else) {
				print("jz _if_else_", label);
				arg.case_then->visit(this);
				print("jmp _if_done_", label);
				print("_if_else_", label, ':');
				arg.case_else->visit(this);
				print("_if_done_", label, ':');
			} else {
				print("jz _if_done_", label);
				arg.case_then->visit(this);
				print("_if_done_", label, ':');
			}
		}
		virtual void apply(const While& arg) {
			size_t label = next_label();
			print("jmp _while_condition_", label);
			print("_while_body_", label, ':');
			arg.action->visit(this);
			print("_while_condition_", label, ':');
			arg.condition->visit(this);
			print("test rax, rax");
			print("jnz _while_body_", label);
		}
		virtual void apply(const For& arg) {
			size_t label = next_label();
			arg.array->visit(this);

			last_block_variables.top() += 3;
			variable_names.push("");
			variable_names.push("");
			variable_ids[arg.var_name].push(variable_names.size());
			variable_names.push(arg.var_name);
			print("push rax");
			print("push qword 0");
			print("sub rsp, 8");

			print("jmp _for_condition_", label);
			print("_for_body_", label, ':');
			print("lea rax, [rbx + rax * 8 + 8]");
			print("mov rax, [rax]");
			print("mov [rsp], rax");
			arg.action->visit(this);
			print("inc qword [rsp+8]");
			print("_for_condition_", label, ':');
			print("mov rax, [rsp+8]");
			print("mov rbx, [rsp+16]");
			print("cmp rax, [rbx]");
			print("jl _for_body_", label);

			print("add rsp, 24");
			variable_ids[arg.var_name].pop();
			variable_names.pop();
			variable_names.pop();
			variable_names.pop();
			last_block_variables.top() -= 3;
		}
		virtual void apply(const Block& arg) {
			last_block_variables.push(0);
			for (const auto& s : arg.statements) {
				s->visit(this);
			}
			size_t popped_vars = last_block_variables.top();
			last_block_variables.pop();
			for(size_t i = 0; i < popped_vars; ++i) {
				variable_ids.at(variable_names.top()).pop();
				if(variable_ids.at(variable_names.top()).empty()) {
					variable_ids.erase(variable_names.top());
				}
				variable_names.pop();
			}
			if(popped_vars) {
				print("add rsp, ", popped_vars * 8);
			}
		}
		virtual void apply(const Empty& arg) {
			(void) arg;
		}
		virtual void apply(const Definition& arg) {
			for(const auto& def : arg.defs) {
				if(def.second) {
					def.second->visit(this);
					print("push rax");
				} else {
					print("push ", get_def_val_for_type(arg.type));
				}
				++last_block_variables.top();
				variable_ids[def.first].push(variable_names.size());
				variable_names.push(def.first);
			}
		}

		x86_64() = delete;
		x86_64(const TypeInfo& info, std::ostream& output, size_t& label, const std::vector<std::pair<std::string, std::string>>& function_args, std::map<std::string, size_t>& string_literals) : info(info), output(output), label(label), function_args(function_args), string_literals(string_literals) {}
	};

	void generate_constructor_and_vtable(const ClassInfo& cl, std::ostream& output) {
		print(output, encode_constructor_name(cl.data->name), ':');
		print(output, "push qword ", (cl.variables.size() + 1) * 8);
		print(output, "call _alloc");
		print(output, "add rsp, 8");
		print(output, "mov qword [rax], ", encode_vtable_name(cl.data->name));
		size_t id = 1;
		for(const std::pair<std::string, std::string>& var : cl.variables) {
			print(output, "mov qword [rax+", id * 8, "], ", get_def_val_for_type(var.first));
			++id;
		}
		print(output, "ret");
		print(output, encode_vtable_name(cl.data->name), ':');
		std::map<size_t, std::string> id_to_fun_name;
		for(const auto& fun : cl.function_name_to_id) {
			id_to_fun_name[fun.second] = fun.first;
		}
		for(size_t i = 0; i < cl.functions.size(); ++i) {
			print(output, "dq ", encode_class_function_name(cl.functions[i]->class_info->data->name, id_to_fun_name.at(i)));
		}
	}
}

void emit_code(const TypeInfo& info, std::ostream& output) {
	print(output, "section .text");
	print(output, "extern _alloc");
	print(output, "extern _new_array");
	print(output, "extern ", empty_array_label);
	print(output, "extern ", empty_string_label);
	print(output, "extern ", CONCAT_FUN_NAME);
	for(const auto& f : BUILTIN_FUNCTIONS) {
		print(output, "extern ", f.first);
	}
	print(output, "global _start");
	print(output, "_start:");
	print(output, "call main");
	print(output, "mov rdi, rax");
	print(output, "mov rax, 60");
	print(output, "syscall");
	std::map<std::string, size_t> string_literals;
	size_t label = 0;
	for(const auto& fun : info.functions) {
		print(output, fun.first, ':');
		x86_64 v(info, output, label, fun.second->args, string_literals);
		fun.second->data->body->visit(&v);
	}
	for(const auto& cl : info.classes) {
		generate_constructor_and_vtable(*(cl.second), output);
		for(const auto& fun : cl.second->function_name_to_id) {
			if(cl.second->functions[fun.second]->class_info->data->name == cl.second->data->name) {
				print(output, encode_class_function_name(cl.second->data->name, fun.first), ':');
				auto args = cl.second->functions[fun.second]->args;
				args.push_back(std::make_pair(cl.second->data->name, "self"));
				x86_64 v(info, output, label, args, string_literals);
				cl.second->functions[fun.second]->data->body->visit(&v);
			}
		}
	}
	for(const auto& str : string_literals) {
		output << string_label(str.second) << " dq " << str.first.size() << '\n';
		if(str.first.empty()) {
			continue;
		}
		output << "db " << (short) str.first[0];
		for(size_t i = 1; i < str.first.size(); ++i) {
			output << ',' << (short) str.first[i];
		}
		output << '\n';
	}
}
