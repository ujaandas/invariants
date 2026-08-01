#pragma once

#include <string>

#include "expression.hpp"
#include "statements.hpp"
#include "types.hpp"

namespace ast = invariants::ast;

class DependencyGraphGen {
  // Expressions
  std::string operator()(const ast::LiteralExpr& e) const;
  std::string operator()(const ast::IdentifierExpr& e) const;
  std::string operator()(const ast::ThisExpr&) const;
  std::string operator()(const ast::ListExpr& e) const;
  std::string operator()(const ast::GroupingExpr& e) const;
  std::string operator()(const ast::PostfixExpr& e) const;
  std::string operator()(const ast::MemberAccessOp& e) const;
  std::string operator()(const ast::IndexOp& e) const;
  std::string operator()(const ast::UnaryExpr& e) const;
  std::string operator()(const ast::BinaryExpr& e) const;

  // Statements
  std::string operator()(const ast::ConstraintStmt& e) const;
  std::string operator()(const ast::FieldStmt& e) const;
  std::string operator()(const ast::InvariantStmt&) const;
  std::string operator()(const ast::SpecStmt& e) const;
  std::string operator()(const ast::ModuleStmt& e) const;

  // Types
  std::string operator()(const ast::BuiltinType& e) const;
  std::string operator()(const ast::SimpleType& e) const;
  std::string operator()(const ast::ArrayType&) const;
  std::string operator()(const ast::MapType& e) const;
};