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
  std::string operator()(const ast::LiteralExpr& e);
  std::string operator()(const ast::IdentifierExpr& e);
  std::string operator()(const ast::ThisExpr&);
  std::string operator()(const ast::ListExpr& e);
  std::string operator()(const ast::GroupingExpr& e);
  std::string operator()(const ast::PostfixExpr& e);
  std::string operator()(const ast::MemberAccessOp& e);
  std::string operator()(const ast::IndexOp& e);
  std::string operator()(const ast::UnaryExpr& e);
  std::string operator()(const ast::BinaryExpr& e);

  // Types
  std::string operator()(const ast::BuiltinType& e);
  std::string operator()(const ast::SimpleType& e);
  std::string operator()(const ast::ArrayType&);
  std::string operator()(const ast::MapType& e);
};

}  // namespace invariants::resolver