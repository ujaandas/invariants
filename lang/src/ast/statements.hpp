#pragma once

#include <vector>

#include "expression.hpp"
#include "types.hpp"

namespace invariants::ast {

struct Constraint : Node {
  ExprPtr expression;
};

struct Invariant : Node {
  IdentifierPtr identifier;
  std::vector<Constraint> constraints;
};

struct Field : Node {
  std::string identifier;
  TypePtr type;
  std::vector<Constraint> constraints;
};

using SpecMember = std::variant<Field, Invariant>;

struct Spec : Node {
  std::string identifier;
  std::vector<SpecMember> members;
};

struct Module : Node {
  std::vector<Spec> specs;
};

}  // namespace invariants::ast