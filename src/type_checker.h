#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "type_info_builder.h"

namespace TypeChecker {
	TypeInfo check_types(ProgramTree::Program& prog);
}

#endif
