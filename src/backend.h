#ifndef BACKEND_H
#define BACKEND_H

#include "program_tree.h"
#include "type_info_builder.h"

#include <ostream>

void emit_code(const TypeChecker::TypeInfo& info, std::ostream& output);


#endif
