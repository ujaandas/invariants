#pragma once

#include "statements.hpp"

namespace invariants::resolver {

class Resolver {
  // Statements
  void operator()(const ast::ConstraintStmt& e) const;
  void operator()(const ast::FieldStmt& e) const;
  void operator()(const ast::InvariantStmt&) const;
  void operator()(const ast::SpecStmt& e) const;
  void operator()(const ast::ModuleStmt& e) const;

  // Expressions
  //   std::string operator()(const LiteralExpr& e) const;
  //   std::string operator()(const IdentifierExpr& e) const;
  //   std::string operator()(const ThisExpr&) const;
  //   std::string operator()(const ListExpr& e) const;
  //   std::string operator()(const GroupingExpr& e) const;
  //   std::string operator()(const PostfixExpr& e) const;
  //   std::string operator()(const MemberAccessOp& e) const;
  //   std::string operator()(const IndexOp& e) const;
  //   std::string operator()(const UnaryExpr& e) const;
  //   std::string operator()(const BinaryExpr& e) const;

  // Types
  //   std::string operator()(const BuiltinType& e) const;
  //   std::string operator()(const SimpleType& e) const;
  //   std::string operator()(const ArrayType&) const;
  //   std::string operator()(const MapType& e) const;
};

}  // namespace invariants::resolver