#ifndef TYPE_INFO_BUILDER_H
#define TYPE_INFO_BUILDER_H

#include "program_tree.h"
#include "location.h"

#include <map>
#include <string>
#include <set>
#include <sstream>
#include <list>

namespace TypeChecker {
	struct ClassInfo;
	class Checker;
	class InfoBuilder;

	struct InheritanceTreeNode {
		std::vector<std::shared_ptr<InheritanceTreeNode>> children;
		InheritanceTreeNode* parent;
		ClassInfo* class_info;
		InheritanceTreeNode(InheritanceTreeNode* parent, ClassInfo* class_info);
		InheritanceTreeNode(ClassInfo* class_info);
	private:
		friend class InfoBuilder;
		InheritanceTreeNode() = delete;
		InheritanceTreeNode(const InheritanceTreeNode&) = delete;
		InheritanceTreeNode(InheritanceTreeNode&&) = delete;
		InheritanceTreeNode& operator=(const InheritanceTreeNode&) = delete;
		InheritanceTreeNode& operator=(InheritanceTreeNode&&) = delete;
	};

	struct FunctionInfo {
		const std::shared_ptr<ProgramTree::Function> data;
		std::string return_type;
		std::vector<std::pair<std::string, std::string>> args;
	private:
		friend class InfoBuilder;
		FunctionInfo() = delete;
		FunctionInfo(const std::shared_ptr<ProgramTree::Function>& data, const std::string& return_type, std::vector<std::pair<std::string, std::string>>&& args);
	};

	struct VirtualFunctionInfo : public FunctionInfo {
		ClassInfo* class_info;
	private:
		VirtualFunctionInfo() = delete;
		VirtualFunctionInfo(FunctionInfo&& base, ClassInfo* class_info);
		friend class InfoBuilder;
	};

	struct ClassInfo {
		std::vector<std::shared_ptr<VirtualFunctionInfo>> functions;
		std::map<std::string, size_t> function_name_to_id;
		std::vector<std::pair<std::string, std::string>> variables;
		std::map<std::string, size_t> variable_name_to_id;
		InheritanceTreeNode* inheritance_tree_node;
		const std::shared_ptr<ProgramTree::Class> data;
		ClassInfo(const std::shared_ptr<ProgramTree::Class>& data);
	private:
		friend class InfoBuilder;
		ClassInfo() = delete;
		ClassInfo(const ClassInfo&) = delete;
		ClassInfo(ClassInfo&&) = delete;
		ClassInfo& operator=(const ClassInfo&) = delete;
		ClassInfo& operator=(ClassInfo&&) = delete;
	};

	struct TypeInfo {
		std::map<std::string, std::shared_ptr<ClassInfo>> classes;
		std::map<std::string, std::shared_ptr<FunctionInfo>> functions;
		std::vector<std::shared_ptr<InheritanceTreeNode>> inheritance_tree;
	private:
		TypeInfo() = default;
		friend class InfoBuilder;
	};

	class TypeCheckerError {
		const size_t begin;
		const size_t end;
		const std::string msg;
	public:
		TypeCheckerError() = delete;
		TypeCheckerError(size_t begin, size_t end, const std::string& msg);
		TypeCheckerError(size_t begin, size_t end, std::string&& msg);
		void operator()(std::ostream& o, const LocationTranslator& lt) const;
	};

	struct TypeCheckerException {
		std::list<TypeCheckerError> errors;
		TypeCheckerException() = delete;
		TypeCheckerException(std::list<TypeCheckerError>&& errors);
	};

	TypeInfo build_program_info(const ProgramTree::Program& prog);
}

#endif
