#pragma once

#include "statements.hpp"
#include "symbol_table.hpp"

namespace invariants::resolver {

class Resolver {
 private:
  SymbolTable table;
  std::string currSpecName = "";
  std::string currFieldName = "";
  SpecId nextSpecId = 0;
  FieldId nextFieldId = 0;

 public:
  Resolver() = default;
  const SymbolTable& getSt() const { return table; }

  // Statements
  void operator()(ast::ConstraintStmt& e);
  void operator()(ast::FieldStmt& e);
  void operator()(ast::InvariantStmt&);
  void operator()(ast::SpecStmt& e);
  void operator()(ast::ModuleStmt& e);

  // Expressions
  void operator()(ast::LiteralExpr& e);
  void operator()(ast::IdentifierExpr& e);
  void operator()(ast::ThisExpr&);
  void operator()(ast::ListExpr& e);
  void operator()(ast::GroupingExpr& e);
  void operator()(ast::PostfixExpr& e);
  void operator()(ast::MemberAccessOp& e);
  void operator()(ast::IndexOp& e);
  void operator()(ast::UnaryExpr& e);
  void operator()(ast::BinaryExpr& e);

  // Types
  void operator()(ast::BuiltinType& e);
  void operator()(ast::SimpleType& e);
  void operator()(ast::ArrayType&);
  void operator()(ast::MapType& e);
};

}  // namespace invariants::resolver