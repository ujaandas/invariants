#pragma once

#include <vector>

#include "expression.hpp"
#include "types.hpp"

namespace invariants::ast {

struct ConstraintStmt : Node {
  ExprPtr expression;
};

struct InvariantStmt : Node {
  IdentifierPtr identifier;
  std::vector<ConstraintStmt> constraints;
};

struct FieldStmt : Node {
  std::string identifier;
  TypePtr type;
  std::vector<ConstraintStmt> constraints;
};

using SpecMember = std::variant<FieldStmt, InvariantStmt>;

struct SpecStmt : Node {
  std::string identifier;
  std::vector<SpecMember> members;
};

struct ModuleStmt : Node {
  std::vector<SpecStmt> specs;
};

}  // namespace invariants::ast