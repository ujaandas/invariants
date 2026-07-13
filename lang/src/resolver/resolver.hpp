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
  //   std::string operator()(const LiteralExpr& e);
  //   std::string operator()(const IdentifierExpr& e);
  //   std::string operator()(const ThisExpr&);
  //   std::string operator()(const ListExpr& e);
  //   std::string operator()(const GroupingExpr& e);
  //   std::string operator()(const PostfixExpr& e);
  //   std::string operator()(const MemberAccessOp& e);
  //   std::string operator()(const IndexOp& e);
  //   std::string operator()(const UnaryExpr& e);
  //   std::string operator()(const BinaryExpr& e);

  // Types
  //   std::string operator()(const BuiltinType& e);
  //   std::string operator()(const SimpleType& e);
  //   std::string operator()(const ArrayType&);
  //   std::string operator()(const MapType& e);
};

}  // namespace invariants::resolver