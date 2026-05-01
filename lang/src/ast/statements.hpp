#pragma once

#include <vector>

#include "expression.hpp"
#include "types.hpp"

namespace invariants::ast {

struct ConstraintStmt : Node {
  ExprPtr expression;
};

struct SpecMember : Node {
  virtual ~SpecMember() = default;
};

struct FieldStmt : SpecMember {
  std::string identifier;
  TypePtr type;
  std::vector<ConstraintStmt> constraints;
};

struct InvariantStmt : SpecMember {
  IdentifierPtr identifier;
  std::vector<ConstraintStmt> constraints;
};

struct SpecStmt : Node {
  std::string identifier;
  std::vector<SpecMember> members;
};

struct ModuleStmt : Node {
  std::vector<SpecStmt> specs;
};

}  // namespace invariants::ast