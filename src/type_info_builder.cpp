#include "type_info_builder.h"

using namespace ProgramTree;

TypeChecker::ClassInfo::ClassInfo(const std::shared_ptr<ProgramTree::Class>& data) : inheritance_tree_node(nullptr), data(data) {}

TypeChecker::InheritanceTreeNode::InheritanceTreeNode(ClassInfo* class_info) : parent(nullptr), class_info(class_info) {}

TypeChecker::InheritanceTreeNode::InheritanceTreeNode(InheritanceTreeNode* parent, ClassInfo* class_info) : parent(parent), class_info(class_info) {}

TypeChecker::FunctionInfo::FunctionInfo(const std::shared_ptr<Function>& data, const std::string& return_type, std::vector<std::pair<std::string, std::string>>&& args) : data(data), return_type(return_type), args(std::move(args)) {}

TypeChecker::VirtualFunctionInfo::VirtualFunctionInfo(FunctionInfo&& base, ClassInfo* class_info) : FunctionInfo(std::move(base)), class_info(class_info) {}

TypeChecker::TypeCheckerException::TypeCheckerException(std::list<TypeCheckerError>&& errors) : errors(std::move(errors)) {}

TypeChecker::TypeCheckerError::TypeCheckerError(size_t begin, size_t end, const std::string& msg) : begin(begin), end(end), msg(msg) {}

TypeChecker::TypeCheckerError::TypeCheckerError(size_t begin, size_t end, std::string&& msg) : begin(begin), end(end), msg(std::move(msg)) {}

void TypeChecker::TypeCheckerError::operator()(std::ostream& o, const LocationTranslator& lt) const {
	o << "From " << lt(begin) << " to " << lt(end) << ":\n" << msg << "\n";
}

namespace TypeChecker {
	class InfoBuilder {
		const ProgramTree::Program& prog;
		TypeInfo result;
		std::list<TypeCheckerError> errors;

		void push_error(size_t begin, size_t end, const std::string& msg) {
			errors.emplace_back(begin, end, msg);
		}

		void gather_class_names() {
			for(const std::shared_ptr<Class>& c : prog.classes) {
				bool add = true;
				if(DEFAULT_TYPES.find(c->name) != DEFAULT_TYPES.end()) {
					push_error(c->dec_begin, c->dec_end, "Class name " + c->name + " already used by a builtin type.");
					add = false;
				}
				if(result.classes.find(c->name) != result.classes.end()) {
					push_error(c->dec_begin, c->dec_end, "Redefinition of the class " + c->name + ".");
					add = false;
				}
				if(add) {
					result.classes.emplace(std::make_pair(c->name, std::make_unique<ClassInfo>(c)));
				}
			}
		}

		InheritanceTreeNode* build_inheritance_tree_node(const std::map<std::string, std::string>& superclasses, ClassInfo& info, std::set<std::string>& call_stack) {
			if(info.inheritance_tree_node) {
				return info.inheritance_tree_node;
			}
			if(call_stack.find(info.data->name) != call_stack.end()) {
				push_error(info.data->dec_begin, info.data->dec_end, "Class " + info.data->name + " has a loop in its inheritance tree.");
				throw TypeCheckerException(std::move(errors));
			}
			if(!superclasses.at(info.data->name).empty()) {
				call_stack.insert(info.data->name);
				InheritanceTreeNode* parent = build_inheritance_tree_node(superclasses, *result.classes.at(superclasses.at(info.data->name)), call_stack);
				parent->children.emplace_back(std::make_shared<InheritanceTreeNode>(parent, &info));
				InheritanceTreeNode* node = parent->children.back().get();
				info.inheritance_tree_node = node;
				call_stack.erase(info.data->name);
				return node;
			} else {
				result.inheritance_tree.push_back(std::make_unique<InheritanceTreeNode>(&info));
				InheritanceTreeNode* node = result.inheritance_tree.back().get();
				info.inheritance_tree_node = node;
				return node;
			}
		}

		void build_inheritance_tree() {
			std::map<std::string, std::string> superclasses;
			bool err = false;
			for(const std::shared_ptr<Class>& c : prog.classes) {
				if(!c->superclass.empty() && result.classes.find(c->superclass) ==  result.classes.end()) {
					err = true;
					push_error(c->dec_begin, c->dec_end, "Class " + c->name + " extends a non-existent class " + c->superclass + '.');
				}
				superclasses[c->name] = c->superclass;
			}
			if(err) {
				throw TypeCheckerException(std::move(errors));
			}
			for(auto& c : result.classes) {
				std::set<std::string> call_stack;
				build_inheritance_tree_node(superclasses, *c.second, call_stack);
			}
		}

