#pragma once

#include <vector>

#include "expression.hpp"
#include "types.hpp"

namespace invariants::ast {

// Forward decl
struct Stmt;
using StmtPtr = std::unique_ptr<Stmt>;

struct ConstraintStmt {
  ExprPtr expression;
};

struct FieldStmt {
  std::string identifier;
  TypePtr type;
  std::vector<ConstraintStmt> constraints;
};

struct InvariantStmt {
  std::string identifier;  // simplified (no need for IdentifierExpr)
  std::vector<ConstraintStmt> constraints;
};

using SpecMember = std::variant<FieldStmt, InvariantStmt>;

struct SpecStmt {
  std::string identifier;
  std::vector<SpecMember> members;
};

struct ModuleStmt {
  std::vector<SpecStmt> specs;
};

struct Stmt {
  using StmtT = std::variant<ConstraintStmt, FieldStmt, InvariantStmt, SpecStmt,
                             ModuleStmt>;

  StmtT value;

  template <typename T>
  Stmt(T v) : value(std::move(v)) {}
};

}  // namespace invariants::ast