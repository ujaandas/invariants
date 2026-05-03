#pragma once

#include <memory>
#include <vector>

#include "expression.hpp"
#include "types.hpp"

namespace invariants::ast {

struct ConstraintStmt {
  ExprPtr expression;
};

using ConstraintPtr = std::unique_ptr<ConstraintStmt>;

struct FieldStmt {
  std::string identifier;
  TypePtr type;
  std::vector<ConstraintPtr> constraints;
};

struct InvariantStmt {
  std::string identifier;  // simplified (no need for IdentifierExpr)
  std::vector<ConstraintPtr> constraints;
};

using SpecMember = std::variant<FieldStmt, InvariantStmt>;

struct SpecStmt {
  std::string identifier;
  std::vector<SpecMember> members;
};

using SpecPtr = std::unique_ptr<SpecStmt>;

struct ModuleStmt {
  std::vector<SpecPtr> specs;
};

struct Stmt {
  using StmtT = std::variant<ConstraintStmt, FieldStmt, InvariantStmt, SpecStmt,
                             ModuleStmt>;

  StmtT value;

  template <typename T>
  explicit Stmt(T v) : value(std::move(v)) {}
};

}  // namespace invariants::ast