		bool is_valid_type(const std::string& name, bool with_void) {
			const std::string* type = &name;
			std::string sname;
			if(name.size() >= 2 && name[name.size() - 1] == ']' && name[name.size() - 2] == '[') {
				sname = name.substr(0, name.size() - 2);
				type = &sname;
			}
			if((!with_void && *type == VOID_NAME)) {
				return false;
			}
			return !(DEFAULT_TYPES.find(*type) == DEFAULT_TYPES.end() && result.classes.find(*type) == result.classes.end());
		}

		std::pair<size_t, size_t> find_class_variable_declaration_location(const Class& cl, const std::string& variable_name) {
			for(const std::unique_ptr<Definition>& var : cl.variables) {
				for(const std::pair<std::string, std::unique_ptr<Expression>>& def : var->defs) {
					if(def.first == variable_name) {
						return std::pair<size_t, size_t>(var->begin, var->end);
					}
				}
			}
			throw std::runtime_error("Type checker internal error.");
		}

		void inherit_class_variables(InheritanceTreeNode& node, std::map<std::string, std::map<std::string, std::string>>& vars) {
			node.class_info->variables = node.parent->class_info->variables;
			node.class_info->variable_name_to_id = node.parent->class_info->variable_name_to_id;
			size_t counter = node.class_info->variables.size();
			for(const auto& var : vars.at(node.class_info->data->name)) {
				if(node.class_info->variable_name_to_id.find(var.first) != node.class_info->variable_name_to_id.end()) {
					std::pair<size_t, size_t> pos = find_class_variable_declaration_location(*(node.class_info->data), var.first);
					push_error(pos.first, pos.second, "Class " + node.class_info->data->name + " redefines an inherited variable " + var.first + '.');
					continue;
				}
				node.class_info->variables.push_back(std::make_pair(var.second, var.first));
				node.class_info->variable_name_to_id[var.first] = counter++;
			}
			for(std::shared_ptr<InheritanceTreeNode>& j : node.children) {
				inherit_class_variables(*j, vars);
			}
		}

		void gather_class_variables() {
			std::map<std::string, std::map<std::string, std::string>> vars;
			for(const std::shared_ptr<Class>& c : prog.classes) {
				vars[c->name];
				for(const std::unique_ptr<Definition>& var : c->variables) {
					bool t_add = true;
					if (!is_valid_type(var->type, false)) {
						push_error(var->begin, var->end, "Class " + c->name + " contains a variable of unknown type " + var->type + '.');
						t_add = false;
					}
					std::map<std::string, std::string>& var_map = vars[c->name];
					for(const std::pair<std::string, std::unique_ptr<Expression>>& v : var->defs) {
						bool v_add = true;
						if(var_map.find(v.first) != var_map.end()) {
							push_error(var->begin, var->end, "Class " + c->name + " contains a redeclaration of variable " + v.first + '.');
							v_add = false;
						}
						if(v.second) {
							push_error(v.second->begin, v.second->end, "Class " + c->name + " contains a definition of variable " + v.first + ", expected just a declaration.");
							v_add = false;
						}
						if(t_add && v_add) {
							var_map[v.first] = var->type;
						}
					}
				}
			}
			for(std::shared_ptr<InheritanceTreeNode>& i : result.inheritance_tree) {
				size_t counter = 0;
				for(auto& var : vars.at(i->class_info->data->name)) {
					i->class_info->variables.push_back(std::make_pair(var.second, var.first));
					i->class_info->variable_name_to_id[var.first] = counter++;
				}
				for(std::shared_ptr<InheritanceTreeNode>& j : i->children) {
					inherit_class_variables(*j, vars);
				}
			}
		}

		FunctionInfo read_function_info(const std::shared_ptr<ProgramTree::Function> fun) {
			std::vector<std::pair<std::string, std::string>> args;
			bool err = false;
			if(!is_valid_type(fun->return_type, true)) {
				err = true;
				push_error(fun->dec_begin, fun->dec_end, "Function " + fun->name + " has an unknown return type " + fun->return_type + '.');
			}
			std::set<std::string> arg_names;
			for(const std::pair<std::string, std::string>& arg : fun->arguments) {
				if(arg_names.find(arg.second) != arg_names.end()) {
					err = true;
					push_error(fun->dec_begin, fun->dec_end, "Function " + fun->name + " has a redeclared argument " + arg.second + '.');
				} else {
					arg_names.insert(arg.second);
				}
				if(!is_valid_type(arg.first, false)) {
					err = true;
					push_error(fun->dec_begin, fun->dec_end, "Function " + fun->name + " has an argument of unknown type " + arg.first + '.');
				}
				args.push_back(arg);
			}
			if(!err) {
				return FunctionInfo(fun, fun->return_type, std::move(args));
			} else {
				throw TypeCheckerException(std::move(errors));
			}
		}

