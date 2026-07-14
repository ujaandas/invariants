#pragma once

#include "statements.hpp"
#include "symbol_table.hpp"

namespace invariants::resolver {

class Resolver {
 private:
  SymbolTable table;
  std::string currSpecName = "";
  SpecId nextSpecId = 0;
  FieldId nextFieldId = 0;

 public:
  Resolver() = default;
  const SymbolTable& getSt() const { return table; }

  // Statements
  void operator()(const ast::ConstraintStmt& e);
  void operator()(const ast::FieldStmt& e);
  void operator()(const ast::InvariantStmt&);
  void operator()(const ast::SpecStmt& e);
  void operator()(const ast::ModuleStmt& e);

  // Expressions
  void operator()(const ast::LiteralExpr& e);
  void operator()(const ast::IdentifierExpr& e);
  void operator()(const ast::ThisExpr&);
  void operator()(const ast::ListExpr& e);
  void operator()(const ast::GroupingExpr& e);
  void operator()(const ast::PostfixExpr& e);
  void operator()(const ast::MemberAccessOp& e);
  void operator()(const ast::IndexOp& e);
  void operator()(const ast::UnaryExpr& e);
  void operator()(const ast::BinaryExpr& e);

  // Types
  void operator()(const ast::BuiltinType& e);
  void operator()(const ast::SimpleType& e);
  void operator()(const ast::ArrayType&);
  void operator()(const ast::MapType& e);
};

}  // namespace invariants::resolver