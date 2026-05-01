#pragma once

#include <memory>
#include <vector>

#include "expression.hpp"
#include "types.hpp"

namespace invariants::ast {

struct Node {
  virtual ~Node() = default;
};

struct Constraint : Node {
  std::unique_ptr<Expr> expression;
};

struct Invariant : Node {
  std::string identifier;
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