		void inherit_class_functions(InheritanceTreeNode& node, std::map<std::string, std::map<std::string, std::shared_ptr<VirtualFunctionInfo>>>& funs) {
			node.class_info->functions = node.parent->class_info->functions;
			node.class_info->function_name_to_id = node.parent->class_info->function_name_to_id;
			size_t counter = node.class_info->functions.size();
			for(const auto& fun : funs.at(node.class_info->data->name)) {
				auto id = node.class_info->function_name_to_id.find(fun.first);
				if(id != node.class_info->function_name_to_id.end()) {
					node.class_info->functions[id->second] = fun.second;
				} else {
					node.class_info->functions.push_back(fun.second);
					node.class_info->function_name_to_id[fun.first] = counter++;
				}
			}
			for(std::shared_ptr<InheritanceTreeNode>& j : node.children) {
				inherit_class_functions(*j, funs);
			}
		}

		void gather_class_functions() {
			std::map<std::string, std::map<std::string, std::shared_ptr<VirtualFunctionInfo>>> funs;
			for(const std::shared_ptr<Class>& c : prog.classes) {
				funs[c->name];
				for(const std::shared_ptr<Function>& fun : c->functions) {
					try {
						std::shared_ptr<VirtualFunctionInfo> fun_info(std::make_shared<VirtualFunctionInfo>(VirtualFunctionInfo(read_function_info(fun), result.classes.at(c->name).get())));
						std::map<std::string, std::shared_ptr<VirtualFunctionInfo>>& fun_map = funs.at(c->name);
						if(fun_map.find(fun->name) != fun_map.end()) {
							push_error(fun->dec_begin, fun->dec_end, "Class " + c->name + ": redefinition of function " + fun->name);
							continue;
						}
						if(result.classes.find(c->name) != result.classes.end() && result.classes.at(c->name)->variable_name_to_id.find(fun->name) != result.classes.at(c->name)->variable_name_to_id.end()) {
							push_error(fun->dec_begin, fun->dec_end, "Class " + c->name + ": function " + fun->name + " shadows a variable.");
						}
						fun_map.emplace(std::make_pair(fun->name, std::move(fun_info)));
					} catch (TypeCheckerException& except) {
						errors = std::move(except.errors);
					}
				}
			}
			for(std::shared_ptr<InheritanceTreeNode>& i : result.inheritance_tree) {
				size_t counter = 0;
				for(auto& fun : funs.at(i->class_info->data->name)) {
					i->class_info->functions.push_back(std::move(fun.second));
					i->class_info->function_name_to_id[fun.first] = counter++;
				}
				for(std::shared_ptr<InheritanceTreeNode>& j : i->children) {
					inherit_class_functions(*j, funs);
				}
			}
		}

		void gather_functions() {
			for(const std::shared_ptr<Function>& fun : prog.functions) {
				if(fun->name == "main") {
					if(fun->return_type != INT_NAME) {
						push_error(fun->dec_begin, fun->dec_end, "main() function should have int return type.");
					}
					if(!fun->arguments.empty()){
						push_error(fun->dec_begin, fun->dec_end, "main() function should take no arguments.");
					}
				}
				if(BUILTIN_FUNCTIONS.find(fun->name) != BUILTIN_FUNCTIONS.end()) {
					push_error(fun->dec_begin, fun->dec_end, fun->name + "() function name conflicts with a builtin one.");
				}
				try {
					FunctionInfo f = read_function_info(fun);
					if(result.functions.find(fun->name) != result.functions.end()) {
						push_error(fun->dec_begin, fun->dec_end, "Redefinition of function " + fun->name + ".");
					} else {
						result.functions.emplace(std::make_pair(fun->name, std::make_shared<FunctionInfo>(f)));
					}
				} catch (TypeCheckerException& except) {
					errors = std::move(except.errors);
				}
			}
			if(result.functions.find("main") == result.functions.end()) {
				push_error(-1, -1, "main() function not found.");
			}
		}

		InfoBuilder() = delete;

		InfoBuilder(const ProgramTree::Program& prog) : prog(prog) {}

		friend TypeInfo build_program_info(const Program& prog);
	};

	TypeInfo build_program_info(const Program& prog) {
		InfoBuilder b(prog);
		b.gather_class_names();
		b.build_inheritance_tree();
		b.gather_functions();
		b.gather_class_variables();
		b.gather_class_functions();
		if(!b.errors.empty()) {
			throw TypeCheckerException(std::move(b.errors));
		}
		return b.result;
	}
}
